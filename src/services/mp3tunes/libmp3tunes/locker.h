/****************************************************************************************
 * Copyright (c) 2008 MP3tunes, LLC <copyright@mp3tunes.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU Library General Public License as published by the Free         *
 * Software Foundation; either version 2.1 of the License, or (at your option) any      *
 * later version.                                                                       *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU Library General Public License along with *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/**
 * \file oboe.h
 * \brief The \e liboboe public header.
 */

#ifndef __MP3TUNES_LOCKER_H__
#define __MP3TUNES_LOCKER_H__

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define MP3TUNES_SERVER_API_URL "ws.mp3tunes.com"
#define MP3TUNES_SERVER_CONTENT_URL "content.mp3tunes.com"
#define MP3TUNES_SERVER_LOGIN_URL "shop.mp3tunes.com"

#define MP3TUNES_SERVER_API 0
#define MP3TUNES_SERVER_CONTENT 1
#define MP3TUNES_SERVER_LOGIN 2

typedef struct {
  char *username, *password, *session_id, *firstname, *lastname, *nickname;
  char *partner_token;
  char *server_api, *server_content, *server_login;
  char *error_message;
} mp3tunes_locker_object_t;

struct mp3tunes_locker_list_item_s {
    int id;
    void *value;
    struct mp3tunes_locker_list_item_s *prev;
    struct mp3tunes_locker_list_item_s *next;
};

typedef struct mp3tunes_locker_list_item_s mp3tunes_locker_list_item_t;

struct mp3tunes_locker_list_s {
    int last_id;
    mp3tunes_locker_list_item_t *first;
    mp3tunes_locker_list_item_t *last;
};

typedef struct mp3tunes_locker_list_s mp3tunes_locker_track_list_t;
typedef struct mp3tunes_locker_list_s mp3tunes_locker_artist_list_t;
typedef struct mp3tunes_locker_list_s mp3tunes_locker_album_list_t;
typedef struct mp3tunes_locker_list_s mp3tunes_locker_playlist_list_t;

typedef struct {
    int trackId;
    char *trackTitle;
    int trackNumber;
    float trackLength;
    char *trackFileName;
    char *trackFileKey;
    int trackFileSize;
    char *downloadURL;
    char *playURL;
    int albumId;
    char *albumTitle;
    int albumYear;
    char *artistName;
    int artistId;
} mp3tunes_locker_track_t;

typedef struct {
    int artistId;
    char* artistName;
    int artistSize;
    int albumCount;
    int trackCount;
} mp3tunes_locker_artist_t;

typedef struct {
    int albumId;
    char *albumTitle;
    int artistId;
    char *artistName;
    int trackCount;
    int albumSize;
    int hasArt;
} mp3tunes_locker_album_t;

typedef struct {
    char* playlistId;
    char* playlistTitle;
    char* title;
    char* fileName;
    int fileCount;
    int playlistSize;
} mp3tunes_locker_playlist_t;

int mp3tunes_locker_init( mp3tunes_locker_object_t **obj, const char *partner_token );
int mp3tunes_locker_deinit( mp3tunes_locker_object_t **obj );
int mp3tunes_locker_login( mp3tunes_locker_object_t *obj, const char* username, const char* password );
int mp3tunes_locker_session_valid( mp3tunes_locker_object_t *obj );
int mp3tunes_locker_artists( mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists_return);
int mp3tunes_locker_artists_search( mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists_return, char *query);
int mp3tunes_locker_albums_with_artist_id( mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums_return, int artist_id);
int mp3tunes_locker_albums( mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums_return);
int mp3tunes_locker_albums_search(  mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums_return, char *query);
int mp3tunes_locker_playlists(mp3tunes_locker_object_t *obj, mp3tunes_locker_playlist_list_t **playlist_return);
int mp3tunes_locker_search(mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists_return, mp3tunes_locker_album_list_t **albums_return, mp3tunes_locker_track_list_t **tracks_return, const char *query);

int mp3tunes_locker_tracks( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks_return);
int mp3tunes_locker_tracks_search( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks_return, char *query);
int mp3tunes_locker_tracks_with_playlist_id( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks_return, const char* playlist_id);
int mp3tunes_locker_tracks_with_album_id( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks_return, int album_id);
int mp3tunes_locker_tracks_with_artist_id( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks_return, int artist_id);
int mp3tunes_locker_tracks_with_file_key( mp3tunes_locker_object_t *obj, const char *file_keys, mp3tunes_locker_track_list_t **tracks );
int mp3tunes_locker_track_with_file_key( mp3tunes_locker_object_t *obj, const char *file_key, mp3tunes_locker_track_t **track );

int mp3tunes_locker_track_list_deinit( mp3tunes_locker_track_list_t** list );
int mp3tunes_locker_artist_list_deinit( mp3tunes_locker_track_list_t** list );
int mp3tunes_locker_album_list_deinit( mp3tunes_locker_track_list_t** list );
int mp3tunes_locker_playlist_list_deinit( mp3tunes_locker_track_list_t** list );

char* mp3tunes_locker_generate_download_url_from_file_key(mp3tunes_locker_object_t *obj, char *file_key);
char* mp3tunes_locker_generate_download_url_from_file_key_and_bitrate(mp3tunes_locker_object_t *obj, char *file_key, char* bitrate);

char* mp3tunes_locker_generate_filekey(const char *filename);
int mp3tunes_locker_upload_track(mp3tunes_locker_object_t *obj, const char *path);
int mp3tunes_locker_load_track(mp3tunes_locker_object_t *obj, const char *url);

int mp3tunes_locker_sync_down(mp3tunes_locker_object_t *obj, char* type, char* bytes_local, char* files_local, char* keep_local_files, char* playlist_id);
#endif
