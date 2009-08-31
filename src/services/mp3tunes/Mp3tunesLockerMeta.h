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

#ifndef MP3TUNESLOCKERMETA_H
#define MP3TUNESLOCKERMETA_H

/**
 * These are the c++ wrappers for the libmp3tunes meta types:
 * artist, album, track, playlist
 */

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

        QString playlistId() const;
        QString playlistTitle() const;
        QString title() const;
        QString fileName() const;
        int fileCount() const;
        int playlistSize() const;
    private:
        mp3tunes_locker_playlist_t *m_playlist;
};

class Mp3tunesLockerArtist {
    public:
        Mp3tunesLockerArtist( mp3tunes_locker_artist_t *artist );
        ~Mp3tunesLockerArtist();

        int artistId() const;
        QString artistName() const;
        int artistSize() const;
        int albumCount() const;
        int trackCount() const;
    private:
        int m_artistId;
        QString m_artistName;
        int m_artistSize;
        int m_albumCount;
        int m_trackCount;
};

class Mp3tunesLockerAlbum {
    public:
        Mp3tunesLockerAlbum( mp3tunes_locker_album_t *album );
        ~Mp3tunesLockerAlbum();
        int albumId() const;
        QString albumTitle() const;
        int artistId() const;
        QString artistName() const;
        int trackCount() const;
        int albumSize() const;
        bool hasArt() const;
    private:
        int m_albumId;
        QString m_albumTitle;
        int m_artistId;
        QString m_artistName;
        int m_trackCount;
        int m_albumSize;
        bool m_hasArt;
};

class Mp3tunesLockerTrack {
    public:
        Mp3tunesLockerTrack( mp3tunes_locker_track_t *track = 0 );
        ~Mp3tunesLockerTrack();
        
        int trackId() const;
        QString trackTitle() const;
        int trackNumber() const;
        float trackLength() const;
        QString trackFileName() const;
        QString trackFileKey() const;
        int trackFileSize() const;
        QString downloadUrl() const;
        QString playUrl() const;
        int albumId() const;
        QString albumTitle() const;
        int albumYear() const;
        QString artistName() const;
        int artistId() const;
    private:
        int m_trackId;
        QString m_trackTitle;
        int m_trackNumber;
        float m_trackLength;
        QString m_trackFileName;
        QString m_trackFileKey;
        int m_trackFileSize;
        QString m_downloadUrl;
        QString m_playUrl;
        int m_albumId;
        QString m_albumTitle;
        int m_albumYear;
        QString m_artistName;
        int m_artistId;
};
#endif
