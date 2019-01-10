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

#ifndef AMAROK_METAUTILITY_H
#define AMAROK_METAUTILITY_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"
#include "core/meta/support/MetaConstants.h"

#include <QMap>
#include <QString>
#include <QVariant>

namespace Meta
{
    class Track;

    namespace Field
    {

        //deprecated
        AMAROK_CORE_EXPORT QVariantMap mapFromTrack( const Meta::TrackPtr &track );
        //this method will return a map with keys that are compatible to the fdo MPRIS 1.0 specification
        AMAROK_CORE_EXPORT QVariantMap mprisMapFromTrack( const Meta::TrackPtr &track );
        //this method will return a map with keys that are compatible to the fdo MPRIS 2.0 specification
        AMAROK_CORE_EXPORT QVariantMap mpris20MapFromTrack( const Meta::TrackPtr &track );
        AMAROK_CORE_EXPORT void updateTrack( Meta::TrackPtr track, const QVariantMap &metadata );
        AMAROK_CORE_EXPORT QString xesamPrettyToFullFieldName( const QString &name );
        AMAROK_CORE_EXPORT QString xesamFullToPrettyFieldName( const QString &name );
    }


    AMAROK_CORE_EXPORT QString msToPrettyTime( qint64 ms );

    /** Returns the character representation for the time duration.
        This is a pretty short representation looking like this: 3:45.
        It is used in the playlist.
        It is not useful for times above a 24 hours.
    */
    AMAROK_CORE_EXPORT QString secToPrettyTime( int seconds );

    /** Returns the character representation for the time duration.
        This is a longer human friendly representation looking like this: 5 minutes even when
        the actual seconds are 307.
    */
    AMAROK_CORE_EXPORT QString secToPrettyTimeLong( int seconds );

    AMAROK_CORE_EXPORT QString prettyFilesize( quint64 size );
    AMAROK_CORE_EXPORT QString prettyBitrate( int bitrate );

}

#endif
