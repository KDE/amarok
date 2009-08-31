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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DaapMeta.h"

#include "DaapCollection.h"

using namespace Meta;

DaapTrack::DaapTrack( DaapCollection *collection, const QString &host, quint16 port, const QString &dbId, const QString &itemId, const QString &format)
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
    QString url = QString( "daap://%1:%2/databases/%3/items/%4.%5" )
                    .arg( host, QString::number( port ), dbId, itemId, format );
    m_displayUrl = url;
    m_playableUrl = url;
}

DaapTrack::~DaapTrack()
{
    //nothing to do
}

QString
DaapTrack::name() const
{
    return m_name;
}

QString
DaapTrack::prettyName() const
{
    return m_name;
}

KUrl
DaapTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    url.setProtocol( "http" );
    return url;
}

QString
DaapTrack::uidUrl() const
{
    return m_playableUrl;
}

QString
DaapTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
DaapTrack::isPlayable() const
{
    return true;
}

bool
DaapTrack::isEditable() const
{
    return false;
}

AlbumPtr
DaapTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
DaapTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
DaapTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
DaapTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
DaapTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
DaapTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
DaapTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
DaapTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
DaapTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
DaapTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString
DaapTrack::comment() const
{
    return QString();
}

void
DaapTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
DaapTrack::score() const
{
    return 0.0;
}

void
DaapTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
DaapTrack::rating() const
{
    return 0;
}

void
DaapTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
DaapTrack::length() const
{
    return m_length;
}

int
DaapTrack::filesize() const
{
    return 0;
}

int
DaapTrack::sampleRate() const
{
    return 0;
}

int
DaapTrack::bitrate() const
{
    return 0;
}

int
DaapTrack::trackNumber() const
{
    return m_trackNumber;
}

void
DaapTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
DaapTrack::discNumber() const
{
    return 0;
}

void
DaapTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
DaapTrack::playCount() const
{
    return 0;
}

uint
DaapTrack::lastPlayed() const
{
    return 0;
}

QString
DaapTrack::type() const
{
    return m_type;
}

void
DaapTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
DaapTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

bool
DaapTrack::inCollection() const
{
    return true;
}

Amarok::Collection*
DaapTrack::collection() const
{
    return m_collection;
}

void
DaapTrack::setAlbum( DaapAlbumPtr album )
{
    m_album = album;
}

void
DaapTrack::setArtist( DaapArtistPtr artist )
{
    m_artist = artist;
}

void
DaapTrack::setGenre( DaapGenrePtr genre )
{
    m_genre = genre;
}

void
DaapTrack::setComposer( DaapComposerPtr composer )
{
    m_composer = composer;
}

void
DaapTrack::setYear( DaapYearPtr year )
{
    m_year = year;
}

void
DaapTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
DaapTrack::setLength( int length )
{
    m_length = length;
}

//DaapArtist

DaapArtist::DaapArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

DaapArtist::~DaapArtist()
{
    //nothing to do
}

QString
DaapArtist::name() const
{
    return m_name;
}

QString
DaapArtist::prettyName() const
{
    return m_name;
}

TrackList
DaapArtist::tracks()
{
    return m_tracks;
}

AlbumList
DaapArtist::albums()
{
    //TODO
    return AlbumList();
}

void
DaapArtist::addTrack( DaapTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

DaapAlbum::DaapAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

DaapAlbum::~DaapAlbum()
{
    //nothing to do
}

QString
DaapAlbum::name() const
{
    return m_name;
}

QString
DaapAlbum::prettyName() const
{
    return m_name;
}

bool
DaapAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
DaapAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
DaapAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
DaapAlbum::tracks()
{
    return m_tracks;
}

QPixmap
DaapAlbum::image( int size )
{
    return Meta::Album::image( size );
}

bool
DaapAlbum::canUpdateImage() const
{
    return false;
}

void
DaapAlbum::setImage( const QPixmap &pixmap )
{
    Q_UNUSED(pixmap);
    //TODO
}

void
DaapAlbum::addTrack( DaapTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
DaapAlbum::setAlbumArtist( DaapArtistPtr artist )
{
    m_albumArtist = artist;
}

void
DaapAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

//DaapGenre

DaapGenre::DaapGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

DaapGenre::~DaapGenre()
{
    //nothing to do
}

QString
DaapGenre::name() const
{
    return m_name;
}

QString
DaapGenre::prettyName() const
{
    return m_name;
}

TrackList
DaapGenre::tracks()
{
    return m_tracks;
}

void
DaapGenre::addTrack( DaapTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//DaapComposer

DaapComposer::DaapComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

DaapComposer::~DaapComposer()
{
    //nothing to do
}

QString
DaapComposer::name() const
{
    return m_name;
}

QString
DaapComposer::prettyName() const
{
    return m_name;
}

TrackList
DaapComposer::tracks()
{
    return m_tracks;
}

void
DaapComposer::addTrack( DaapTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//DaapYear

DaapYear::DaapYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

DaapYear::~DaapYear()
{
    //nothing to do
}

QString
DaapYear::name() const
{
    return m_name;
}

QString
DaapYear::prettyName() const
{
    return m_name;
}

TrackList
DaapYear::tracks()
{
    return m_tracks;
}

void
DaapYear::addTrack( DaapTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}
