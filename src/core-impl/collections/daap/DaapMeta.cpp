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

#include "DaapMeta.h"

#include "DaapCollection.h"

using namespace Meta;

DaapTrack::DaapTrack( Collections::DaapCollection *collection, const QString &host, quint16 port, const QString &dbId, const QString &itemId, const QString &format)
    : Meta::Track()
    , m_collection( collection )
    , m_artist( nullptr )
    , m_album( nullptr )
    , m_genre( nullptr )
    , m_composer( nullptr )
    , m_year( nullptr )
    , m_name()
    , m_type( format )
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_displayUrl()
    , m_playableUrl()
{
    QString url = QStringLiteral( "daap://%1:%2/databases/%3/items/%4.%5" )
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

QUrl
DaapTrack::playableUrl() const
{
    QUrl url( m_playableUrl );
    url.setScheme( QStringLiteral("http") );
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

QString
DaapTrack::notPlayableReason() const
{
    return networkNotPlayableReason();
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
DaapTrack::setYear( int newYear )
{
    Q_UNUSED( newYear )
}

/* 
TODO: This isn't good enough, but for now as daapreader/Reader.cpp indicates
 we can query for the BPM from daap server, but desire is to get BPM of files working
 first! 
*/
qreal
DaapTrack::bpm() const
{
    return -1.0;
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

qint64
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

QString
DaapTrack::type() const
{
    return m_type;
}

bool
DaapTrack::inCollection() const
{
    return true;
}

Collections::Collection*
DaapTrack::collection() const
{
    return m_collection;
}

void
DaapTrack::setAlbum( const DaapAlbumPtr &album )
{
    m_album = album;
}

void
DaapTrack::setArtist( const DaapArtistPtr &artist )
{
    m_artist = artist;
}

void
DaapTrack::setGenre( const DaapGenrePtr &genre )
{
    m_genre = genre;
}

void
DaapTrack::setComposer( const DaapComposerPtr &composer )
{
    m_composer = composer;
}

void
DaapTrack::setYear( const DaapYearPtr &year )
{
    m_year = year;
}

void
DaapTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
DaapTrack::setLength( qint64 length )
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
DaapArtist::addTrack( const DaapTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

DaapAlbum::DaapAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( nullptr )
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

void
DaapAlbum::addTrack( const DaapTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
DaapAlbum::setAlbumArtist( const DaapArtistPtr &artist )
{
    m_albumArtist = artist;
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

TrackList
DaapGenre::tracks()
{
    return m_tracks;
}

void
DaapGenre::addTrack( const DaapTrackPtr &track )
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

TrackList
DaapComposer::tracks()
{
    return m_tracks;
}

void
DaapComposer::addTrack( const DaapTrackPtr &track )
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

TrackList
DaapYear::tracks()
{
    return m_tracks;
}

void
DaapYear::addTrack( const DaapTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}
