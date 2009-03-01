/* This file is part of the KDE project

   Note: Mostly taken from Daap code:
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

#include "MediaDeviceMeta.h"

#include "MediaDeviceCollection.h"

using namespace Meta;
MediaDeviceTrack::MediaDeviceTrack( MediaDeviceCollection *collection, const QString &format)
    : Meta::Track()
    , m_collection( collection )
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name()
    , m_type( format )
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_displayUrl()
    , m_playableUrl()
{
  //QString url = QString( "ipod://%1:%2/%3/%4.%5" )
  //                .arg( host, QString::number( port ), dbId, itemId, format );
}

MediaDeviceTrack::~MediaDeviceTrack()
{
    //nothing to do
}

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
MediaDeviceTrack::uidUrl() const
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
MediaDeviceTrack::type() const
{
    return m_type;
}

void
MediaDeviceTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
MediaDeviceTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

bool
MediaDeviceTrack::inCollection() const
{
    return true;
}

Amarok::Collection*
MediaDeviceTrack::collection() const
{
    return m_collection;
}

void
MediaDeviceTrack::setAlbum( MediaDeviceAlbumPtr album )
{
    m_album = album;
}

void
MediaDeviceTrack::setArtist( MediaDeviceArtistPtr artist )
{
    m_artist = artist;
}

void
MediaDeviceTrack::setGenre( MediaDeviceGenrePtr genre )
{
    m_genre = genre;
}

void
MediaDeviceTrack::setComposer( MediaDeviceComposerPtr composer )
{
    m_composer = composer;
}

void
MediaDeviceTrack::setYear( MediaDeviceYearPtr year )
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

//MediaDeviceArtist

MediaDeviceArtist::MediaDeviceArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
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

TrackList
MediaDeviceArtist::tracks()
{
    return m_tracks;
}

AlbumList
MediaDeviceArtist::albums()
{
    //TODO
    return AlbumList();
}

void
MediaDeviceArtist::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

MediaDeviceAlbum::MediaDeviceAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

MediaDeviceAlbum::~MediaDeviceAlbum()
{
    //nothing to do
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

QPixmap
MediaDeviceAlbum::image( int size )
{
    return Meta::Album::image( size );
}

bool
MediaDeviceAlbum::canUpdateImage() const
{
    return false;
}

void
MediaDeviceAlbum::setImage( const QPixmap &pixmap )
{
    Q_UNUSED(pixmap);
    //TODO
}

void
MediaDeviceAlbum::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceAlbum::setAlbumArtist( MediaDeviceArtistPtr artist )
{
    m_albumArtist = artist;
}

void
MediaDeviceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
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

TrackList
MediaDeviceGenre::tracks()
{
    return m_tracks;
}

void
MediaDeviceGenre::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//MediaDeviceComposer

MediaDeviceComposer::MediaDeviceComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceComposer::~MediaDeviceComposer()
{
    //nothing to do
}

QString
MediaDeviceComposer::name() const
{
    return m_name;
}

QString
MediaDeviceComposer::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceComposer::tracks()
{
    return m_tracks;
}

void
MediaDeviceComposer::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//MediaDeviceYear

MediaDeviceYear::MediaDeviceYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceYear::~MediaDeviceYear()
{
    //nothing to do
}

QString
MediaDeviceYear::name() const
{
    return m_name;
}

QString
MediaDeviceYear::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceYear::tracks()
{
    return m_tracks;
}

void
MediaDeviceYear::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}
