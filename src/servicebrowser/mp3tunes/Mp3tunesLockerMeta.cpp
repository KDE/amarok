/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
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

#include "Mp3tunesLockerMeta.h"
#include "Debug.h"

////////////////////////////////////////////////////////////////////////
//ARTIST
Mp3tunesLockerArtist::Mp3tunesLockerArtist(  mp3tunes_locker_artist_t *artist )
{
    m_artist = ( mp3tunes_locker_artist_t * ) malloc( sizeof( *artist ) );
    memcpy( m_artist, artist, sizeof( *artist ) );
    m_artist->artistName = ( char * ) malloc( strlen( artist->artistName ) );
    strcpy( m_artist->artistName, artist->artistName );
}

Mp3tunesLockerArtist::~Mp3tunesLockerArtist()
{}

int Mp3tunesLockerArtist::artistId() const
{
    return m_artist->artistId;
}

QString Mp3tunesLockerArtist::artistName() const
{
    return QString( m_artist->artistName );
}

int Mp3tunesLockerArtist::artistSize() const
{
    return m_artist->artistSize;
}

int Mp3tunesLockerArtist::albumCount() const
{
    return m_artist->albumCount;
}

int Mp3tunesLockerArtist::trackCount() const
{
    return m_artist->trackCount;
}
////////////////////////////////////////////////////////////////////////
//ALBUM
Mp3tunesLockerAlbum::Mp3tunesLockerAlbum(  mp3tunes_locker_album_t *album )
{
    m_album = ( mp3tunes_locker_album_t * ) malloc( sizeof( *album ) );
    memcpy( m_album, album, sizeof( *album ) );

    m_album->artistName = ( char * ) malloc( strlen( album->artistName ) );
    strcpy( m_album->artistName, album->artistName );

    m_album->albumTitle = ( char * ) malloc( strlen( album->albumTitle ) );
    strcpy( m_album->albumTitle, album->albumTitle );
}

Mp3tunesLockerAlbum::~Mp3tunesLockerAlbum()
{}

int Mp3tunesLockerAlbum::albumId() const
{
    return m_album->albumId;
}

QString Mp3tunesLockerAlbum::albumTitle() const
{
    return QString( m_album->albumTitle );
}

int Mp3tunesLockerAlbum::artistId() const
{
    return m_album->artistId;
}

QString Mp3tunesLockerAlbum::artistName() const
{
    return QString( m_album->artistName );
}

int Mp3tunesLockerAlbum::trackCount() const
{
    return m_album->trackCount;
}

int Mp3tunesLockerAlbum::albumSize() const
{
    return m_album->albumSize;
}

bool Mp3tunesLockerAlbum::hasArt() const
{
    return m_album->hasArt;
}
////////////////////////////////////////////////////////////////////////
//TRACK
Mp3tunesLockerTrack::Mp3tunesLockerTrack(  mp3tunes_locker_track_t *track )
{
    m_track = ( mp3tunes_locker_track_t * ) malloc( sizeof( *track ) );
    memcpy( m_track, track, sizeof( *track ) );

    m_track->artistName = ( char * ) malloc( strlen( track->artistName ) );
    strcpy( m_track->artistName, track->artistName );

    m_track->albumTitle = ( char * ) malloc( strlen( track->albumTitle ) );
    strcpy( m_track->albumTitle, track->albumTitle );

    m_track->trackTitle = ( char * ) malloc( strlen( track->trackTitle ) );
    strcpy( m_track->trackTitle, track->trackTitle );

    m_track->trackFileName = ( char * ) malloc( strlen( track->trackFileName ) );
    strcpy( m_track->trackFileName, track->trackFileName );

    m_track->trackFileKey = ( char * ) malloc( strlen( track->trackFileKey ) );
    strcpy( m_track->trackFileKey, track->trackFileKey );

    m_track->downloadURL = ( char * ) malloc( strlen( track->downloadURL ) );
    strcpy( m_track->downloadURL, track->downloadURL );

    m_track->playURL = ( char * ) malloc( strlen( track->playURL ) );
    strcpy( m_track->playURL, track->playURL );
}
Mp3tunesLockerTrack::~Mp3tunesLockerTrack()
{}

int Mp3tunesLockerTrack::trackId() const
{
    return m_track->trackId;
}

QString Mp3tunesLockerTrack::trackTitle() const
{
    return QString( m_track->trackTitle );
}

int Mp3tunesLockerTrack::trackNumber() const
{
    return m_track->trackNumber;
}

float Mp3tunesLockerTrack::trackLength() const
{
    return m_track->trackLength;
}

QString Mp3tunesLockerTrack::trackFileName() const
{
    return QString( m_track->trackFileName );
}

QString Mp3tunesLockerTrack::trackFileKey() const
{
    return QString( m_track->trackFileKey );
}

int Mp3tunesLockerTrack::trackFileSize() const
{
    return m_track->trackFileSize;
}

QString Mp3tunesLockerTrack::downloadUrl() const
{
    return QString( m_track->downloadURL );
}

QString Mp3tunesLockerTrack::playUrl() const
{
    return QString( m_track->playURL );
}

int Mp3tunesLockerTrack::albumId() const
{
    return m_track->albumId;
}

QString Mp3tunesLockerTrack::albumTitle() const
{
    return QString( m_track->albumTitle );
}

int Mp3tunesLockerTrack::albumYear() const
{
    return m_track->albumYear;
}

QString Mp3tunesLockerTrack::artistName() const
{
    return QString( m_track->artistName );
}
////////////////////////////////////////////////////////////////////////
//PLAYLIST
Mp3tunesLockerPlaylist::Mp3tunesLockerPlaylist(  mp3tunes_locker_playlist_t *playlist )
{
    m_playlist = ( mp3tunes_locker_playlist_t * ) malloc( sizeof( *playlist ) );
    memcpy( m_playlist, playlist, sizeof( *playlist ) );

    m_playlist->playlistId = ( char * ) malloc( strlen( playlist->playlistId ) );
    strcpy( m_playlist->playlistId, playlist->playlistId );

    m_playlist->playlistTitle = ( char * ) malloc( strlen( playlist->playlistTitle ) );
    strcpy( m_playlist->playlistTitle, playlist->playlistTitle );

    m_playlist->title = ( char * ) malloc( strlen( playlist->title ) );
    strcpy( m_playlist->title, playlist->title );

    m_playlist->fileName = ( char * ) malloc( strlen( playlist->fileName ) );
    strcpy( m_playlist->fileName, playlist->fileName );
}
Mp3tunesLockerPlaylist::~Mp3tunesLockerPlaylist()
{}

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
