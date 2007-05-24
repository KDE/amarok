/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_STANDARDTRACKADVANCER_H
#define AMAROK_STANDARDTRACKADVANCER_H

#include "playlist/TrackAdvancer.h"

namespace PlaylistNS {
class Model;
/**
 * Simply plays the next track and stops playing when the playlist is finished.
 */
    class StandardTrackAdvancer : public TrackAdvancer
    {
        public:
            StandardTrackAdvancer( Model* m ) : TrackAdvancer( m ) { }
            void advanceTrack();
    };

 }

#endif
