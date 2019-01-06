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
#include "UpnpCache.h"

#include <QMutexLocker>

#include "upnptypes.h"

#include "UpnpMeta.h"
#include "UpnpCollectionBase.h"

// TODO : move this to CollectionBase
static qint64 duration( const QString &duration ) {
    if( duration.isEmpty() )
        return 0;

    QStringList parts = duration.split( ':' );
    int hours = parts.takeFirst().toInt();
    int minutes = parts.takeFirst().toInt();
    QString rest = parts.takeFirst();
    int seconds = 0;
    int mseconds = 0;
    if( rest.contains( QLatin1Char('.') ) ) {
        int dotIndex = rest.indexOf( "." );
        seconds = rest.left( dotIndex ).toInt();
        QString frac = rest.mid( dotIndex + 1 );
        if( frac.contains( '/' ) ) {
            int slashIndex = frac.indexOf( '/' );
            int num = frac.left( frac.indexOf( '/' ) ).toInt();
            int den = frac.mid( slashIndex + 1 ).toInt();
            mseconds = num * 1000 / den;
        }
        else {
            mseconds = QString( '.' + frac ).toFloat() * 1000;
        }
    }
    else {
        seconds = rest.toInt();
    }

    return hours * 60 * 60 * 1000
        + minutes * 60 * 1000
        + seconds * 1000
        + mseconds;
}

namespace Collections {

UpnpCache::UpnpCache( UpnpCollectionBase* collection )
    : m_collection( collection )
{
}

Meta::TrackPtr UpnpCache::getTrack( const KIO::UDSEntry &entry, bool refresh )
{
    QMutexLocker lock( &m_cacheMutex );

    // a little indirection to get the nicely formatted track uidUrl
    Meta::UpnpTrackPtr track( new Meta::UpnpTrack( m_collection ) );
    track->setUidUrl( entry.stringValue( KIO::UPNP_ID ) );

    // if we have a reference ID search for that
    // in either case the original ID (refID) becomes our UID URL instead of the UPNP_ID
    if( entry.contains( KIO::UPNP_REF_ID ) ) {
        track->setUidUrl( entry.stringValue( KIO::UPNP_REF_ID ) );
    }

    QString uidUrl = track->uidUrl();
    if( m_trackMap.contains( uidUrl ) && !refresh ) {
        return m_trackMap[uidUrl];
    }

    // UDS_NAME is the plain ASCII, relative path prefixed name
    // but UDS_DISPLAY_NAME is the unicode, 'file' name.
    track->setTitle( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
    track->setPlayableUrl( entry.stringValue(KIO::UDSEntry::UDS_TARGET_URL) );
    track->setTrackNumber( entry.stringValue(KIO::UPNP_TRACK_NUMBER).toInt() );
    // TODO validate and then convert to kbps
    track->setBitrate( entry.stringValue( KIO::UPNP_BITRATE ).toInt() / 1024 );
    track->setLength( duration( entry.stringValue( KIO::UPNP_DURATION ) ) );

    Meta::UpnpArtistPtr artist = Meta::UpnpArtistPtr::staticCast( getArtist( entry.stringValue( KIO::UPNP_ARTIST ) ) );
    artist->addTrack( track );
    track->setArtist( artist );

    Meta::UpnpAlbumPtr album = Meta::UpnpAlbumPtr::staticCast( getAlbum( entry.stringValue( KIO::UPNP_ALBUM ), artist->name() ) );
    album->setAlbumArtist( artist );
    album->addTrack( track );
    track->setAlbum( album );
    // album art
    if( ! album->imageLocation().isValid() )
        album->setAlbumArtUrl( QUrl(entry.stringValue( KIO::UPNP_ALBUMART_URI )) );

    Meta::UpnpGenrePtr genre = Meta::UpnpGenrePtr::staticCast( getGenre( entry.stringValue( KIO::UPNP_GENRE ) ) );
    genre->addTrack( track );
    track->setGenre( genre );


    // TODO this is plain WRONG! the UPNP_DATE will not have year of the album
    // it will have year of addition to the collection
    //QString yearStr = yearForDate( entry.stringValue( KIO::UPNP_DATE ) );
    //
    //Meta::UpnpYearPtr year = Meta::UpnpYearPtr::staticCast( getYear( yearStr ) );
    //year->addTrack( track );
    //track->setYear( year );

    m_trackMap.insert( uidUrl, Meta::TrackPtr::staticCast( track ) );
    return Meta::TrackPtr::staticCast( track );
}

Meta::ArtistPtr UpnpCache::getArtist( const QString& name )
{
    if( m_artistMap.contains( name ) )
        return m_artistMap[name];

    Meta::UpnpArtistPtr artist( new Meta::UpnpArtist( name ) );
    m_artistMap.insert( name, Meta::ArtistPtr::staticCast( artist ) );
    return m_artistMap[name];
}

Meta::AlbumPtr UpnpCache::getAlbum(const QString& name, const QString &artist )
{
    if( m_albumMap.contains( name, artist ) )
        return m_albumMap.value( name, artist );

    Meta::UpnpAlbumPtr album( new Meta::UpnpAlbum( name ) );
    album->setAlbumArtist( Meta::UpnpArtistPtr::staticCast( getArtist( artist ) ) );
    m_albumMap.insert( Meta::AlbumPtr::staticCast( album ) );
    return Meta::AlbumPtr::staticCast( album );
}

Meta::GenrePtr UpnpCache::getGenre(const QString& name)
{
    if( m_genreMap.contains( name ) )
        return m_genreMap[name];

    Meta::UpnpGenrePtr genre( new Meta::UpnpGenre( name ) );
    m_genreMap.insert( name, Meta::GenrePtr::staticCast( genre ) );
    return m_genreMap[name];
}

Meta::YearPtr UpnpCache::getYear(int name)
{
    if( m_yearMap.contains( name ) )
        return m_yearMap[name];

    Meta::UpnpYearPtr year( new Meta::UpnpYear( name ) );
    m_yearMap.insert( name, Meta::YearPtr::staticCast( year ) );
    return m_yearMap[name];
}

void UpnpCache::removeTrack( Meta::TrackPtr t )
{
#define DOWNCAST( Type, item ) Meta::Upnp##Type##Ptr::staticCast( item )
    Meta::UpnpTrackPtr track = DOWNCAST( Track, t );
    DOWNCAST( Artist, m_artistMap[ track->artist()->name() ] )->removeTrack( track );
    DOWNCAST( Album, m_albumMap.value( track->album() ) )->removeTrack( track );
    DOWNCAST( Genre, m_genreMap[ track->genre()->name() ] )->removeTrack( track );
    DOWNCAST( Year, m_yearMap[ track->year()->year() ] )->removeTrack( track );
#undef DOWNCAST
    m_trackMap.remove( track->uidUrl() );
}
}
