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
#ifndef MP3TUNESLOCKERMETA_H
#define MP3TUNESLOCKERMETA_H

extern "C" {
   // Get libmp3tunes declarations
#include "libmp3tunes/locker.h"
}

#include <QString>
#include <QList>

class Mp3tunesLockerPlaylist {
    public:
        Mp3tunesLockerPlaylist( mp3tunes_locker_playlist_t *playlist );
        ~Mp3tunesLockerPlaylist();
        
        QString getPlaylistId() const;
        QString getPlaylistTitle() const;
        QString getTitle() const;
        QString getFileName() const;
        int getFileCount() const;
        int getPlaylistSize() const;
    private:
        mp3tunes_locker_playlist_t *m_playlist;
};

class Mp3tunesLockerArtist {
    public:
        Mp3tunesLockerArtist( mp3tunes_locker_artist_t *artist );
        ~Mp3tunesLockerArtist();
        
        int getArtistId() const;
        QString getArtistName() const;
        int getArtistSize() const;
        int getAlbumCount() const;
        int getTrackCount() const;
    private:
        mp3tunes_locker_artist_t *m_artist;
};

class Mp3tunesLockerAlbum {
    public:
        Mp3tunesLockerAlbum( mp3tunes_locker_album_t *album );
        ~Mp3tunesLockerAlbum();
        int getAlbumId() const;
        QString getAlbumTitle() const;
        int getArtistId() const;
        QString getArtistName() const;
        int getTrackCount() const;
        int getAlbumSize() const;
        bool hasArt() const;
    private:
        mp3tunes_locker_album_t *m_album;
};

class Mp3tunesLockerTrack {
    public:
        Mp3tunesLockerTrack( mp3tunes_locker_track_t *track );
        ~Mp3tunesLockerTrack();
        
        int getTrackId() const;
        QString getTrackTitle() const;
        int getTrackNumber() const;
        float getTrackLength() const;
        QString getTrackFileName() const;
        QString getTrackFileKey() const;
        int getTrackFileSize() const;
        QString getDownloadUrl() const;
        QString getPlayUrl() const;
        int getAlbumId() const;
        QString getAlbumTitle() const;
        int getAlbumYear() const;
        QString getArtistName() const;
    private:
        mp3tunes_locker_track_t *m_track;
};
#endif
