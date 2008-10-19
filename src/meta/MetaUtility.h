/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_METAUTILITY_H
#define AMAROK_METAUTILITY_H

#include "amarok_export.h"
#include "Meta.h"

#include <QMap>
#include <QString>
#include <QVariant>

// Taglib
#include <taglib/fileref.h>

namespace Meta
{
    class Track;

    namespace Field
    {
        //actual string values are not final yet
        static const QString ALBUM          = "xesam:album";
        static const QString ARTIST         = "xesam:author";
        static const QString BITRATE        = "xesam:audioBitrate";
        static const QString BPM            = "xesam:audioBPM";
        static const QString CODEC          = "xesam:audioCodec";
        static const QString COMMENT        = "xesam:comment";
        static const QString COMPOSER       = "xesam:composer";
        static const QString DISCNUMBER     = "xesam:discNumber";
        static const QString FILESIZE       = "xesam:size";
        static const QString GENRE          = "xesam:genre";
        static const QString LENGTH         = "xesam:mediaDuration";
        static const QString RATING         = "xesam:userRating";
        static const QString SAMPLERATE     = "xesam:audioSampleRate";
        static const QString TITLE          = "xesam:title";
        static const QString TRACKNUMBER    = "xesam:trackNumber";
        static const QString URL            = "xesam:url";
        static const QString YEAR           = "xesam:contentCreated";

        static const QString SCORE          = "xesam:autoRating";
        static const QString PLAYCOUNT      = "xesam:useCount";
        static const QString FIRST_PLAYED   = "xesam:firstUsed";
        static const QString LAST_PLAYED    = "xesam:lastUsed";

        static const QString UNIQUEID       = "xesam:id";


        AMAROK_EXPORT QVariantMap mapFromTrack( const Meta::TrackPtr track );
        AMAROK_EXPORT void updateTrack( Meta::TrackPtr track, const QVariantMap &metadata );
        AMAROK_EXPORT void writeFields( const QString &filename, const QVariantMap &changes );
        AMAROK_EXPORT void writeFields( TagLib::FileRef fileref, const QVariantMap &changes );
        AMAROK_EXPORT QString xesamPrettyToFullFieldName( const QString &name );
        AMAROK_EXPORT QString xesamFullToPrettyFieldName( const QString &name );
    }


    AMAROK_EXPORT QString msToPrettyTime( int ms );
    AMAROK_EXPORT QString secToPrettyTime( int seconds );

    AMAROK_EXPORT QString prettyFilesize( int size );
    AMAROK_EXPORT QString prettyBitrate( int bitrate );

    AMAROK_EXPORT QString prettyRating( int rating );
}

#endif
