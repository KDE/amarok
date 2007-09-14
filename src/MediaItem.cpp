/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <ruiz@kde.org>  
    (c) 2006 T.R.Shashwath <trshash84@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "amarok.h"
#include "MediaItem.h"

using namespace Meta;

MediaDeviceTrack::MediaDeviceTrack(const QString & name)
    : Meta::Track()
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name( name )
    , m_trackNumber( 0 )
    , m_length( 0 )
    , m_displayUrl( 0 )
    , m_playableUrl( 0 )
    , m_albumName( 0 )
    , m_artistName( 0 )
    , m_type( 0 )
{
}

MediaDeviceTrack::~MediaDeviceTrack()
{
    //nothing to do
}
e
QString
MediaDeviceTrack::name() const
{
    return m_name;
}

QString
MediaDeviceTrack::prettyName() const
{
    return m_name;
}

KUrl
MediaDeviceTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
MediaDeviceTrack::url() const
{
    return m_playableUrl;
}

QString
MediaDeviceTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
MediaDeviceTrack::isPlayable() const
{
    return true;
}

bool
MediaDeviceTrack::isEditable() const
{
    return false;
}

AlbumPtr
MediaDeviceTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
MediaDeviceTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
MediaDeviceTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
MediaDeviceTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
MediaDeviceTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
MediaDeviceTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
MediaDeviceTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
MediaDeviceTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
MediaDeviceTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
MediaDeviceTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString
MediaDeviceTrack::comment() const
{
    return QString();
}

void
MediaDeviceTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
MediaDeviceTrack::score() const
{
    return 0.0;
}

void
MediaDeviceTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
MediaDeviceTrack::rating() const
{
    return 0;
}

void
MediaDeviceTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
MediaDeviceTrack::length() const
{
    return m_length;
}

int
MediaDeviceTrack::filesize() const
{
    return 0;
}

int
MediaDeviceTrack::sampleRate() const
{
    return 0;
}

int
MediaDeviceTrack::bitrate() const
{
    return 0;
}

int
MediaDeviceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
MediaDeviceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
MediaDeviceTrack::discNumber() const
{
    return 0;
}

void
MediaDeviceTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
MediaDeviceTrack::playCount() const
{
    return 0;
}

uint
MediaDeviceTrack::lastPlayed() const
{
    return 0;
}

QString
MediaDeviceTrack::filename() const
{
    return m_filename;
}

QString
MediaDeviceTrack::type() const
{
    return m_type;
}

void
MediaDeviceTrack::setAlbum( AlbumPtr album )
{
    m_album = album;
}

void
MediaDeviceTrack::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
MediaDeviceTrack::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
MediaDeviceTrack::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
MediaDeviceTrack::setYear( YearPtr year )
{
    m_year = year;
}

void
MediaDeviceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
MediaDeviceTrack::setLength( int length )
{
    m_length = length;
}

void
MediaDeviceTrack::setFilename( const QString &filename )
{
    m_filename = filename;
}


//MediaDeviceArtist



MediaDeviceArtist::MediaDeviceArtist( const QString & name )
    : Meta::Artist()
    , m_name( name )
    , m_description( 0 )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceArtist::~MediaDeviceArtist()
{
    //nothing to do
}

QString
MediaDeviceArtist::name() const
{
    return m_name;
}

QString
MediaDeviceArtist::prettyName() const
{
    return m_name;
}

void MediaDeviceArtist::setTitle(const QString & title)
{
    m_name = title;
}

TrackList
MediaDeviceArtist::tracks()
{
    return m_tracks;
}

void
MediaDeviceArtist::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

MediaDeviceAlbum::MediaDeviceAlbum( const QString & name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
    , m_artistName( 0 )
{
    //nothing to do
}

MediaDeviceAlbum::~MediaDeviceAlbum()
{
    //nothing to do
}

void MediaDeviceAlbum::setArtistName(const QString & name)
{
    m_artistName = name;
}

QString MediaDeviceAlbum::artistName() const
{
    return m_artistName;
}

QString
MediaDeviceAlbum::name() const
{
    return m_name;
}

QString
MediaDeviceAlbum::prettyName() const
{
    return m_name;
}

void MediaDeviceAlbum::setTitle(const QString & title)
{
    m_name = title;
}

bool
MediaDeviceAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
MediaDeviceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
MediaDeviceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
MediaDeviceAlbum::tracks()
{
    return m_tracks;
}

void
MediaDeviceAlbum::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
MediaDeviceAlbum::setAlbumArtist( ArtistPtr artist )
{
    m_albumArtist = artist;
}

void
MediaDeviceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

void MediaDeviceAlbum::setDescription(const QString &description)
{
    m_description = description;
}

QString MediaDeviceAlbum::description() const
{
    return m_description;
}



//MediaDeviceGenre

MediaDeviceGenre::MediaDeviceGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceGenre::~MediaDeviceGenre()
{
    //nothing to do
}

QString
MediaDeviceGenre::name() const
{
    return m_name;
}

QString
MediaDeviceGenre::prettyName() const
{
    return m_name;
}

void MediaDeviceGenre::setName(const QString & name)
{
    m_name = name;
}


int MediaDeviceGenre::albumId()
{
    return m_albumId;
}

void MediaDeviceGenre::setAlbumId(int albumId)
{
    m_albumId = albumId;
}


TrackList
MediaDeviceGenre::tracks()
{
    return m_tracks;
}

void
MediaDeviceGenre::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}


