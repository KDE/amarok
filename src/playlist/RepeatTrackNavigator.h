/***************************************************************************
 * copyright       : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_REPEATTRACKADVANCER_H
#define AMAROK_REPEATTRACKADVANCER_H

#include "meta/Meta.h"
#include "playlist/TrackNavigator.h"


namespace Playlist {
class Model;
/**
 * Simply plays the next track and stops playing when the playlist is finished.
 */
    class RepeatTrackNavigator : public TrackNavigator
    {
        public:
            RepeatTrackNavigator( Model* m ) : TrackNavigator( m ) { }
            void advanceTrack();
            void userAdvanceTrack();

        private:
            Meta::TrackPtr m_previousTrack;
    };

 }

#endif
