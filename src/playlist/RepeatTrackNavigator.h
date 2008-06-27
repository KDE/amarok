/***************************************************************************
 * copyright       : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com> *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU General Public License as          *
 * published by the Free Software Foundation; either version 2 of          *
 * the License, or (at your option) any later version.                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef REPEATTRACKNAVIGATOR_H
#define REPEATTRACKNAVIGATOR_H

#include "meta/Meta.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistRowList.h"
#include "playlist/TrackNavigator.h"

namespace Playlist {
class Model;
/**
 * Simply plays the next track and stops playing when the playlist is finished.
 */
    class RepeatTrackNavigator : public TrackNavigator
    {
        public:
            RepeatTrackNavigator( Model* m ) : TrackNavigator( m ) {}
            int nextRow();
            int userNextRow();

        private:
            RowList m_previousTrack; // always only has one or zero elements
    };

 }

#endif
