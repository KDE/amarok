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

#include "amarok_export.h"

#include <QObject>

namespace Meta
{
    class AMAROK_EXPORT Capability : public QObject
    {
        Q_OBJECT
        Q_ENUMS( Type )

        public:
            //add additional capabilities here
            enum Type { Unknown = 0
                        , Editable = 1
                        , LastFm = 2
                        , Buyable = 3
                        , CustomActions = 4
                        , EditablePlaylist = 5
                        , MultiPlayable = 6
                        , Organisable = 7
                        , SourceInfo = 8
                        , CurrentTrackActions = 9
                        , StreamInfo = 10
                        , Updatable = 11
                        , Importable = 12
                        , Collection = 13
                        , BookmarkThis = 14
                        , WriteTimecode = 15
                        , LoadTimecode = 16
                        , MultiSource = 17
                        , BoundedPlayback = 18
                        , Decorator
                      };

            virtual ~Capability();

    };
}


#endif
