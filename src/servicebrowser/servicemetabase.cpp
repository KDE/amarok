/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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

#include "servicemetabase.h"

ServiceTrack::ServiceTrack( const QString & name )
    : Meta::Track()
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name( name )
    , m_type()
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_displayUrl( 0 )
    , m_playableUrl( 0 )
{
}

ServiceTrack::~ServiceTrack()
{
    //nothing to do
}

void ServiceTrack::setId(int id)
{
    m_id = id;
}

int ServiceTrack::id()
{
    return m_id;
}

void ServiceTrack::setAlbumId(int albumId)
{
    m_albumId = albumId;
}

int ServiceTrack::albumId()
{
    return m_albumId;
}


QString
ServiceTrack::name() const
{
    return m_name;
}

QString
ServiceTrack::prettyName() const
{
    return m_name;
}

KUrl
ServiceTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
ServiceTrack::url() const
{
    return m_playableUrl;
}

QString
ServiceTrack::prettyUrl() const
{
    return m_displayUrl;
}

void ServiceTrack::setUrl(const QString & url)
{
    m_playableUrl = url;
    m_displayUrl = url;
}

bool
ServiceTrack::isPlayable() const
{
    return true;
}

bool
ServiceTrack::isEditable() const
{
    return false;
}

AlbumPtr
ServiceTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
ServiceTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
ServiceTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
ServiceTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
ServiceTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
ServiceTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
ServiceTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
ServiceTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
ServiceTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
ServiceTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString
ServiceTrack::comment() const
{
    return QString();
}

void
ServiceTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
ServiceTrack::score() const
{
    return 0.0;
}

void
ServiceTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
ServiceTrack::rating() const
{
    return 0;
}

void
ServiceTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
ServiceTrack::length() const
{
    return m_length;
}

int
ServiceTrack::filesize() const
{
    return 0;
}

int
ServiceTrack::sampleRate() const
{
    return 0;
}

int
ServiceTrack::bitrate() const
{
    return 0;
}

int
ServiceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
ServiceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
ServiceTrack::discNumber() const
{
    return 0;
}

void
ServiceTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
ServiceTrack::playCount() const
{
    return 0;
}

uint
ServiceTrack::lastPlayed() const
{
    return 0;
}

QString
ServiceTrack::type() const
{
    return m_type;
}

void
ServiceTrack::subscribe( TrackObserver *observer )
{
    Q_UNUSED( observer )
}

void
ServiceTrack::unsubscribe( TrackObserver *observer )
{
    Q_UNUSED( observer )
}

void
ServiceTrack::setAlbum( ServiceAlbumPtr album )
{
    m_album = album;
}

void
ServiceTrack::setArtist( ServiceArtistPtr artist )
{
    m_artist = artist;
}

void
ServiceTrack::setGenre( ServiceGenrePtr genre )
{
    m_genre = genre;
}

void
ServiceTrack::setComposer( ServiceComposerPtr composer )
{
    m_composer = composer;
}

void
ServiceTrack::setYear( ServiceYearPtr year )
{
    m_year = year;
}

void
ServiceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
ServiceTrack::setLength( int length )
{
    m_length = length;
}

//ServiceArtist

ServiceArtist::ServiceArtist( const QString & name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceArtist::~ServiceArtist()
{
    //nothing to do
}

void ServiceArtist::setId(int id)
{
    m_id = id;
}

int ServiceArtist::id()
{
    return m_id;
}



QString
ServiceArtist::name() const
{
    return m_name;
}

QString
ServiceArtist::prettyName() const
{
    return m_name;
}

void ServiceArtist::setTitle(const QString & title)
{
    m_name = title;
}

TrackList
ServiceArtist::tracks()
{
    return m_tracks;
}

void
ServiceArtist::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}


void ServiceArtist::setDescription(const QString &description)
{
    m_description = description;
}

QString ServiceArtist::description()
{
    return m_description;
}


ServiceAlbum::ServiceAlbum( const QString & name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

ServiceAlbum::~ServiceAlbum()
{
    //nothing to do
}

void ServiceAlbum::setId(int id)
{
    m_id = id;
}

int ServiceAlbum::id()
{
    return m_id;
}

void ServiceAlbum::setArtistId(int artistId)
{
    m_artistId = artistId;
}

int ServiceAlbum::artistId()
{
    return m_artistId;
}

QString
ServiceAlbum::name() const
{
    return m_name;
}

QString
ServiceAlbum::prettyName() const
{
    return m_name;
}

void ServiceAlbum::setTitle(const QString & title)
{
    m_name = title;
}

bool
ServiceAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
ServiceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
ServiceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
ServiceAlbum::tracks()
{
    return m_tracks;
}

void
ServiceAlbum::image() const
{
    //TODO
}

bool
ServiceAlbum::canUpdateImage() const
{
    return false;
}

void
ServiceAlbum::updateImage()
{
    //TODO
}

void
ServiceAlbum::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
ServiceAlbum::setAlbumArtist( ServiceArtistPtr artist )
{
    m_albumArtist = artist;
}

void
ServiceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

void ServiceAlbum::setDescription(const QString &description)
{
    m_description = description;
}

QString ServiceAlbum::description()
{
    return m_description;
}



//ServiceGenre

ServiceGenre::ServiceGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceGenre::~ServiceGenre()
{
    //nothing to do
}

QString
ServiceGenre::name() const
{
    return m_name;
}

QString
ServiceGenre::prettyName() const
{
    return m_name;
}

TrackList
ServiceGenre::tracks()
{
    return m_tracks;
}

void
ServiceGenre::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//ServiceComposer

ServiceComposer::ServiceComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceComposer::~ServiceComposer()
{
    //nothing to do
}

QString
ServiceComposer::name() const
{
    return m_name;
}

QString
ServiceComposer::prettyName() const
{
    return m_name;
}

TrackList
ServiceComposer::tracks()
{
    return m_tracks;
}

void
ServiceComposer::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//ServiceYear

ServiceYear::ServiceYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceYear::~ServiceYear()
{
    //nothing to do
}

QString
ServiceYear::name() const
{
    return m_name;
}

QString
ServiceYear::prettyName() const
{
    return m_name;
}

TrackList
ServiceYear::tracks()
{
    return m_tracks;
}

void
ServiceYear::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}











