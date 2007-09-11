/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_STANDARDTRACKADVANCER_H
#define AMAROK_STANDARDTRACKADVANCER_H

#include "playlist/TrackNavigator.h"

namespace Playlist {
class Model;
/**
 * Simply plays the next track and stops playing when the playlist is finished.
 */
    class StandardTrackNavigator : public TrackNavigator
    {
        public:
            StandardTrackNavigator( Model* m ) : TrackNavigator( m ) { }
            void advanceTrack();
    };

 }

#endif
