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

PodcastTrack::PodcastTrack()
        : Meta::Track()
        , m_genre( 0 )
        , m_composer( 0 )
        , m_year( 0 )
        , m_type( 0 )
{}

PodcastTrack::~PodcastTrack()
{
    //nothing to do
}


void
PodcastTrack::setId( int id )
{
    m_id = id;
}

int

PodcastTrack::id() const
{
    return m_id;
}

void

PodcastTrack::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int

PodcastTrack::albumId() const
{
    return m_albumId;
}

void

PodcastTrack::setAlbumName( const QString & name )
{
    m_albumName = name;
}

QString

PodcastTrack::albumName() const
{
    return m_albumName;
}

void

PodcastTrack::setArtistId( int id )
{
    m_artistId = id;
}

int

PodcastTrack::artistId() const
{
    return m_artistId;
}

void

PodcastTrack::setArtistName( const QString & name )
{
    m_artistName = name;
}

QString

PodcastTrack::artistName() const
{
    return m_artistName;
}


QString

PodcastTrack::name() const
{
    return m_name;
}

QString

PodcastTrack::prettyName() const
{
    return m_name;
}

KUrl

PodcastTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString

PodcastTrack::url() const
{
    return m_playableUrl;
}

QString

PodcastTrack::prettyUrl() const
{
    return m_displayUrl;
}

void PodcastTrack::setUrl( const QString & url )

{
    m_playableUrl = url;
    m_displayUrl = url;
}

bool

PodcastTrack::isPlayable() const
{
    return true;
}

bool

PodcastTrack::isEditable() const
{
    return false;
}

AlbumPtr

PodcastTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr

PodcastTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr

PodcastTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr

PodcastTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr

PodcastTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void

PodcastTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void

PodcastTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void

PodcastTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void

PodcastTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void

PodcastTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString

PodcastTrack::comment() const
{
    return QString();
}

void

PodcastTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double

PodcastTrack::score() const
{
    return 0.0;
}

void

PodcastTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int

PodcastTrack::rating() const
{
    return 0;
}

void

PodcastTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int

PodcastTrack::length() const
{
    return m_length;
}

int

PodcastTrack::filesize() const
{
    return 0;
}

int

PodcastTrack::sampleRate() const
{
    return 0;
}

int

PodcastTrack::bitrate() const
{
    return 0;
}

int

PodcastTrack::trackNumber() const
{
    return m_trackNumber;
}

void

PodcastTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int

PodcastTrack::discNumber() const
{
    return 0;
}

void

PodcastTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int

PodcastTrack::playCount() const
{
    return 0;
}

uint

PodcastTrack::lastPlayed() const
{
    return 0;
}

QString

PodcastTrack::type() const
{
    return m_type;
}

void

PodcastTrack::subscribe( TrackObserver *observer )
{
    Q_UNUSED( observer )
}

void
PodcastTrack::unsubscribe( TrackObserver *observer )
{
    Q_UNUSED( observer )
}

void
PodcastTrack::setAlbum( AlbumPtr album )
{
    m_album = album;
}

void
PodcastTrack::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
PodcastTrack::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
PodcastTrack::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
PodcastTrack::setYear( YearPtr year )
{
    m_year = year;
}

void
PodcastTrack::setLength( int length )
{
    m_length = length;
}


// void PodcastTrack::/*processInfoOf*/(InfoParserBase * infoParser)
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

// void PodcastArtist::processInfoOf(InfoParserBase * infoParser)
// {
//     infoParser->getInfo( ArtistPtr ( this ) );
// }

PodcastAlbum::PodcastAlbum()
        : Meta::Album()
{
//     m_id = resultRow[0].toInt();
//     m_name = resultRow[1];
//     m_description = resultRow[2];
//     m_artistId = resultRow[3].toInt();
//     m_artistName = resultRow[4];
}



PodcastAlbum::~PodcastAlbum()

