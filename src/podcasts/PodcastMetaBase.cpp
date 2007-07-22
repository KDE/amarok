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

#include "PodcastMetaBase.h"

PodcastEpisode::PodcastEpisode()
        : Meta::Track()
        , m_genre( 0 )
        , m_composer( 0 )
        , m_year( 0 )
        , m_type( 0 )
{}

PodcastEpisode::~PodcastEpisode()
{
    //nothing to do
}


void
PodcastEpisode::setId( int id )
{
    m_id = id;
}

int

PodcastEpisode::id() const
{
    return m_id;
}

void

PodcastEpisode::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int

PodcastEpisode::albumId() const
{
    return m_albumId;
}

void

PodcastEpisode::setAlbumName( const QString & name )
{
    m_albumName = name;
}

QString

PodcastEpisode::albumName() const
{
    return m_albumName;
}

void

PodcastEpisode::setArtistId( int id )
{
    m_artistId = id;
}

int

PodcastEpisode::artistId() const
{
    return m_artistId;
}

void

PodcastEpisode::setArtistName( const QString & name )
{
    m_artistName = name;
}

QString

PodcastEpisode::artistName() const
{
    return m_artistName;
}


QString

PodcastEpisode::name() const
{
    return m_name;
}

QString

PodcastEpisode::prettyName() const
{
    return m_name;
}

KUrl

PodcastEpisode::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString

PodcastEpisode::url() const
{
    return m_playableUrl;
}

QString

PodcastEpisode::prettyUrl() const
{
    return m_displayUrl;
}

void PodcastEpisode::setUrl( const QString & url )

{
    m_playableUrl = url;
    m_displayUrl = url;
}

bool

PodcastEpisode::isPlayable() const
{
    return true;
}

bool

PodcastEpisode::isEditable() const
{
    return false;
}

AlbumPtr

PodcastEpisode::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr

PodcastEpisode::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr

PodcastEpisode::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr

PodcastEpisode::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr

PodcastEpisode::year() const
{
    return YearPtr::staticCast( m_year );
}

void

PodcastEpisode::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void

PodcastEpisode::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void

PodcastEpisode::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void

PodcastEpisode::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void

PodcastEpisode::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString

PodcastEpisode::comment() const
{
    return QString();
}

void

PodcastEpisode::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double

PodcastEpisode::score() const
{
    return 0.0;
}

void

PodcastEpisode::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int

PodcastEpisode::rating() const
{
    return 0;
}

void

PodcastEpisode::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int

PodcastEpisode::length() const
{
    return m_length;
}

int

PodcastEpisode::filesize() const
{
    return 0;
}

int

PodcastEpisode::sampleRate() const
{
    return 0;
}

int

PodcastEpisode::bitrate() const
{
    return 0;
}

int

PodcastEpisode::trackNumber() const
{
    return m_trackNumber;
}

void

PodcastEpisode::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int

PodcastEpisode::discNumber() const
{
    return 0;
}

void

PodcastEpisode::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int

PodcastEpisode::playCount() const
{
    return 0;
}

uint

PodcastEpisode::lastPlayed() const
{
    return 0;
}

QString

PodcastEpisode::type() const
{
    return m_type;
}

void
PodcastEpisode::setAlbum( AlbumPtr album )
{
    m_album = album;
}

void
PodcastEpisode::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
PodcastEpisode::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
PodcastEpisode::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
PodcastEpisode::setYear( YearPtr year )
{
    m_year = year;
}

void
PodcastEpisode::setLength( int length )
{
    m_length = length;
}


// void PodcastEpisode::/*processInfoOf*/(InfoParserBase * infoParser)
// {
//     infoParser->getInfo( TrackPtr( this ) );
// }


//PodcastArtist



PodcastArtist::PodcastArtist( const QString & name )
        : Meta::Artist()
//         , PodcastDisplayInfoProvider()
        , m_id( 0 )
        , m_name( name )
        , m_description( 0 )
        , m_tracks()
{
    //nothing to do
}

PodcastArtist::PodcastArtist()

        : Meta::Artist()
        , m_tracks()
{

//     m_id = resultRow[0].toInt();
//     m_name = resultRow[1];
//     m_description = resultRow[2];


}

PodcastArtist::~PodcastArtist()
{
    //nothing to do
}


void PodcastArtist::setId( int id )
{
    m_id = id;
}

int PodcastArtist::id() const
{
    return m_id;
}



QString

PodcastArtist::name() const
{
    return m_name;
}

QString

PodcastArtist::prettyName() const
{
    return m_name;
}

void PodcastArtist::setTitle( const QString & title )

{
    m_name = title;
}

TrackList

PodcastArtist::tracks()
{
    return m_tracks;
}

void
PodcastArtist::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}


void PodcastArtist::setDescription( const QString &description )
{
    m_description = description;
}

QString PodcastArtist::description() const

{
    return m_description;
}

PodcastChannel::PodcastChannel()
        : Meta::Album()
{
//     m_id = resultRow[0].toInt();
//     m_name = resultRow[1];
//     m_description = resultRow[2];
//     m_artistId = resultRow[3].toInt();
//     m_artistName = resultRow[4];
}



PodcastChannel::~PodcastChannel()

{
    //nothing to do
}


void PodcastChannel::setId( int id )
{
    m_id = id;
}

int PodcastChannel::id() const
{
    return m_id;
}

void PodcastChannel::setArtistId( int artistId )

{
    m_artistId = artistId;
}

int PodcastChannel::artistId() const
{
    return m_artistId;
}

void PodcastChannel::setArtistName( const QString & name )

{
    m_artistName = name;
}

QString PodcastChannel::artistName() const

{
    return m_artistName;
}

QString

PodcastChannel::name() const
{
    return m_name;
}

QString

PodcastChannel::prettyName() const
{
    return m_name;
}


bool
PodcastChannel::isCompilation() const
{
    return m_isCompilation;
}

bool
PodcastChannel::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
PodcastChannel::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList

PodcastChannel::tracks()
{
    return m_tracks;
}

void
PodcastChannel::image() const
{
    //TODO
}

bool
PodcastChannel::canUpdateImage() const
{
    return false;
}

void
PodcastChannel::updateImage()
{
    //TODO
}

void
PodcastChannel::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
PodcastChannel::setAlbumArtist( ArtistPtr artist )
{
    m_albumArtist = artist;
}

void
PodcastChannel::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

// void PodcastChannel::processInfoOf(InfoParserBase * infoParser)
// {
//     infoParser->getInfo( AlbumPtr( this ) );
// }
