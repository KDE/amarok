/****************************************************************************************
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
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
 * A psudeo type to encapsulate the various return types from a search query.
 */
class Mp3tunesSearchResult {
    public:
        enum SearchType {
            ArtistQuery = 1,
            AlbumQuery = 2,
            TrackQuery = 4
        };
        QList<Mp3tunesLockerArtist> artistList;
        QList<Mp3tunesLockerAlbum> albumList;
        QList<Mp3tunesLockerTrack> trackList;
        SearchType searchFor;
};

/**
 * A wrapper class for the libmp3tunes locker object.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesLocker {
    public:

        /**
         * Initialize the connection, but do not login.
         * @param partnerToken your partnerToken to access the mp3tunes API.
         */
        Mp3tunesLocker( const QString & partnerToken );

        /**
         * Initialize the connection, and login.
         * for testing use demo@mp3tunes.com:demo
         * @param partnerToken your partnerToken to access the mp3tunes API.
         * @param userName the username
         * @param password username's password
         */
        Mp3tunesLocker( const QString & partnerToken, const QString & userName, const QString & password );

        ~Mp3tunesLocker();

        /**
         * Logs into the locker.
         * for testing use demo@mp3tunes.com:demo
         * @param userName the username
         * @param passowrd username's password
         * @return if login successful, the sessionId is returned
         *         if login failed, an empty QString is returned
         */
        QString login( const QString & userName, const QString & password );

        /**
         * Logs into the locker using stored credentials.
         * Useful if a session is no longer valid.
         * @return if login successful, the sessionId is returned
         *         if login failed, an empty QString is returned
         */
         QString login();

        /**
         * Detects if a session has timed out.
         * @return true, if session is still valid.
         *         false, if session is invalid.
         */
        bool sessionValid() const;

        /**
         * @return a list of all the artists in the locker
         */
        QList<Mp3tunesLockerArtist> artists() const;

        /**
         * Searches the locker for artists containing the query.
         * @param query the string to search for
         * @return a list of all the artists in the locker containing the query
         */
        QList<Mp3tunesLockerArtist> artistsSearch( const QString &query ) const;

        /**
         * @return a list of all the albums in the locker
         */
        QList<Mp3tunesLockerAlbum> albums() const;

        /**
         * Searches the locker for albums containing the query.
         * @param query the string to search for
         * @return a list of all the albums in the locker containing the query
         */
        QList<Mp3tunesLockerAlbum> albumsSearch( const QString &query ) const;

        /**
         * @param artistId the id of the artist to list albums for
         * @return a QList of albums belonging to the supplied artist
         */
        QList<Mp3tunesLockerAlbum> albumsWithArtistId( int artistId ) const;

        /**
         * @return a list of all the playlists in the locker
         */
        QList<Mp3tunesLockerPlaylist> playlists() const;

        /**
         * @return a list of all the tracks in the locker
         */
        QList<Mp3tunesLockerTrack> tracks() const;

        /**
         * Searches the locker for tracks containing the query.
         * @param query the string to search for
         * @return a list of all the tracks in the locker containing the query
         */
        QList<Mp3tunesLockerTrack> tracksSearch( const QString &query ) const;

         /**
          * @param playlistId a playlist id
          * @return a list of all the tracks with playlistId
          */
        QList<Mp3tunesLockerTrack> tracksWithPlaylistId( const QString & playlistId) const;

        /**
         * @param albumId an album id
         * @return a list of all the tracks with albumId
         */
        QList<Mp3tunesLockerTrack> tracksWithAlbumId( int albumId ) const;

        /**
         * @param artistId an artist Id
         * @return a list of all the tracks with artistId
         */
        QList<Mp3tunesLockerTrack> tracksWithArtistId( int artistId ) const;

        /**
         * Get a list of tracks from a list of filekeys.
         * @param filekeys a list of filekeys
         * @return a list of tracks that match the supplied filekeys. The returned list
         *         of tracks be in the same order as the supplied filekeys.
         *         Might return an empty List.
         */
        QList<Mp3tunesLockerTrack> tracksWithFileKeys( QStringList filekeys ) const;

        /**
         * Get a single track from a single filekey.
         * @param filekey the filekey to match to a track
         * @return the track that matches the filekey.
         */
        Mp3tunesLockerTrack trackWithFileKey( const QString &filekey ) const;

        /**
         * Searches the Locker for tracks, albums, and/or artists.
         * Which type it searches depends on the Mp3tunesSearchResult's
         * searchFor passed to it.
         * @pre Mp3tunesSearchResult's fields are initialized properly
         *      depending on the types (artist, track album) you want searched.
         *      Properly means: If you want a type searched initialize an empty
         *      QList and set the appropriate SearchType bit.
         * @post The Mp3tunesSearchResult's fields will filled with the
         *      search results. If the search returned empty, then the
         *      QLists will be empty.
         * @param container contains the QList's to be filled with search results
         * @return true if search succeeded. Note: Zero search results is not failure.
         *         false if search failed.
         */
        bool search( Mp3tunesSearchResult &container, const QString &query ) const;

        /**
         * Uploads a track to the locker using HTTP Put.
         * @param path the absolute filename of the track to upload
         * @return true if upload succeeded
         *         false if upload failed
         * @pre The track exists and is a filetype that mp3tunes accepts.
         */
        bool uploadTrack( const QString &path );

        /**
         * Generates the file key identifier for a media file.
         * @param path the absolute filename of the track
         * @return the file key
         * @pre The track exists and is a filetype that mp3tunes accepts.
         */
        QString fileKey(const QString &path );

        /**
         * Loads a URL into the locker via the LockerLoad API.
         * The URL may be password protected, as long as the username
         * and password can be transferred in the URL to retrieve the
         * file. Any access control based on user sessions cannot be
         * used in locker loading.
         * @param the full URL to the track
         * @return true if the lockerload succeeded
         *         false if the lockerload failed.
         * @pre The URL must be accessible by the MP3tunes servers,
         *      it is assumed that all the tracks in the file are
         *      non-DRM music files.
         */
        bool lockerLoad( const QString &url );
        //TODO wrapper for mp3tunes_locker_generate_download_url_from_file_key
        //TODO wrapper for mp3tunes_locker_generate_download_url_from_file_key_and_bitrate
        //TODO wrapper for mp3tunes_locker_sync_down

        QString userName() const;
        QString password() const;
        QString sessionId() const;
        QString firstName() const;
        QString lastName() const;
        QString nickName() const;
        QString partnerToken() const;
        QString serverApi() const;
        QString serverContent() const;
        QString serverLogin() const;
        QString errorMessage() const;
        bool authenticated() const;
    private:
        mp3tunes_locker_object_t *m_locker;
};
#endif
