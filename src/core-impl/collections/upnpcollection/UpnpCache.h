/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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
 ***************************************************************************************/
#ifndef UPNPCACHE_H
#define UPNPCACHE_H

#include <QMutex>

#include <kio/udsentry.h>

#include "core/meta/forward_declarations.h"
#include "core-impl/collections/support/MemoryCollection.h"


namespace Collections {

class UpnpCollectionBase;

class UpnpCache
{
public:
    explicit UpnpCache( UpnpCollectionBase *collection );

    Meta::TrackPtr getTrack( const KIO::UDSEntry &entry, bool refresh = false );
    Meta::ArtistPtr getArtist( const QString &name );
    Meta::AlbumPtr getAlbum( const QString& name, const QString& artist = QString() );
    Meta::GenrePtr getGenre( const QString &name );
    Meta::YearPtr getYear( int name );

    void removeTrack( Meta::TrackPtr track );

    TrackMap tracks() { return m_trackMap; }
    ArtistMap artists() { return m_artistMap; }
    AlbumMap albums() { return m_albumMap; }
    GenreMap genres() { return m_genreMap; }
    YearMap years() { return m_yearMap; }

private:
    TrackMap m_trackMap;
    ArtistMap m_artistMap;
    AlbumMap m_albumMap;
    GenreMap m_genreMap;
    YearMap m_yearMap;

    QMutex m_cacheMutex;
    UpnpCollectionBase *m_collection;
};

}

#endif // UPNPCACHE_H
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
