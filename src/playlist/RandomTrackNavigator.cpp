/***************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>    
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

#include "RandomTrackNavigator.h"

#include "Debug.h"
#include "PlaylistItem.h"
#include "PlaylistModel.h"

#include <KRandom>

using namespace Playlist;

Meta::TrackPtr
RandomTrackNavigator::nextTrack()
{
    if( playlistChanged() )
    {
        debug() << "Playlist has changed, regenerating unplayed tracks";
        generateUnplayedTracks();
        TrackNavigator::playlistChangeHandled();
    }

    Meta::TrackPtr lastTrack = m_playlistModel->activeTrack();
    m_playedTracks.append( lastTrack ); 
    m_unplayedTracks.removeAll( lastTrack );

    if( !m_unplayedTracks.isEmpty() && m_playlistModel->stopAfterMode() != StopAfterCurrent )
    {
        int nextRow = KRandom::random() % m_unplayedTracks.count();
        return m_unplayedTracks.at( nextRow );
    }

    m_playedTracks.clear();
    TrackNavigator::setPlaylistChanged(); // will cause generateUnplayedTracks() to be called
    debug() << "There are no more tracks to play, starting over";
    
    // out of tracks to play or stopAfterMode == Current.
    return Meta::TrackPtr();
}

void
RandomTrackNavigator::generateUnplayedTracks()
{
    m_unplayedTracks.clear();
    foreach( Item *i, m_playlistModel->itemList() )
    {
        if( i && !m_playedTracks.contains( i->track() ) )
            m_unplayedTracks.append( i->track() );
    }
}

