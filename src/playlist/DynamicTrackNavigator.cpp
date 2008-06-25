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
            this, SLOT(removePlayed()));
}

Meta::TrackPtr
DynamicTrackNavigator::nextTrack()
{
    DEBUG_BLOCK


    appendUpcoming();

    Meta::TrackPtr activeTrack = m_playlistModel->activeTrack();
    for( int i = 0; i < m_upcomingTracks.size(); ++i )
    {
        if( m_upcomingTracks[i] == activeTrack )
        {
            m_upcomingTracks.removeAt(i);
            m_playedTracks.push_back( activeTrack );
            break;
        }
    }

    int updateRow = m_playlistModel->activeRow() + 1;

    if( m_playlistModel->stopAfterMode() == StopAfterCurrent )
    {
        return Meta::TrackPtr();
    }
    else if( updateRow > m_playlistModel->rowCount() )
    {
        warning() << "DynamicPlaylist is not delivering.";
        return Meta::TrackPtr();
    }
    else 
    {
        return m_playlistModel->itemList().at(updateRow)->track();
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
        m_upcomingTracks += newUpcoming;
        m_playlistModel->insertOptioned( newUpcoming, Append | Colorize );
    }
}

void DynamicTrackNavigator::removePlayed()
{
    DEBUG_BLOCK

    QList<Item*> items = m_playlistModel->itemList();
    Meta::TrackPtr poped;
    while( m_playedTracks.size() > m_playlist->previousCount() )
    {
        poped = m_playedTracks.front();
        m_playedTracks.pop_front();

        for( int i = m_playlistModel->activeRow()-1; i >= 0; --i )
        {
            if( items[i]->track() == poped ) 
            {
                items.removeAt(i);
                m_playlistModel->removeRows( i, 1 );
                break;
            }
        }
    }
}

