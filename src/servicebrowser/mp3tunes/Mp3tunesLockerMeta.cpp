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
////////////////////////////////////////////////////////////////////////
//ARTIST
Mp3tunesLockerArtist::Mp3tunesLockerArtist(  mp3tunes_locker_artist_t *artist )
{
    m_artist = artist;
}

Mp3tunesLockerArtist::~Mp3tunesLockerArtist()
{
    delete m_artist;
}

int Mp3tunesLockerArtist::getArtistId() const
{
    return m_artist->artistId;
}

QString Mp3tunesLockerArtist::getArtistName() const
{
    return QString( m_artist->artistName );
}

int Mp3tunesLockerArtist::getArtistSize() const
{
    return m_artist->artistSize;
}

int Mp3tunesLockerArtist::getAlbumCount() const
{
    return m_artist->albumCount;
}

int Mp3tunesLockerArtist::getTrackCount() const
{
    return m_artist->trackCount;
}
////////////////////////////////////////////////////////////////////////
//ALBUM
Mp3tunesLockerAlbum::Mp3tunesLockerAlbum(  mp3tunes_locker_album_t *album )
{
    m_album = album;
}

Mp3tunesLockerAlbum::~Mp3tunesLockerAlbum()
{
    delete m_album;
}

int Mp3tunesLockerAlbum::getAlbumId() const
{
    return m_album->albumId;
}

QString Mp3tunesLockerAlbum::getAlbumTitle() const
{
    return QString( m_album->albumTitle );
}

int Mp3tunesLockerAlbum::getArtistId() const
{
    return m_album->artistId;
}

QString Mp3tunesLockerAlbum::getArtistName() const
{
    return QString( m_album->artistName );
}

int Mp3tunesLockerAlbum::getTrackCount() const
{
    return m_album->trackCount;
}

int Mp3tunesLockerAlbum::getAlbumSize() const
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
    m_track = track;
}
Mp3tunesLockerTrack::~Mp3tunesLockerTrack()
{
    delete m_track;
}

int Mp3tunesLockerTrack::getTrackId() const
{
    return m_track->trackId;
}

QString Mp3tunesLockerTrack::getTrackTitle() const
{
    return QString( m_track->trackTitle );
}

int Mp3tunesLockerTrack::getTrackNumber() const
{
    return m_track->trackNumber;
}

float Mp3tunesLockerTrack::getTrackLength() const
{
    return m_track->trackLength;
}

QString Mp3tunesLockerTrack::getTrackFileName() const
{
    return QString( m_track->trackFileName );
}

QString Mp3tunesLockerTrack::getTrackFileKey() const
{
    return QString( m_track->trackFileKey );
}

int Mp3tunesLockerTrack::getTrackFileSize() const
{
    return m_track->trackFileSize;
}

QString Mp3tunesLockerTrack::getDownloadUrl() const
{
    return QString( m_track->downloadURL );
}

QString Mp3tunesLockerTrack::getPlayUrl() const
{
    return QString( m_track->playURL );
}

int Mp3tunesLockerTrack::getAlbumId() const
{
    return m_track->albumId;
}

QString Mp3tunesLockerTrack::getAlbumTitle() const
{
    return QString( m_track->albumTitle );
}

int Mp3tunesLockerTrack::getAlbumYear() const
{
    return m_track->albumYear;
}

QString Mp3tunesLockerTrack::getArtistName() const
{
    return QString( m_track->artistName );
}
////////////////////////////////////////////////////////////////////////
//PLAYLIST
Mp3tunesLockerPlaylist::Mp3tunesLockerPlaylist(  mp3tunes_locker_playlist_t *playlist )
{
    m_playlist = playlist;
}
Mp3tunesLockerPlaylist::~Mp3tunesLockerPlaylist()
{
    delete m_playlist;
}

QString Mp3tunesLockerPlaylist::getPlaylistId() const
{
    return QString( m_playlist->playlistId );
}

QString Mp3tunesLockerPlaylist::getPlaylistTitle() const{
    return QString( m_playlist->playlistTitle );
}

QString Mp3tunesLockerPlaylist::getTitle() const
{
    return QString( m_playlist->title );
}

QString Mp3tunesLockerPlaylist::getFileName() const
{
    return QString( m_playlist->fileName );
}

int Mp3tunesLockerPlaylist::getFileCount() const
{
    return m_playlist->fileCount;
}

int Mp3tunesLockerPlaylist::getPlaylistSize() const
{
    return m_playlist->playlistSize;
}
