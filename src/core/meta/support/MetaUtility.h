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

#include "shared/amarok_export.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"

#include <QMap>
#include <QString>
#include <QVariant>

class AMAROK_EXPORT AlbumKey
{
public:
    QString albumName;
    QString artistName;

    AlbumKey() {}
    AlbumKey( const QString &artist, const QString &album )
    { artistName = artist; albumName = album; }

    AlbumKey &operator=( const AlbumKey &o )
    { albumName = o.albumName; artistName = o.artistName; return *this; }
};

class AMAROK_EXPORT TrackKey
{
public:
    QString trackName;
    QString albumName;
    QString artistName;
    //more?

    TrackKey() {}

    TrackKey &operator=( const TrackKey &o )
    { trackName = o.trackName; albumName = o.albumName; artistName = o.artistName; return *this; }
};

namespace Meta
{
    class Track;

    namespace Field
    {

        //deprecated
        AMAROK_EXPORT QVariantMap mapFromTrack( const Meta::TrackPtr track );
        //this method will return a map with keys that are compatible to the fdo MPRIS specification
        AMAROK_EXPORT QVariantMap mprisMapFromTrack( const Meta::TrackPtr track );
        AMAROK_EXPORT void updateTrack( Meta::TrackPtr track, const QVariantMap &metadata );
        AMAROK_EXPORT QString xesamPrettyToFullFieldName( const QString &name );
        AMAROK_EXPORT QString xesamFullToPrettyFieldName( const QString &name );
    }


    AMAROK_EXPORT QString msToPrettyTime( qint64 ms );
    AMAROK_EXPORT QString secToPrettyTime( int seconds );

    AMAROK_EXPORT QString prettyFilesize( quint64 size );
    AMAROK_EXPORT QString prettyBitrate( int bitrate );

    AMAROK_EXPORT QString prettyRating( int rating );

    AMAROK_EXPORT TrackKey keyFromTrack( const Meta::TrackPtr &track );
}

inline bool
operator==( const TrackKey &k1, const TrackKey &k2 )
{
    return k1.trackName == k2.trackName &&
                          k1.albumName == k2.albumName &&
                          k1.artistName == k2.artistName;
}

inline uint
qHash( const TrackKey &key )
{
    return qHash( key.trackName ) + 17 * qHash( key.albumName ) + 31 * qHash( key.artistName );
}

inline bool
operator==( const AlbumKey &k1, const AlbumKey &k2 )
{
    return k1.albumName == k2.albumName && k1.artistName == k2.artistName;
}

inline uint
qHash( const AlbumKey &key )
{
    return qHash( key.albumName ) + 17 * qHash( key.artistName );
}

#endif
