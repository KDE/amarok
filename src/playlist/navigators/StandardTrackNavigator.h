/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef STANDARDTRACKNAVIGATOR_H
#define STANDARDTRACKNAVIGATOR_H

#include "TrackNavigator.h"

namespace Playlist
{
    /**
     * Simply plays the next track.
     */
    class StandardTrackNavigator : public TrackNavigator
    {
        public:
            StandardTrackNavigator();

            quint64 likelyNextTrack() override { return chooseNextTrack( m_repeatPlaylist ); }
            quint64 likelyLastTrack() override { return chooseLastTrack( m_repeatPlaylist ); }
            quint64 requestNextTrack() override;
            quint64 requestUserNextTrack() override;
            quint64 requestLastTrack() override;

        private:
            /**
             * This function does the same job as 'likelyNextTrack()'. It exists as a
             * distinct function because 'requestNextTrack()' should not call
             * 'likelyNextTrack()': a child class can override that to do something
             * unexpected (e.g. child class 'RepeatTrackNavigator').
             */
            quint64 chooseNextTrack( bool repeatPlaylist );

            /**
             * This function does the same job as 'likelyLastTrack()'. It exists as a
             * distinct function because 'requestLastTrack()' should not call
             * 'likelyLastTrack()': a child class can override that to do something
             * unexpected (e.g. child class 'RepeatTrackNavigator').
             */
            quint64 chooseLastTrack( bool repeatPlaylist );

            // repeat the entire playlist when we've reached the end
            bool m_repeatPlaylist;
            // only play items explicitly queued
            bool m_onlyQueue;
    };

}

#endif
