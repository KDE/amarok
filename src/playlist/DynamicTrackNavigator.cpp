/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "DynamicTrackNavigator.h"

#include "Debug.h"
#include "DynamicPlaylist.h"
#include "EngineController.h"
#include "Meta.h"
#include "PlaylistModel.h"

using namespace Playlist;

DynamicTrackNavigator::DynamicTrackNavigator( Model* m, Meta::DynamicPlaylistPtr p )
    : TrackNavigator(m), m_playlist(p)
{
    QObject::connect( m_playlistModel, SIGNAL(activeRowChanged()),
            this, SLOT(activeRowChanged()));
    QObject::connect( m_playlistModel, SIGNAL(activeRowExplicitlyChanged()),
            this, SLOT(activeRowExplicitlyChanged()));
}


int
DynamicTrackNavigator::nextRow()
{
    appendUpcoming();

    int activeRow = m_playlistModel->activeRow();


    if( m_upcomingRows.contains( activeRow ) ) setAsPlayed( activeRow );
    m_upcomingRows.removeAll( activeRow );
    int updateRow = activeRow + 1;

    if( m_playlistModel->stopAfterMode() == StopAfterCurrent ) return -1;
    else if( m_playlistModel->rowExists( updateRow ) )         return updateRow;
    else
    {
        warning() << "DynamicPlaylist is not delivering.";
        return -1;
    }
}

void DynamicTrackNavigator::appendUpcoming()
{
    int updateRow = m_playlistModel->activeRow() + 1;
    int rowCount = m_playlistModel->rowCount();
    int upcomingCountLag = m_playlist->upcomingCount() - (rowCount - updateRow);

    if( upcomingCountLag > 0 ) 
    {
        Meta::TrackList newUpcoming = m_playlist->getTracks( upcomingCountLag );
        m_playlistModel->insertOptioned( newUpcoming, Append | Colorize );

        for( int row = 1; row <= newUpcoming.size(); ++row )
        {
            setAsUpcoming( m_playlistModel->rowCount() - row );
        }
    }
}

void DynamicTrackNavigator::removePlayed()
{
    int playedRow;
    while( m_playedRows.size() > m_playlist->previousCount() )
    {
        playedRow = m_playedRows.front();
        m_playedRows.pop_front();

        m_playlistModel->removeRows( playedRow, 1 );
    }
}

void DynamicTrackNavigator::activeRowChanged()
{
    removePlayed();
}


void DynamicTrackNavigator::activeRowExplicitlyChanged()
{
    DEBUG_BLOCK

    int activeRow = m_playlistModel->activeRow();

    // move played tracks that follow it into upcomming
    QMutableListIterator<int> i( m_playedRows );
    while( i.hasNext() )
    {
        debug() << "rearranging played track...";
        i.next();
        if( i.value() >= activeRow )
        {
            setAsUpcoming( i.value() );
            i.remove();
        }
    }
    
    // move upcoming that precede it into played
    QMutableListIterator<int> j( m_upcomingRows );
    while( j.hasNext() )
    {
        debug() << "rearranging upcoming track...";
        j.next();
        if( j.value() < activeRow )
        {
            setAsPlayed( j.value() );
            j.remove();
        }
    }

    removePlayed();
    appendUpcoming();
}

void DynamicTrackNavigator::setAsUpcoming( int row )
{
    m_upcomingRows.append( row );
    QModelIndex i = m_playlistModel->index( row, 0 );
    i.data( ItemRole ).value< Playlist::Item* >()->setState( Item::DynamicUpcoming );
}

void DynamicTrackNavigator::setAsPlayed( int row )
{
    m_playedRows.append( row );
    QModelIndex i = m_playlistModel->index( row, 0 );
    i.data( ItemRole ).value< Playlist::Item* >()->setState( Item::DynamicPlayed );
}