{
    //nothing to do
}


void PodcastAlbum::setId( int id )
{
    m_id = id;
}

int PodcastAlbum::id() const
{
    return m_id;
}

void PodcastAlbum::setArtistId( int artistId )

{
    m_artistId = artistId;
}

int PodcastAlbum::artistId() const
{
    return m_artistId;
}

void PodcastAlbum::setArtistName( const QString & name )

{
    m_artistName = name;
}

QString PodcastAlbum::artistName() const

{
    return m_artistName;
}

QString

PodcastAlbum::name() const
{
    return m_name;
}

QString

PodcastAlbum::prettyName() const
{
    return m_name;
}

// void
// PodcastAlbum::setTitle( const QString & title )
// {
//     m_name = title;
// }

bool
PodcastAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
PodcastAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
PodcastAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList

PodcastAlbum::tracks()
{
    return m_tracks;
}

void
PodcastAlbum::image() const
{
    //TODO
}

bool
PodcastAlbum::canUpdateImage() const
{
    return false;
}

void
PodcastAlbum::updateImage()
{
    //TODO
}

void
PodcastAlbum::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
PodcastAlbum::setAlbumArtist( ArtistPtr artist )
{
    m_albumArtist = artist;
}

void
PodcastAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

// void PodcastAlbum::processInfoOf(InfoParserBase * infoParser)
// {
//     infoParser->getInfo( AlbumPtr( this ) );
// }



//PodcastGenre

PodcastGenre::PodcastGenre( const QString &name )
        : Meta::Genre()
//         , PodcastDisplayInfoProvider()
        , m_name( name )
        , m_tracks()
{
    //nothing to do
}

PodcastGenre::PodcastGenre( const QStringList & row )

        : Meta::Genre()
        , m_tracks()
{
    m_name = row[0];
}

PodcastGenre::~PodcastGenre()

{
    //nothing to do
}

QString
PodcastGenre::name() const
{
    return m_name;
}

QString
PodcastGenre::prettyName() const
{
    return m_name;
}

void
PodcastGenre::setName( const QString & name )
{
    m_name = name;
}


int
PodcastGenre::albumId()
{
    return m_albumId;
}

void
PodcastGenre::setAlbumId( int albumId )
{
    m_albumId = albumId;
}


TrackList
PodcastGenre::tracks()
{
    return m_tracks;
}

void
PodcastGenre::addTrack( PodcastTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

// void PodcastGenre::processInfoOf(InfoParserBase * infoParser)
// {
//     return; // do nothing
// }

//PodcastComposer

PodcastComposer::PodcastComposer( const QString &name )
        : Meta::Composer()
//         , PodcastDisplayInfoProvider()
        , m_name( name )
        , m_tracks()
{
    //nothing to do
}

PodcastComposer::~PodcastComposer()

{
    //nothing to do
}

QString
PodcastComposer::name() const
{
    return m_name;
}

QString

PodcastComposer::prettyName() const
{
    return m_name;
}

TrackList

PodcastComposer::tracks()
{
    return m_tracks;
}

void
PodcastComposer::addTrack( PodcastTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

// void PodcastComposer::processInfoOf(InfoParserBase * infoParser)
// {
//     return; // do nothing
// }

//PodcastYear

PodcastYear::PodcastYear( const QString &name )
        : Meta::Year()
//         , PodcastDisplayInfoProvider()
        , m_name( name )
        , m_tracks()
{
    //nothing to do
}

PodcastYear::~PodcastYear()

{
    //nothing to do
}

QString

PodcastYear::name() const
{
    return m_name;
}

QString

PodcastYear::prettyName() const
{
    return m_name;
}

TrackList

PodcastYear::tracks()
{
    return m_tracks;
}

void
PodcastYear::addTrack( PodcastTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}


// void PodcastYear::/*/*processInfoOf*/*/(InfoParserBase * infoParser)
// {
//     return; // do nothing
// }
