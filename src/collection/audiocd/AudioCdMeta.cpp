/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "AudioCdMeta.h"
#include "AudioCdCollection.h"

#include "Debug.h"

using namespace Meta;

AudioCdTrack::AudioCdTrack( AudioCdCollection *collection, const QString &name, const QString &url )
    : Meta::Track()
    , m_collection( collection )
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name( name)
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_displayUrl( url )
    , m_playableUrl( url )
{
}

AudioCdTrack::~AudioCdTrack()
{
    //nothing to do
}

QString
AudioCdTrack::name() const
{
    return m_name;
}

QString
AudioCdTrack::prettyName() const
{
    return m_name;
}

KUrl
AudioCdTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
AudioCdTrack::uidUrl() const
{
    return m_playableUrl;
}

QString
AudioCdTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
AudioCdTrack::isPlayable() const
{
    return true;
}

bool
AudioCdTrack::isEditable() const
{
    return false;
}

AlbumPtr
AudioCdTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
AudioCdTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
AudioCdTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
AudioCdTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
AudioCdTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
AudioCdTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
AudioCdTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
AudioCdTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
AudioCdTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
AudioCdTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

float
AudioCdTrack::bpm() const
{
    return -1.0;
}

QString
AudioCdTrack::comment() const
{
    return QString();
}

void
AudioCdTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
AudioCdTrack::score() const
{
    return 0.0;
}

void
AudioCdTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
AudioCdTrack::rating() const
{
    return 0;
}

void
AudioCdTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

qint64
AudioCdTrack::length() const
{
    return m_length;
}

int
AudioCdTrack::filesize() const
{
    return 0;
}

int
AudioCdTrack::sampleRate() const
{
    return 0;
}

int
AudioCdTrack::bitrate() const
{
    return 0;
}

int
AudioCdTrack::trackNumber() const
{
    return m_trackNumber;
}

void
AudioCdTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
AudioCdTrack::discNumber() const
{
    return 0;
}

void
AudioCdTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
AudioCdTrack::playCount() const
{
    return 0;
}

uint
AudioCdTrack::lastPlayed() const
{
    return 0;
}

QString
AudioCdTrack::type() const
{
    return m_collection->encodingFormat();
}

void
AudioCdTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
AudioCdTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

bool
AudioCdTrack::inCollection() const
{
    return true;
}

Amarok::Collection*
AudioCdTrack::collection() const
{
    return m_collection;
}

void
AudioCdTrack::setAlbum( AudioCdAlbumPtr album )
{
    m_album = album;
}

void
AudioCdTrack::setArtist( AudioCdArtistPtr artist )
{
    m_artist = artist;
}

void
AudioCdTrack::setGenre( AudioCdGenrePtr genre )
{
    m_genre = genre;
}

void
AudioCdTrack::setComposer( AudioCdComposerPtr composer )
{
    m_composer = composer;
}

void
AudioCdTrack::setYear( AudioCdYearPtr year )
{
    m_year = year;
}

void
AudioCdTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
AudioCdTrack::setLength( qint64 length )
{
    m_length = length;
}

void Meta::AudioCdTrack::setFileNameBase( const QString & fileNameBase )
{
    m_fileNameBase = fileNameBase;
}

QString Meta::AudioCdTrack::fileNameBase()
{
    return m_fileNameBase;
}


//AudioCdArtist

AudioCdArtist::AudioCdArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

AudioCdArtist::~AudioCdArtist()
{
    //nothing to do
}

QString
AudioCdArtist::name() const
{
    return m_name;
}

QString
AudioCdArtist::prettyName() const
{
    return m_name;
}

TrackList
AudioCdArtist::tracks()
{
    return m_tracks;
}

AlbumList
AudioCdArtist::albums()
{
    //TODO
    return AlbumList();
}

void
AudioCdArtist::addTrack( AudioCdTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

AudioCdAlbum::AudioCdAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

AudioCdAlbum::~AudioCdAlbum()
{
    //nothing to do
}

QString
AudioCdAlbum::name() const
{
    return m_name;
}

QString
AudioCdAlbum::prettyName() const
{
    return m_name;
}

bool
AudioCdAlbum::isCompilation() const
{
    DEBUG_BLOCK
    return m_isCompilation;
}

bool
AudioCdAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
AudioCdAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
AudioCdAlbum::tracks()
{
    return m_tracks;
}

QPixmap
AudioCdAlbum::image( int size )
{
    if ( m_cover.isNull() )
        return Meta::Album::image( size );

    //only cache during session
    if ( m_coverSizeMap.contains( size ) )
         return m_coverSizeMap.value( size );

    QPixmap scaled = m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    m_coverSizeMap.insert( size, scaled );
    return scaled;
}

bool
AudioCdAlbum::canUpdateImage() const
{
    return false;
}

void
AudioCdAlbum::setImage( const QPixmap &pixmap )
{
    m_cover = pixmap;
}

void
AudioCdAlbum::addTrack( AudioCdTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
AudioCdAlbum::setAlbumArtist( AudioCdArtistPtr artist )
{
    m_albumArtist = artist;
}

void
AudioCdAlbum::setIsCompilation( bool compilation )
{
    DEBUG_BLOCK
    m_isCompilation = compilation;
}

//AudioCdGenre

AudioCdGenre::AudioCdGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

AudioCdGenre::~AudioCdGenre()
{
    //nothing to do
}

QString
AudioCdGenre::name() const
{
    return m_name;
}

QString
AudioCdGenre::prettyName() const
{
    return m_name;
}

TrackList
AudioCdGenre::tracks()
{
    return m_tracks;
}

void
AudioCdGenre::addTrack( AudioCdTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//AudioCdComposer

AudioCdComposer::AudioCdComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

AudioCdComposer::~AudioCdComposer()
{
    //nothing to do
}

QString
AudioCdComposer::name() const
{
    return m_name;
}

QString
AudioCdComposer::prettyName() const
{
    return m_name;
}

TrackList
AudioCdComposer::tracks()
{
    return m_tracks;
}

void
AudioCdComposer::addTrack( AudioCdTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

//AudioCdYear

AudioCdYear::AudioCdYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

AudioCdYear::~AudioCdYear()
{
    //nothing to do
}

QString
AudioCdYear::name() const
{
    return m_name;
}

QString
AudioCdYear::prettyName() const
{
    return m_name;
}

TrackList
AudioCdYear::tracks()
{
    return m_tracks;
}

void
AudioCdYear::addTrack( AudioCdTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}



