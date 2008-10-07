/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
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

#ifndef AMAROK_SIMPLETRACKNAVIGATOR_H
#define AMAROK_SIMPLETRACKNAVIGATOR_H

#include "TrackNavigator.h"

namespace Playlist
{
    /** A abstract class that provides a simpler interface to implement for
     * navigators that don't need to do any extra work or require any kind of
     * threading (which is really every navigator except DynamicTrackNavigator).
     */

    typedef QList<quint64> AlbumTrackList; // used in RandomAlbum and RepeatAlbum

    class SimpleTrackNavigator : public TrackNavigator
    {
        public:
            SimpleTrackNavigator();
            virtual ~SimpleTrackNavigator() {}

            void requestNextTrack();
            void requestUserNextTrack();
            void requestLastTrack();

        protected:
            virtual int nextRow() = 0;
            virtual int userNextRow() { return nextRow(); }
            virtual int lastRow();
    };
}
 


#endif

