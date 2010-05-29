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
 ****************************************************************************************/

#include "UpnpMeta.h"

#include "UpnpCollection.h"
#include "core/support/Debug.h"

using namespace Meta;

UpnpTrack::UpnpTrack( Collections::UpnpCollection *collection )
    : Meta::Track()
    , m_collection( collection )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
{
}

UpnpTrack::~UpnpTrack()
{
    //nothing to do
}

QString
UpnpTrack::name() const
{
    return m_name;
}

QString
UpnpTrack::prettyName() const
{
    return m_name;
}

KUrl
UpnpTrack::playableUrl() const
{
DEBUG_BLOCK
    KUrl url( m_playableUrl );
    url.setProtocol( "http" );
    url.setHost("www.google.com");
    return url;
}

QString
UpnpTrack::uidUrl() const
{
    return m_playableUrl;
}

QString
UpnpTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
UpnpTrack::isPlayable() const
{
    return true;
}

bool
UpnpTrack::isEditable() const
{
    return false;
}

AlbumPtr
UpnpTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
UpnpTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
UpnpTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
UpnpTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
UpnpTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
UpnpTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
UpnpTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
UpnpTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
UpnpTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
UpnpTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

void
UpnpTrack::setUidUrl( const QString &url )
{
    m_playableUrl = url;
}

/* 
TODO: This isn't good enough, but for now as daapreader/Reader.cpp indicates
 we can query for the BPM from daap server, but desire is to get BPM of files working
 first! 
*/
qreal
UpnpTrack::bpm() const
{
    return -1.0;
}

QString
UpnpTrack::comment() const
{
    return QString();
}

void
UpnpTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
UpnpTrack::score() const
{
    return 0.0;
}

void
UpnpTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
UpnpTrack::rating() const
{
    return 0;
}

void
UpnpTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

qint64
UpnpTrack::length() const
{
    return m_length;
}

int
UpnpTrack::filesize() const
{
    return 0;
}

int
UpnpTrack::sampleRate() const
{
    return 0;
}

int
UpnpTrack::bitrate() const
{
    return 0;
}

int
UpnpTrack::trackNumber() const
{
    return m_trackNumber;
}

void
UpnpTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
UpnpTrack::discNumber() const
{
    return 0;
}

void
UpnpTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
UpnpTrack::playCount() const
{
    return 0;
}

uint
UpnpTrack::lastPlayed() const
{
    return 0;
}

QString
UpnpTrack::type() const
{
    return m_type;
}

void
UpnpTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
UpnpTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

bool
UpnpTrack::inCollection() const
{
    return true;
}

Collections::Collection*
UpnpTrack::collection() const
{
    return m_collection;
}

void
UpnpTrack::setAlbum( UpnpAlbumPtr album )
{
    m_album = album;
}

void
UpnpTrack::setArtist( UpnpArtistPtr artist )
{
    m_artist = artist;
}

void
UpnpTrack::setGenre( UpnpGenrePtr genre )
{
    m_genre = genre;
}

void
UpnpTrack::setComposer( UpnpComposerPtr composer )
{
    m_composer = composer;
}

void
UpnpTrack::setYear( UpnpYearPtr year )
{
    m_year = year;
}

void
UpnpTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
UpnpTrack::setLength( qint64 length )
{
    m_length = length;
}

//UpnpArtist

UpnpArtist::UpnpArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpArtist::~UpnpArtist()
{
    //nothing to do
}

QString
UpnpArtist::name() const
{
    return m_name;
}

QString
UpnpArtist::prettyName() const
{
    return m_name;
}

TrackList
UpnpArtist::tracks()
{
DEBUG_BLOCK
    return m_tracks;
}

AlbumList
UpnpArtist::albums()
{
    //TODO
    return m_albums;
}

void
UpnpArtist::addTrack( UpnpTrackPtr track )
{
DEBUG_BLOCK
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpArtist::addAlbum( UpnpAlbumPtr album )
{
DEBUG_BLOCK
    m_albums.append( AlbumPtr::staticCast( album ) );
}

UpnpAlbum::UpnpAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

UpnpAlbum::~UpnpAlbum()
{
    //nothing to do
}

QString
UpnpAlbum::name() const
{
    return m_name;
}

QString
UpnpAlbum::prettyName() const
{
    return m_name;
}

bool
UpnpAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
UpnpAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
UpnpAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
UpnpAlbum::tracks()
{
    return m_tracks;
}

QPixmap
UpnpAlbum::image( int size )
{
    return Meta::Album::image( size );
}

bool
UpnpAlbum::canUpdateImage() const
{
    return false;
}

void
UpnpAlbum::setImage( const QPixmap &pixmap )
{
    Q_UNUSED(pixmap);
    //TODO
}

void
UpnpAlbum::addTrack( UpnpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpAlbum::setAlbumArtist( UpnpArtistPtr artist )
{
    m_albumArtist = artist;
}

void
UpnpAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

//UpnpGenre

UpnpGenre::UpnpGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpGenre::~UpnpGenre()
{
    //nothing to do
}

QString
UpnpGenre::name() const
{
    return m_name;
}

QString
UpnpGenre::prettyName() const
{
    return m_name;
}

TrackList
UpnpGenre::tracks()
{
    return m_tracks;
}

void
UpnpGenre::addTrack( UpnpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//UpnpComposer

UpnpComposer::UpnpComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpComposer::~UpnpComposer()
{
    //nothing to do
}

QString
UpnpComposer::name() const
{
    return m_name;
}

QString
UpnpComposer::prettyName() const
{
    return m_name;
}

TrackList
UpnpComposer::tracks()
{
    return m_tracks;
}

void
UpnpComposer::addTrack( UpnpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//UpnpYear

UpnpYear::UpnpYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpYear::~UpnpYear()
{
    //nothing to do
}

QString
UpnpYear::name() const
{
    return m_name;
}

QString
UpnpYear::prettyName() const
{
    return m_name;
}

TrackList
UpnpYear::tracks()
{
    return m_tracks;
}

void
UpnpYear::addTrack( UpnpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}
