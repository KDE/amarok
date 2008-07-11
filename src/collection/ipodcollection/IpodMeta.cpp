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

#include "IpodMeta.h"

#include "IpodCollection.h"

using namespace Meta;
IpodTrack::IpodTrack( IpodCollection *collection, const QString &format)
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
    m_displayUrl = QString();
    m_playableUrl = QString();
}

IpodTrack::~IpodTrack()
{
    //nothing to do
}

QString
IpodTrack::name() const
{
    return m_name;
}

QString
IpodTrack::prettyName() const
{
    return m_name;
}

KUrl
IpodTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
IpodTrack::url() const
{
    return m_playableUrl;
}

QString
IpodTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
IpodTrack::isPlayable() const
{
    return true;
}

bool
IpodTrack::isEditable() const
{
    return false;
}

Itdb_Track*
IpodTrack::getIpodTrack()
{
    return m_ipodtrack;
}

void
IpodTrack::setIpodTrack ( Itdb_Track *ipodtrack )
{
    m_ipodtrack = ipodtrack;
}

AlbumPtr
IpodTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
IpodTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
IpodTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
IpodTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
IpodTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
IpodTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
IpodTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
IpodTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
IpodTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
IpodTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString
IpodTrack::comment() const
{
    return QString();
}

void
IpodTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
}

double
IpodTrack::score() const
{
    return 0.0;
}

void
IpodTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
IpodTrack::rating() const
{
    return 0;
}

void
IpodTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
IpodTrack::length() const
{
    return m_length;
}

int
IpodTrack::filesize() const
{
    return 0;
}

int
IpodTrack::sampleRate() const
{
    return 0;
}

int
IpodTrack::bitrate() const
{
    return m_bitrate;
}

void
IpodTrack::setBitrate( int newBitrate )
{
    m_bitrate = newBitrate;
}

int
IpodTrack::samplerate() const
{
    return m_samplerate;
}

void
IpodTrack::setSamplerate( int newSamplerate )
{
    m_samplerate = newSamplerate;
}

float
IpodTrack::bpm() const
{
    return m_bpm;
}
void
IpodTrack::setBpm( float newBpm )
{
    m_bpm = newBpm;
}

int
IpodTrack::trackNumber() const
{
    return m_trackNumber;
}

void
IpodTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
IpodTrack::discNumber() const
{
    return m_discNumber;
}

void
IpodTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
}

int
IpodTrack::playCount() const
{
    return 0;
}

uint
IpodTrack::lastPlayed() const
{
    return 0;
}

QString
IpodTrack::type() const
{
    return m_type;
}

void
IpodTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
IpodTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}
// TODO: implement this for IpodCollectionLocation
bool
IpodTrack::inCollection() const
{
    return true;
}

Collection*
IpodTrack::collection() const
{
    return m_collection;
}

void
IpodTrack::setAlbum( IpodAlbumPtr album )
{
    m_album = album;
}

void
IpodTrack::setArtist( IpodArtistPtr artist )
{
    m_artist = artist;
}

void
IpodTrack::setGenre( IpodGenrePtr genre )
{
    m_genre = genre;
}

void
IpodTrack::setComposer( IpodComposerPtr composer )
{
    m_composer = composer;
}

void
IpodTrack::setYear( IpodYearPtr year )
{
    m_year = year;
}

QString
IpodTrack::title() const
{
    return m_name;
}

void
IpodTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
IpodTrack::setLength( int length )
{
    m_length = length;
}

//IpodArtist

IpodArtist::IpodArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodArtist::~IpodArtist()
{
    //nothing to do
}

QString
IpodArtist::name() const
{
    return m_name;
}

QString
IpodArtist::prettyName() const
{
    return m_name;
}

TrackList
IpodArtist::tracks()
{
    return m_tracks;
}

AlbumList
IpodArtist::albums()
{
    //TODO
    return AlbumList();
}

void
IpodArtist::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

IpodAlbum::IpodAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

IpodAlbum::~IpodAlbum()
{
    //nothing to do
}

QString
IpodAlbum::name() const
{
    return m_name;
}

QString
IpodAlbum::prettyName() const
{
    return m_name;
}

bool
IpodAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
IpodAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
IpodAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
IpodAlbum::tracks()
{
    return m_tracks;
}

QPixmap
IpodAlbum::image( int size, bool withShadow )
{
    return Meta::Album::image( size, withShadow );
}

bool
IpodAlbum::canUpdateImage() const
{
    return false;
}

void
IpodAlbum::setImage( const QImage &image )
{
    Q_UNUSED(image);
    //TODO
}

void
IpodAlbum::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodAlbum::setAlbumArtist( IpodArtistPtr artist )
{
    m_albumArtist = artist;
}

void
IpodAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

//IpodGenre

IpodGenre::IpodGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodGenre::~IpodGenre()
{
    //nothing to do
}

QString
IpodGenre::name() const
{
    return m_name;
}

QString
IpodGenre::prettyName() const
{
    return m_name;
}

TrackList
IpodGenre::tracks()
{
    return m_tracks;
}

void
IpodGenre::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//IpodComposer

IpodComposer::IpodComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodComposer::~IpodComposer()
{
    //nothing to do
}

QString
IpodComposer::name() const
{
    return m_name;
}

QString
IpodComposer::prettyName() const
{
    return m_name;
}

TrackList
IpodComposer::tracks()
{
    return m_tracks;
}

void
IpodComposer::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//IpodYear

IpodYear::IpodYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodYear::~IpodYear()
{
    //nothing to do
}

QString
IpodYear::name() const
{
    return m_name;
}

QString
IpodYear::prettyName() const
{
    return m_name;
}

TrackList
IpodYear::tracks()
{
    return m_tracks;
}

void
IpodYear::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}
