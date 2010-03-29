/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef MEDIADEVICEHANDLER_CAPABILITY_H
#define MEDIADEVICEHANDLER_CAPABILITY_H


#include "mediadevicecollection_export.h"

#include <QObject>

namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT Capability : public QObject
    {
        Q_OBJECT
        Q_ENUMS( Type )

        public:
            //add additional capabilities here
            enum Type { Unknown = 0
                        , Readable = 1 // can read from device
                        , Writable = 2 // can write to device
                        , Playlist = 3 // can read/write playlists
                        , Artwork = 4 // can read/write artwork
                        , Podcast = 5 // can read/write podcasts
                      };

            virtual ~Capability();

    };
}


#endif
