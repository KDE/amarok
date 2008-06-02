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
#ifndef MP3TUNESLOCKER_H
#define MP3TUNESLOCKER_H

extern "C" {
   // Get libmp3tunes declarations
    #include "libmp3tunes/locker.h"
}

#include "Mp3tunesLockerMeta.h"

#include <QString>
#include <QList>

/**
 * A wrapper class for the libmp3tunes locker object.
 * @author
 */
class Mp3tunesLocker {
    public:
        /**
         * Initialize the connection, but do not login.
         * @param partnerToken your partnerToken to access the mp3tunes API.
         */
        Mp3tunesLocker( QString partnerToken );
        
        /**
         * Initialize the connection, and login.
         * for testing use demo@mp3tunes.com:demo
         * @param partnerToken your partnerToken to access the mp3tunes API.
         * @param userName the username
         * @param password username's password
         */
        Mp3tunesLocker( QString partnerToken, QString userName, QString password );
        
        ~Mp3tunesLocker();
        
        /**
         * Logs into the locker.
         * for testing use demo@mp3tunes.com:demo
         * @param userName the username
         * @param passowrd username's password
         */
        void login(QString userName, QString password);

        /**
         * @return a list of all the artists in the locker
         */
        QList<Mp3tunesLockerArtist> artists();
        
        /**
         * @return a list of all the albums in the locker
         */
        QList<Mp3tunesLockerAlbum> albums();

        /**
         * @param artistId the id of the artist to list albums for
         * @return a QList of albums belonging to the supplied artist
         */
        QList<Mp3tunesLockerAlbum> albumsWithArtistId( int artistId);

        /**
         * @return a list of all the playlists in the locker
         */
        QList<Mp3tunesLockerPlaylist> playlists();

        /**
         * @return a list of all the tracks in the locker
         */
        QList<Mp3tunesLockerTrack> tracks();
        
         /**
          * @param playlistId a playlist id
          * @return a list of all the tracks with playlistId
          */
        QList<Mp3tunesLockerTrack> tracksWithPlaylistId( QString playlistId);

        /**
         * @param albumId an album id
         * @return a list of all the tracks with albumId
         */
        QList<Mp3tunesLockerTrack> tracksWithAlbumId( int albumId);

        /**
         * @param artistId an artist Id
         * @return a list of all the tracks with artistId
         */
        QList<Mp3tunesLockerTrack> tracksWithArtistId( int artistId);

        //TODO wrapper for mp3tunes_locker_generate_download_url_from_file_key
        //TODO wrapper for mp3tunes_locker_generate_download_url_from_file_key_and_bitrate
        //TODO wrapper for mp3tunes_locker_sync_down
        
        QString getUserName() const;
        QString getPassword() const;
        QString getSessionId() const;
        QString getFirstName() const;
        QString getLastName() const;
        QString getNickName() const;
        QString getPartnerToken() const;
        QString getServerApi() const;
        QString getServerContent() const;
        QString getServerLogin() const;

    private:
        mp3tunes_locker_object_t *mp3tunes_locker;
};
#endif
