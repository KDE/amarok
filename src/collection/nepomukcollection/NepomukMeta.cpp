/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

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

#define DEBUG_PREFIX "NepomukMeta"

#include "NepomukMeta.h"

#include "Debug.h"

using namespace Meta;

NepomukArtist::NepomukArtist( const QString &name )
    : Artist()
    , m_name( name )
{

}

QString
NepomukArtist::name() const
{
    return m_name;
}

QString
NepomukArtist::prettyName() const
{
    return m_name;
}

TrackList
NepomukArtist::tracks()
{
    return TrackList();
}

AlbumList
NepomukArtist::albums()
{
    return AlbumList();
}

// --- ALBUM ----

NepomukAlbum::NepomukAlbum( const QString &name, const QString &artist )
    : Album()
    , m_name( name )
    , m_artist( artist )
{

}

QString
NepomukAlbum::name() const
{
    return m_name;
}

QString
NepomukAlbum::prettyName() const
{
    return m_name;
}

TrackList
NepomukAlbum::tracks()
{
    return TrackList();
}

bool
NepomukAlbum::isCompilation() const
{
    return false;
}

bool
NepomukAlbum::hasAlbumArtist() const
{
    return true;
}

ArtistPtr
NepomukAlbum::albumArtist() const
{
    return ArtistPtr( new NepomukArtist( m_artist ) );
}


// -- TRACK --

NepomukTrack::NepomukTrack( const KUrl &url, const QString &title, const QString &artist, 
                const QString &album, const QString &genre, const QString &year,
                const QString &composer, const QString &type, const int &trackNumber,
                const int &length )
    : Track()
    , m_url( url )
    , m_title( title )
    , m_artist( artist )
    , m_album( album )
    , m_genre( genre )
    , m_year( year )
    , m_composer ( composer )
    , m_type ( type )
    , m_trackNumber ( trackNumber )
    , m_length( length )
{
     
}

QString
NepomukTrack::name() const
{
    return m_title;
}

QString
NepomukTrack::prettyName() const
{
    return m_title;
}

KUrl
NepomukTrack::playableUrl() const
{
    return m_url;
}

QString
NepomukTrack::url() const
{
    return m_url.url();
}

QString
NepomukTrack::prettyUrl() const
{
    return m_url.prettyUrl();
}

bool
NepomukTrack::isPlayable() const
{
    return true;
}

AlbumPtr
NepomukTrack::album() const
{
    return AlbumPtr( new NepomukAlbum( m_album, m_artist ) );
}

ArtistPtr
NepomukTrack::artist() const
{
    return ArtistPtr( new NepomukArtist( m_artist ) );
}

GenrePtr
NepomukTrack::genre() const
{
    return GenrePtr( new NepomukGenre( m_genre ) );
}

ComposerPtr
NepomukTrack::composer() const
{
    return ComposerPtr( new NepomukComposer( m_composer ) );
}

YearPtr
NepomukTrack::year() const
{
    return YearPtr( new NepomukYear( m_year ) );
}

QString
NepomukTrack::comment() const
{
    return QString();
}

double
NepomukTrack::score() const
{
    return 0.0;
}

void
NepomukTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
NepomukTrack::rating() const
{
    return 0;
}

void
NepomukTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
NepomukTrack::length() const
{
    return m_length;
}

int
NepomukTrack::filesize() const
{
    return 0;
}

int
NepomukTrack::sampleRate() const
{
    return 0;
}

int
NepomukTrack::bitrate() const
{
    return 0;
}

int
NepomukTrack::trackNumber() const
{
    return m_trackNumber;
}

int
NepomukTrack::discNumber() const
{
    return 0;
}

uint
NepomukTrack::lastPlayed() const
{
    return 0;
}

int
NepomukTrack::playCount() const
{
    return 0;
}

QString
NepomukTrack::type() const
{
    return m_type;
}

// -- GENRE --

NepomukGenre::NepomukGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
{

}

QString
NepomukGenre::name() const
{
    return m_name;
}

QString
NepomukGenre::prettyName() const
{
    return m_name;
}

TrackList
NepomukGenre::tracks()
{
    return TrackList();
}

// -- COMPOSER --

NepomukComposer::NepomukComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
{

}

QString
NepomukComposer::name() const
{
    return m_name;
}

QString
NepomukComposer::prettyName() const
{
    return m_name;
}

TrackList
NepomukComposer::tracks()
{
    return TrackList();
}

// -- YEAR --

NepomukYear::NepomukYear( const QString &name )
    : Meta::Year()
    , m_name( name )
{

}

QString
NepomukYear::name() const
{
    return m_name;
}

QString
NepomukYear::prettyName() const
{
    return m_name;
}

TrackList
NepomukYear::tracks()
{
    return TrackList();
}




