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

#ifndef AMAROK_METAKEYS_H
#define AMAROK_METAKEYS_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"

#include <QHash>
#include <QString>

class TestMetaAlbumKey;
class TestMetaTrackKey;

namespace Meta
{

    /** The AlbumKey can be used when an album needs to be identified independent of a collection */
    class AMAROKCORE_EXPORT AlbumKey
    {
        public:
            AlbumKey();
            explicit AlbumKey( const QString &name, const QString &artistName );
            explicit AlbumKey( const AlbumPtr &album );

            bool operator<( const AlbumKey &other ) const;

            QString albumName() const
            { return m_albumName; }

            QString artistName() const
            { return m_artistName; }

        private:
            QString m_albumName;
            QString m_artistName;

            friend bool operator==( const AlbumKey &k1, const AlbumKey &k2 );
            friend uint qHash( const AlbumKey &key );

            // Required for unit testing
            friend class ::TestMetaAlbumKey;
    };

    /** The TrackKey can be used when an album needs to be identified independent of a collection */
    class AMAROKCORE_EXPORT TrackKey
    {
        public:
            TrackKey();
            explicit TrackKey( const TrackPtr &track );

        private:
            QString m_trackName;
            int m_discNumber;
            int m_trackNumber;
            QString m_albumName;
            QString m_artistName;

            friend bool operator==( const TrackKey &k1, const TrackKey &k2 );
            friend uint qHash( const TrackKey &key );

            // Required for unit testing
            friend class ::TestMetaTrackKey;
    };


    inline bool
    operator==( const TrackKey &k1, const TrackKey &k2 )
    {
        return k1.m_trackName == k2.m_trackName &&
            k1.m_discNumber == k2.m_discNumber &&
            k1.m_trackNumber == k2.m_trackNumber &&
            k1.m_albumName == k2.m_albumName &&
            k1.m_artistName == k2.m_artistName;
    }

    inline uint
    qHash( const TrackKey &key )
    {
        return qHash( key.m_trackName ) + 17 * qHash( key.m_albumName ) +
            31 * qHash( key.m_artistName ) + 13 * key.m_discNumber + 11 * key.m_trackNumber;
    }

    inline bool
    operator==( const AlbumKey &k1, const AlbumKey &k2 )
    {
        return k1.m_albumName == k2.m_albumName && k1.m_artistName == k2.m_artistName;
    }

    inline uint
    qHash( const AlbumKey &key )
    {
        return qHash( key.m_albumName ) + 17 * qHash( key.m_artistName );
    }

}

#endif
