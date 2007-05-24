/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_TRACKADVANCER_H
#define AMAROK_TRACKADVANCER_H

namespace PlaylistNS {

class Model;

/**
 * An abstract class which defines what should be done after a track finishes playing.
 * The Playlist::Model will have an object of the currently active strategy.
 * It is the "strategy" pattern from the Design Patterns book. In Amarok 1.x, the Playlist 
 * became very confusing due to random mode and dynamic playlists requiring complicated
 * nested if statements. This should prevent that.
 */
    class TrackAdvancer
    {
        public: 
            TrackAdvancer( Model* model ) : m_playlistModel( model ) { }
            virtual ~TrackAdvancer() { }
            /// Performs actions that need to be done after a track has finished playing.
            virtual void advanceTrack() = 0;
        protected:
            ///Convenience function, set the current track in the playlistmodel and play it.
            ///@param position position in Model of track to start playing
            void setCurrentTrack( int position );
            Model* m_playlistModel; //! needed to manipulate the playlist
    };
}

#endif
