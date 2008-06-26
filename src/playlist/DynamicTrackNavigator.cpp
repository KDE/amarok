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
    : TrackNavigator(m), m_playedRows(m), m_playlist(p)
{
    QObject::connect( m_playlistModel, SIGNAL(activeRowChanged()),
            this, SLOT(removePlayed()));
}


int
DynamicTrackNavigator::nextRow()
{
    DEBUG_BLOCK

    appendUpcoming();

    m_playedRows.append( m_playlistModel->activeRow () );
    int updateRow = m_playlistModel->activeRow() + 1;

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
    }
}

void DynamicTrackNavigator::removePlayed()
{
    DEBUG_BLOCK


    int playedRow;
    while( m_playedRows.size() > m_playlist->previousCount() )
    {
        playedRow = m_playedRows.front();
        m_playedRows.pop_front();

        m_playlistModel->removeRows( playedRow, 1 );
    }
}

