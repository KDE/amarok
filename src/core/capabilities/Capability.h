/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_CAPABILITY_H
#define AMAROK_CAPABILITY_H

#include "amarokcore_export.h"

#include <QObject>

namespace Capabilities
{
    /** The capabilities are used by several amarok objects to express add on functionality.
        Capabilities are used inside Amarok to implement a Java-like interface pattern.
        Several object (Collection, Track, Album) can return capability object.
        Since the implementation of these objects is in a library usually it would
        be otherwise difficult to implement something like this.

        Please note that the capability object will be created on demand and
        also destroyed.
    */
    class AMAROKCORE_EXPORT Capability : public QObject
    {
        Q_OBJECT

        public:
            //add additional capabilities here
            enum Type { Unknown = 0
                        // not longer used
                        // not longer used
                        , Buyable = 3
                        , Actions = 4
                        , EditablePlaylist = 5
                        , MultiPlayable = 6
                        , Organisable = 7
                        , SourceInfo = 8
                        // not longer used
                        , StreamInfo = 10
                        // not longer used
                        // not longe used
                        // not longer used
                        , BookmarkThis = 14
                        , WriteTimecode = 15
                        , LoadTimecode = 16
                        , MultiSource = 17
                        , BoundedPlayback = 18
                        // not longer used
                        , ReadLabel = 20
                        , WriteLabel = 21
                        , FindInSource = 22
                        , CollectionImport = 23
                        , CollectionScan = 24
                        , Transcode = 25
                      };
            Q_ENUM( Type )

            virtual ~Capability();

    };
}


#endif
