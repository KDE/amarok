/****************************************************************************************
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Mp3tunesLockerMeta.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////
//ARTIST
Mp3tunesLockerArtist::Mp3tunesLockerArtist(  mp3tunes_locker_artist_t *artist )
    : m_artistId( 0 )
    , m_artistName()
    , m_artistSize( 0 )
    , m_albumCount( 0 )
    , m_trackCount( 0 )
{
    if( !artist ) return;

    m_artistId = artist->artistId;
    m_artistName = artist->artistName;
    m_artistSize = artist->artistSize;
    m_albumCount = artist->albumCount;
    m_trackCount = artist->trackCount;
}

Mp3tunesLockerArtist::~Mp3tunesLockerArtist()
{}

int Mp3tunesLockerArtist::artistId() const
{
    return m_artistId;
}

QString Mp3tunesLockerArtist::artistName() const
{
    return m_artistName;
}

int Mp3tunesLockerArtist::artistSize() const
{
    return m_artistSize;
}

int Mp3tunesLockerArtist::albumCount() const
{
    return m_albumCount;
}

int Mp3tunesLockerArtist::trackCount() const
{
    return m_trackCount;
}
////////////////////////////////////////////////////////////////////////
//ALBUM
Mp3tunesLockerAlbum::Mp3tunesLockerAlbum(  mp3tunes_locker_album_t *album )
    : m_albumId( 0 )
    , m_albumTitle()
    , m_artistId( 0 )
    , m_artistName()
    , m_trackCount( 0 )
    , m_albumSize( 0 )
    , m_hasArt( false )
{
    if( !album ) return;

    m_albumId = album->albumId;
    m_albumTitle = album->albumTitle;
    m_artistId = album->artistId;
    m_artistName = album->artistName;
    m_trackCount = album->trackCount;
    m_albumSize = album->albumSize;
    m_hasArt = album->hasArt;
}

Mp3tunesLockerAlbum::~Mp3tunesLockerAlbum()
{}

int Mp3tunesLockerAlbum::albumId() const
{
    return m_albumId;
}

QString Mp3tunesLockerAlbum::albumTitle() const
{
    return m_albumTitle;
}

int Mp3tunesLockerAlbum::artistId() const
{
    return m_artistId;
}

QString Mp3tunesLockerAlbum::artistName() const
{
    return m_artistName;
}

int Mp3tunesLockerAlbum::trackCount() const
{
    return m_trackCount;
}

int Mp3tunesLockerAlbum::albumSize() const
{
    return m_albumSize;
}

bool Mp3tunesLockerAlbum::hasArt() const
{
    return m_hasArt;
}
////////////////////////////////////////////////////////////////////////
//TRACK
Mp3tunesLockerTrack::Mp3tunesLockerTrack(  mp3tunes_locker_track_t *track )
    : m_trackId( 0 )
    , m_trackTitle()
    , m_trackNumber( 0 )
    , m_trackLength( 0.0 )
    , m_trackFileName()
    , m_trackFileKey()
    , m_trackFileSize( 0 )
    , m_downloadUrl()
    , m_playUrl()
    , m_albumId( 0 )
    , m_albumTitle()
    , m_albumYear( 0 )
    , m_artistName()
    , m_artistId( 0 )
{
    if ( !track ) return;

    m_trackTitle = track->trackTitle;
    m_trackNumber = track->trackNumber;
    m_trackLength = track->trackLength;
    m_trackFileName = track->trackFileName;
    m_trackFileKey = track->trackFileKey;
    m_trackFileSize = track->trackFileSize;
    m_downloadUrl = track->downloadURL;
    m_playUrl = track->playURL;
    m_albumId = track->albumId;
    m_albumTitle = track->albumTitle;
    m_albumYear = track->albumYear;
    m_artistName = track->artistName;
    m_artistId = track->artistId;
}

Mp3tunesLockerTrack::~Mp3tunesLockerTrack()
{}

int Mp3tunesLockerTrack::trackId() const
{
    return m_trackId;
}

QString Mp3tunesLockerTrack::trackTitle() const
{
    return m_trackTitle;
}

int Mp3tunesLockerTrack::trackNumber() const
{
    return m_trackNumber;
}

float Mp3tunesLockerTrack::trackLength() const
{
    return m_trackLength;
}

QString Mp3tunesLockerTrack::trackFileName() const
{
    return m_trackFileName;
}

QString Mp3tunesLockerTrack::trackFileKey() const
{
    return m_trackFileKey;
}

int Mp3tunesLockerTrack::trackFileSize() const
{
    return m_trackFileSize;
}

QString Mp3tunesLockerTrack::downloadUrl() const
{
    return m_downloadUrl;
}

QString Mp3tunesLockerTrack::playUrl() const
{
    return m_playUrl;
}

int Mp3tunesLockerTrack::albumId() const
{
    return m_albumId;
}

QString Mp3tunesLockerTrack::albumTitle() const
{
    return m_albumTitle;
}

int Mp3tunesLockerTrack::albumYear() const
{
    return m_albumYear;
}

QString Mp3tunesLockerTrack::artistName() const
{
    return m_artistName;
}

int Mp3tunesLockerTrack::artistId() const
{
    return m_artistId;
}
////////////////////////////////////////////////////////////////////////
//PLAYLIST
Mp3tunesLockerPlaylist::Mp3tunesLockerPlaylist(  mp3tunes_locker_playlist_t *playlist )
{
    m_playlist = ( mp3tunes_locker_playlist_t * ) malloc( sizeof( *playlist ) );
    memcpy( m_playlist, playlist, sizeof( *playlist ) );

    m_playlist->playlistId = ( char * ) malloc( strlen( playlist->playlistId ) + 1 );
    strcpy( m_playlist->playlistId, playlist->playlistId );

    m_playlist->playlistTitle = ( char * ) malloc( strlen( playlist->playlistTitle ) + 1 );
    strcpy( m_playlist->playlistTitle, playlist->playlistTitle );

    m_playlist->title = ( char * ) malloc( strlen( playlist->title ) + 1 );
    strcpy( m_playlist->title, playlist->title );

    m_playlist->fileName = ( char * ) malloc( strlen( playlist->fileName ) + 1 );
    strcpy( m_playlist->fileName, playlist->fileName );
}
Mp3tunesLockerPlaylist::~Mp3tunesLockerPlaylist()
{
    free(m_playlist->fileName);
    free(m_playlist->title);
    free(m_playlist->playlistTitle);
    free(m_playlist->playlistId);
    free(m_playlist);
}

QString Mp3tunesLockerPlaylist::playlistId() const
{
    return QString( m_playlist->playlistId );
}

QString Mp3tunesLockerPlaylist::playlistTitle() const{
    return QString( m_playlist->playlistTitle );
}

QString Mp3tunesLockerPlaylist::title() const
{
    return QString( m_playlist->title );
}

QString Mp3tunesLockerPlaylist::fileName() const
{
    return QString( m_playlist->fileName );
}

int Mp3tunesLockerPlaylist::fileCount() const
{
    return m_playlist->fileCount;
}

int Mp3tunesLockerPlaylist::playlistSize() const
{
    return m_playlist->playlistSize;
}
