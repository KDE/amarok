/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TimecodeMeta.h"
#include "meta/Capability.h"
#include "meta/capabilities/BoundedPlaybackCapability.h"

using namespace Meta;


////////////////// BoundedPlaybackCapabilityImpl //////////////////

qint64 BoundedPlaybackCapabilityImpl::startPosition()
{
    return m_track->start();
}

qint64 BoundedPlaybackCapabilityImpl::endPosition()
{
    return m_track->end();
}

////////////////// TRACK //////////////////

TimecodeTrack::TimecodeTrack( const QString & name, const QString & url, qint64 start, qint64 end )
    : m_name( name )
    , m_start( start )
    , m_end( end )
    , m_length( end - start )
    , m_playableUrl( url )
{
    m_displayUrl = url + ":" + QString::number( start ) + "-" + QString::number( end );
}

TimecodeTrack::~ TimecodeTrack()
{
}

QString
TimecodeTrack::name() const
{
    return m_name;
}

QString
TimecodeTrack::prettyName() const
{
    return name();
}

KUrl
TimecodeTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
TimecodeTrack::uidUrl() const
{
    return m_displayUrl;
}

QString
TimecodeTrack::prettyUrl() const
{
    return m_displayUrl;
}

AlbumPtr
TimecodeTrack::album() const
{
    return AlbumPtr::staticCast( m_album );;
}

bool
TimecodeTrack::isEditable() const
{
    return true;
}

bool
TimecodeTrack::isPlayable() const
{
     return true;
}

ArtistPtr
TimecodeTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );;
}

GenrePtr
TimecodeTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );;
}

ComposerPtr
TimecodeTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );;
}

YearPtr
TimecodeTrack::year() const
{
    return YearPtr::staticCast( m_year );;
}

void
TimecodeTrack::setAlbum( const QString & newAlbum )
{
    Q_UNUSED( newAlbum );
}

void
TimecodeTrack::setTitle( const QString & newTitle )
{
    Q_UNUSED( newTitle );
}

void
TimecodeTrack::setYear( const QString & newYear )
{
    Q_UNUSED( newYear );
}

void
TimecodeTrack::setComposer( const QString & newComposer )
{
    Q_UNUSED( newComposer )
}

void
TimecodeTrack::setGenre( const QString & newGenre )
{
    Q_UNUSED( newGenre )
}

void
TimecodeTrack::setArtist( const QString & newArtist )
{
    Q_UNUSED( newArtist )
}

QString
TimecodeTrack::comment() const
{
    //TODO: might be nice for this kind of user generated track
    return QString();
}

void
TimecodeTrack::setComment( const QString & newComment )
{
    Q_UNUSED( newComment )
}

double
TimecodeTrack::score() const
{
    return 0.0;
}

void
TimecodeTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
TimecodeTrack::rating() const
{
    return -1;
}

int
TimecodeTrack::bitrate() const
{
    return -1;
}

int
TimecodeTrack::sampleRate() const
{
    return -1;
}

int
TimecodeTrack::filesize() const
{
    return -1;
}

int
TimecodeTrack::length() const
{
    m_length;
}

void
TimecodeTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
TimecodeTrack::trackNumber() const
{
    return m_trackNumber;
}

void
TimecodeTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
TimecodeTrack::discNumber() const
{
    return 0;
}

void
TimecodeTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

uint
TimecodeTrack::lastPlayed() const
{
    return 0;
}

int
TimecodeTrack::playCount() const
{
    return 0;
}

QString
TimecodeTrack::type() const
{
    return QString();
}

void
TimecodeTrack::setAlbum( TimecodeAlbumPtr album )
{
    m_album = album;
}

void
TimecodeTrack::setYear( TimecodeYearPtr year )
{
    m_year = year;
}

void
TimecodeTrack::setGenre( TimecodeGenrePtr genre )
{
    m_genre = genre;
}

void
TimecodeTrack::setComposer( TimecodeComposerPtr composer )
{
    m_composer = composer;
}

void
TimecodeTrack::setArtist( TimecodeArtistPtr artist )
{
    m_artist = artist;
}


bool
TimecodeTrack::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    return type == Meta::Capability::BoundedPlayback;
}

Meta::Capability *
TimecodeTrack::asCapabilityInterface( Meta::Capability::Type type )
{
    if ( type == Meta::Capability::BoundedPlayback )
        return new BoundedPlaybackCapabilityImpl( this );
    else
        return 0;
}

qint64 Meta::TimecodeTrack::start()
{
    return m_start;
}

qint64 Meta::TimecodeTrack::end()
{
    return m_end;
}


////////////////// ARTIST //////////////////

TimecodeArtist::TimecodeArtist( const QString & name )
    : m_name( name )
{
}

TimecodeArtist::~ TimecodeArtist()
{
}

QString
TimecodeArtist::name() const
{
    return m_name;
}

QString
TimecodeArtist::prettyName() const
{
    return name();
}

TrackList
TimecodeArtist::tracks()
{
    return m_tracks;
}

AlbumList
TimecodeArtist::albums()
{
    return AlbumList();
}

void
TimecodeArtist::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// ALBUM //////////////////

TimecodeAlbum::TimecodeAlbum( const QString & name )
    : m_name( name )
    , m_isCompilation( false )
{
}

TimecodeAlbum::~ TimecodeAlbum()
{
}

QString
TimecodeAlbum::name() const
{
    return m_name;
}

QString
TimecodeAlbum::prettyName() const
{
    return name();
}

bool
TimecodeAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool TimecodeAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr TimecodeAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
TimecodeAlbum::tracks()
{
    return m_tracks;
}

QPixmap
TimecodeAlbum::image( int size )
{
    return Meta::Album::image( size );
}

bool
TimecodeAlbum::canUpdateImage() const
{
    return false;
}

void
TimecodeAlbum::setImage( const QPixmap & pixmap )
{
    Q_UNUSED(pixmap);
}

void
TimecodeAlbum::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void TimecodeAlbum::setAlbumArtist( TimecodeArtistPtr artist )
{
    m_albumArtist = artist;
}

void TimecodeAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

////////////////// GENRE //////////////////

TimecodeGenre::TimecodeGenre(const QString & name)
    : m_name( name )
{
}

TimecodeGenre::~ TimecodeGenre()
{
}

QString
TimecodeGenre::name() const
{
    return m_name;
}

QString
TimecodeGenre::prettyName() const
{
    return name();
}

TrackList
TimecodeGenre::tracks()
{
    return tracks();
}

void
TimecodeGenre::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// COMPOSER //////////////////

TimecodeComposer::TimecodeComposer( const QString & name )
    : m_name( name )
{
}

TimecodeComposer::~ TimecodeComposer()
{
}

QString
TimecodeComposer::name() const
{
    return m_name;
}

QString
TimecodeComposer::prettyName() const
{
    return name();
}

TrackList
TimecodeComposer::tracks()
{
    return m_tracks;
}

void
TimecodeComposer::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// YEAR //////////////////

TimecodeYear::TimecodeYear( const QString & name )
    : m_name( name )
{
}

TimecodeYear::~ TimecodeYear()
{
}

QString
TimecodeYear::name() const
{
    return m_name;
}

QString
TimecodeYear::prettyName() const
{
    return name();
}

TrackList
TimecodeYear::tracks()
{
    return m_tracks;
}

void
TimecodeYear::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

#include "TimecodeMeta.h"