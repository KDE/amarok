/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu> 
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
#ifndef AMAROK_TRACKADVANCER_H
#define AMAROK_TRACKADVANCER_H

#include "meta/Meta.h"

namespace Playlist {

class Model;

/**
 * An abstract class which defines what should be done after a track finishes playing.
 * The Playlist::Model will have an object of the currently active strategy.
 * It is the "strategy" pattern from the Design Patterns book. In Amarok 1.x, the Playlist 
 * became very confusing due to random mode and dynamic playlists requiring complicated
 * nested if statements. This should prevent that.
 */
    class TrackNavigator
    {
        public: 
            TrackNavigator( Model* model ) : m_playlistModel( model ) { }
            virtual ~TrackNavigator() { }
            /** The next track that the engine should play.  This is called a few seconds before the track actually ends */
            virtual Meta::TrackPtr nextTrack() = 0;
            /// The user clicks next.
            virtual Meta::TrackPtr userNextTrack() { return nextTrack(); }
            ///The user clicks previous. By default it just go to the previous item in the playlist
            virtual Meta::TrackPtr lastTrack();
        protected:
            ///Convenience function, set the current track in the playlistmodel and play it.
            ///@param position position in Model of track to start playing
            void setCurrentTrack( int position );
            Model* m_playlistModel; //! needed to manipulate the playlist
    };
}

#endif
