 /*
 * Copyright (C) 2008 MP3tunes, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include "locker.h"
#include "md5.h"

typedef struct {
    char *data;
    size_t size;
    int offset;
} chunk_t;

typedef struct {
    CURL *curl;
    char *url;
} request_t;

void chunk_init(chunk_t** chunk) {
    chunk_t *c = *chunk = (chunk_t*)malloc(sizeof(chunk_t));
    c->data = NULL;
    c->size = 0;
}

void chunk_set_data(chunk_t* chunk, char* data) {
    chunk->data = data;
    chunk->size = strlen(data);
}

void chunk_deinit(chunk_t** chunk) {
    chunk_t *c = *chunk;
    free(c->data);
    free(c);
}

struct xml_xpath_s {
    xmlDocPtr document;
    xmlXPathContextPtr xpath_ctx;
    xmlNodePtr context;
};

typedef struct xml_xpath_s xml_xpath_t;

size_t write_chunk_callback( void *ptr, size_t size, size_t nmemb, void *data ) {
    size_t realsize = size * nmemb;
    chunk_t *chunk = (chunk_t *)data;
    chunk->data = (char *)realloc( chunk->data, chunk->size + realsize + 1 );
    if( chunk->data != NULL ) {
        memcpy( &(chunk->data[ chunk->size ]), ptr, realsize );
        chunk->size += realsize;
        chunk->data[ chunk->size ] = 0;
    }

    return realsize;
}

xml_xpath_t* xml_xpath_init(xmlDocPtr document) {
    xml_xpath_t *result = malloc(sizeof(xml_xpath_t));
    if (result == NULL)
        return NULL;

    result->document = document;
    result->xpath_ctx = xmlXPathNewContext(result->document);
    if(result->xpath_ctx == NULL) {
        xmlFreeDoc(result->document);
        free(result);
        return NULL;
    }
    result->context = NULL;

    return result;
}

xml_xpath_t* xml_xpath_context_init(xml_xpath_t* xml_xpath, xmlNodePtr node) {
    xml_xpath_t *result = malloc(sizeof(xml_xpath_t));
    if (result == NULL)
        return NULL;

    result->document = xml_xpath->document;
    result->xpath_ctx = xmlXPathNewContext(result->document);
    if(result->xpath_ctx == NULL) {
        xmlFreeDoc(result->document);
        free(result);
        return NULL;
    }
    result->xpath_ctx->node = node;
    result->context = node;

    return result;
}

void xml_xpath_deinit(xml_xpath_t* xml_xpath) {
    xmlXPathFreeContext(xml_xpath->xpath_ctx);
    if (xml_xpath->context == NULL) {
        xmlFreeDoc(xml_xpath->document);
    }
    free(xml_xpath);
}

xmlXPathObjectPtr xml_xpath_query(xml_xpath_t *xml_xpath, const char* xpath_expression) {
    xmlXPathObjectPtr xpath_obj;

    xpath_obj = xmlXPathEvalExpression((xmlChar*)xpath_expression, xml_xpath->xpath_ctx);
    if (xpath_obj == NULL) {
        return NULL;
    }
    if (xpath_obj->type != XPATH_NODESET) {
        xmlXPathFreeObject(xpath_obj);
        return NULL;
    }
    return xpath_obj;
}

char* xml_get_text_from_nodeset(xmlNodeSetPtr nodeset) {
    xmlNodePtr node;
    xmlNodePtr child;
    int total_nodes;
    char* result = NULL;
    total_nodes = (nodeset) ? nodeset->nodeNr : 0;

    if (total_nodes != 1) {
        return NULL;
    }

    if (nodeset->nodeTab[0]->type != XML_ELEMENT_NODE) {
        return NULL;
    }

    node = nodeset->nodeTab[0];
    child = node->children;
    while (child && (XML_TEXT_NODE != child->type))
        child = child->next;
    if (child && (XML_TEXT_NODE == child->type)) {
        result = strdup((char*)child->content);
    }
    return result;
}

static char* xml_xpath_get_string(xml_xpath_t *xml_xpath, const char* xpath_expression) {
    xmlXPathObjectPtr xpath_obj;
    char* result = NULL;

    xpath_obj = xml_xpath_query(xml_xpath, xpath_expression);

    result = xml_get_text_from_nodeset(xpath_obj->nodesetval);

    xmlXPathFreeObject(xpath_obj);

    return result;
}

int xml_xpath_get_integer(xml_xpath_t *xml_xpath, const char* xpath_expression) {
    int result = 0;
    char* str = xml_xpath_get_string(xml_xpath, xpath_expression);
    if (str != NULL) {
        result = atoi(str);
    }
    free(str);
    return result;
}

float xml_xpath_get_float(xml_xpath_t *xml_xpath, const char* xpath_expression) {
    float result = 0.0;
    char* str = xml_xpath_get_string(xml_xpath, xpath_expression);
    if (str != NULL) {
        result = atof(str);
    }
    free(str);
    return result;
}

int mp3tunes_locker_init( mp3tunes_locker_object_t **obj, const char *partner_token ) {
    mp3tunes_locker_object_t *o = *obj = (mp3tunes_locker_object_t*)malloc(sizeof(mp3tunes_locker_object_t));
    memset(o, 0, sizeof(*o));

    o->partner_token = strdup(partner_token);
    o->session_id = NULL;
    o->error_message = NULL;

    o->server_api = getenv("MP3TUNES_SERVER_API");
    if(o->server_api == NULL) {
        o->server_api = strdup(MP3TUNES_SERVER_API_URL);
    }

    o->server_content = getenv("MP3TUNES_SERVER_CONTENT");
    if(o->server_content == NULL) {
        o->server_content = strdup(MP3TUNES_SERVER_CONTENT_URL);
    }

    o->server_login = getenv("MP3TUNES_SERVER_LOGIN");
    if(o->server_login == NULL) {
        o->server_login = strdup(MP3TUNES_SERVER_LOGIN_URL);
    }

    return TRUE;
}

int mp3tunes_locker_deinit( mp3tunes_locker_object_t **obj ) {
    mp3tunes_locker_object_t *o = *obj;
    free(o->partner_token);
    free(o->session_id);
    free(o->error_message);
    free(o);
    return TRUE;
}

void mp3tunes_request_init(request_t **request) {
    request_t *r = *request = malloc(sizeof(request_t));
    r->curl = curl_easy_init();
    r->url = NULL;
}

void mp3tunes_request_deinit(request_t **request) {
    request_t *r = *request;
    curl_easy_cleanup(r->curl);
    free(r->url);
    free(r);
}

static request_t* mp3tunes_locker_api_generate_request_valist(mp3tunes_locker_object_t *obj, int server, const char* path, const char* first_name, va_list argp) {
    request_t *request;
    char *server_url;
    char *name, *value;
    char *encoded_name, *encoded_value;

    mp3tunes_request_init(&request);

    switch (server) {
        case MP3TUNES_SERVER_LOGIN:
            server_url = obj->server_login;
            break;
        case MP3TUNES_SERVER_CONTENT:
            server_url = obj->server_content;
            break;
        case MP3TUNES_SERVER_API:
            server_url = obj->server_api;
            break;
        default:
            mp3tunes_request_deinit(&request);
            return NULL;
            break;
    }

    char *url = 0;
    size_t url_size = asprintf(&url, "http://%s/%s?", server_url, path) +1;
    name = (char*) first_name;
    while (name) {
        char *url_part;

        value = va_arg(argp, char*);

        encoded_name = curl_easy_escape(request->curl, name, 0);
        encoded_value = curl_easy_escape(request->curl, value, 0);
        size_t url_part_size = asprintf(&url_part, "%s=%s&", encoded_name, encoded_value);
        curl_free(encoded_name);
        curl_free(encoded_value);

	url = realloc(url, url_size += url_part_size);
        strcat(url, url_part);

        name = va_arg(argp, char*);
    }

    char *end_url_part = NULL;
    size_t end_url_part_size = 0;
    if (server != MP3TUNES_SERVER_LOGIN) {
        if (obj->session_id != NULL) {
            if (server == MP3TUNES_SERVER_API) {
                end_url_part_size = asprintf(&end_url_part, "output=xml&sid=%s&partner_token=%s", obj->session_id, obj->partner_token);
            } else {
                end_url_part_size = asprintf(&end_url_part, "sid=%s&partner_token=%s", obj->session_id, obj->partner_token);
            }
        } else {
            printf("Failed because of no session id\n");
            free(url);
            mp3tunes_request_deinit(&request);
            return NULL;
        }
    } else {
        end_url_part_size = asprintf(&end_url_part, "output=xml&partner_token=%s", obj->partner_token);
    }
    url = realloc(url, url_size += end_url_part_size);
    strcat(url, end_url_part);

    request->url = url;
    return request;

}

static request_t* mp3tunes_locker_api_generate_request(mp3tunes_locker_object_t *obj, int server, char* path, const char* first_name, ...) {
    va_list argp;
    request_t *request;
    va_start(argp, first_name);
    request = mp3tunes_locker_api_generate_request_valist(obj, server, path, first_name, argp);
    va_end(argp);
    return request;
}

static xml_xpath_t* mp3tunes_locker_api_simple_fetch(mp3tunes_locker_object_t *obj, int server, const char* path, const char* first_name, ...) {
    request_t *request;
    CURLcode res;
    chunk_t *chunk;
    va_list argp;

    chunk_init(&chunk);

    va_start(argp, first_name);

    request = mp3tunes_locker_api_generate_request_valist(obj, server, path, first_name, argp);

    va_end(argp);

    if (request == NULL) {
        chunk_deinit(&chunk);
        return NULL;
    }

    curl_easy_setopt( request->curl, CURLOPT_URL, request->url );
    curl_easy_setopt( request->curl, CURLOPT_WRITEFUNCTION, write_chunk_callback );
    curl_easy_setopt( request->curl, CURLOPT_WRITEDATA, (void *)chunk );
    curl_easy_setopt( request->curl, CURLOPT_USERAGENT, "liboboe/1.0" );
    curl_easy_setopt( request->curl, CURLOPT_NOPROGRESS, 1 );

    res = curl_easy_perform(request->curl);
    curl_easy_cleanup(request->curl);
    free(request);

    if (res != CURLE_OK) {
        chunk_deinit(&chunk);
        return NULL;
    }

    if (chunk->data == NULL) {
        return NULL;
    }

    /*printf("Fetch result:\n%s\n", chunk->data);*/

    xmlDocPtr document = xmlParseDoc((xmlChar*)chunk->data);

    chunk_deinit(&chunk);

    if (document == NULL) {
        return NULL;
    }

    return xml_xpath_init(document);
}

static xml_xpath_t* mp3tunes_locker_api_post_fetch(mp3tunes_locker_object_t *obj, int server, const char* path, char* post_data) {
    request_t *request;
    CURLcode res;
    chunk_t *chunk;

    chunk_init(&chunk);

    request = mp3tunes_locker_api_generate_request(obj, server, path, NULL);
    if (request == NULL) {
        chunk_deinit(&chunk);
        return NULL;
    }

    curl_easy_setopt( request->curl, CURLOPT_URL, request->url );
    curl_easy_setopt( request->curl, CURLOPT_WRITEFUNCTION, write_chunk_callback );
    curl_easy_setopt( request->curl, CURLOPT_WRITEDATA, (void *)chunk );
    curl_easy_setopt( request->curl, CURLOPT_USERAGENT, "liboboe/1.0" );
    curl_easy_setopt( request->curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt( request->curl, CURLOPT_NOPROGRESS, 1 );

    res = curl_easy_perform(request->curl);
    curl_easy_cleanup(request->curl);
    free(request);

    if (res != CURLE_OK) {
        chunk_deinit(&chunk);
        return NULL;
    }

    if (chunk->data == NULL) {
        return NULL;
    }

    printf("Fetch result:\n%s\n", chunk->data);

    xmlDocPtr document = xmlParseDoc((xmlChar*)chunk->data);

    chunk_deinit(&chunk);

    if (document == NULL) {
        return NULL;
    }

    return xml_xpath_init(document);
}

char* mp3tunes_locker_generate_download_url_from_file_key(mp3tunes_locker_object_t *obj, char *file_key) {
    request_t *request;
    char *path = malloc(256*sizeof(char));
    char *ret;
    snprintf(path, 256, "storage/lockerget/%s", file_key);
    request = mp3tunes_locker_api_generate_request(obj, MP3TUNES_SERVER_CONTENT, path, NULL);
    ret = request->url; request->url = NULL;
    free(path);
    mp3tunes_request_deinit(&request);
    return ret;
}

char* mp3tunes_locker_generate_download_url_from_file_key_and_bitrate(mp3tunes_locker_object_t *obj, char *file_key, char* bitrate) {
    request_t *request;
    char *path = malloc(256*sizeof(char));
    char *ret;
    snprintf(path, 256, "storage/lockerget/%s", file_key);
    request = mp3tunes_locker_api_generate_request(obj, MP3TUNES_SERVER_CONTENT, path, "bitrate", bitrate, NULL);
    ret = request->url; request->url = NULL;
    free(path);
    mp3tunes_request_deinit(&request);
    return ret;
}


int mp3tunes_locker_login(mp3tunes_locker_object_t *obj, const char* username, const char* password) {
    xml_xpath_t* xml_xpath;
    char *status, *session_id;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_LOGIN, "api/v1/login/", "username", username, "password", password, NULL);

    if (xml_xpath == NULL) {
        return -2;
    }

    status = xml_xpath_get_string(xml_xpath, "/mp3tunes/status");

    if (status[0] != '1') {
      /*printf("status is %s\n", status);*/
        char* error = xml_xpath_get_string(xml_xpath, "/mp3tunes/errorMessage");
        /*printf("error is %s\n", error);*/
        obj->error_message = error;
        free(status);
        xml_xpath_deinit(xml_xpath);
        return -1;
    }
    free(status);

    session_id = xml_xpath_get_string(xml_xpath, "/mp3tunes/session_id");
    obj->username = strdup(username);
    obj->password = strdup(password);
    obj->session_id = session_id;
    xml_xpath_deinit(xml_xpath);

    return 0;
}

int mp3tunes_locker_session_valid(mp3tunes_locker_object_t *obj) {

    request_t *request;
    CURLcode res;
    chunk_t *chunk;

    chunk_init(&chunk);

    request = mp3tunes_locker_api_generate_request(obj, MP3TUNES_SERVER_API, "api/v1/accountData", NULL);
    if (request == NULL) {
        chunk_deinit(&chunk);
        return -1;
    }

    curl_easy_setopt( request->curl, CURLOPT_URL, request->url );
    curl_easy_setopt( request->curl, CURLOPT_WRITEFUNCTION, write_chunk_callback );
    curl_easy_setopt( request->curl, CURLOPT_WRITEDATA, (void *)chunk );
    curl_easy_setopt( request->curl, CURLOPT_NOBODY, 1 );
    curl_easy_setopt( request->curl, CURLOPT_USERAGENT, "liboboe/1.0" );
    curl_easy_setopt( request->curl, CURLOPT_HEADER, 1 );
    curl_easy_setopt( request->curl, CURLOPT_NOPROGRESS, 1 );

    res = curl_easy_perform(request->curl);
    curl_easy_cleanup(request->curl);
    free(request);

    if (res != CURLE_OK) {
        chunk_deinit(&chunk);
        return -1;
    }

    if (chunk->data == NULL) {
        return -1;
    }

    char name[] = "X-MP3tunes-ErrorNo";
    char value[] = "401001";
    char *result = strstr (chunk->data, name);
    if (result != 0)
    {
        int i = strcspn(result, "\n");
        char *result1 = malloc(i + 1);
        if (result1 == NULL)
            return -1;

        strncpy(result1, result, i);
        /*printf("Header String: %s\n", result1);*/
        result = strstr(result1, value);
        free(result1);
        if (result != 0) /* i.e., value could not be located hence there is no 404 error. */
            return -1; /* session is invalid*/
    }

    /*printf("Fetch result:\n%s\n", chunk->data);*/
    return 0; /* session is valid*/
}

int mp3tunes_locker_list_init(struct mp3tunes_locker_list_s **list) {
    struct mp3tunes_locker_list_s *l = *list = (struct mp3tunes_locker_list_s*)malloc(sizeof(struct mp3tunes_locker_list_s));
    l->last_id = 0;
    l->first = l->last = NULL;
    return 0;
}

int mp3tunes_locker_track_list_init(mp3tunes_locker_track_list_t **list) {
    return mp3tunes_locker_list_init((struct mp3tunes_locker_list_s**)list);
}

int mp3tunes_locker_artist_list_init(mp3tunes_locker_artist_list_t **list) {
    return mp3tunes_locker_list_init((struct mp3tunes_locker_list_s**)list);
}

int mp3tunes_locker_album_list_init(mp3tunes_locker_album_list_t **list) {
    return mp3tunes_locker_list_init((struct mp3tunes_locker_list_s**)list);
}

int mp3tunes_locker_playlist_list_init(mp3tunes_locker_playlist_list_t **list) {
    return mp3tunes_locker_list_init((struct mp3tunes_locker_list_s**)list);
}

int mp3tunes_locker_list_add(struct mp3tunes_locker_list_s **list, void* value) {
    struct mp3tunes_locker_list_s *l = *list;
    mp3tunes_locker_list_item_t *item = (mp3tunes_locker_list_item_t*)malloc(sizeof(mp3tunes_locker_list_item_t));
    item->id = l->last_id++;
    item->prev = l->last;
    item->next = NULL;
    item->value = value;

    if (l->first) {
        l->last = item->prev->next = item;
    } else {
        l->first = l->last = item;
    }

    return 0;
}

int mp3tunes_locker_track_list_add(mp3tunes_locker_track_list_t **list, mp3tunes_locker_track_t *track) {
    return mp3tunes_locker_list_add((struct mp3tunes_locker_list_s**)list, (void*)track);
}

int mp3tunes_locker_artist_list_add(mp3tunes_locker_artist_list_t **list, mp3tunes_locker_artist_t *artist) {
    return mp3tunes_locker_list_add((struct mp3tunes_locker_list_s**)list, (void*)artist);
}

int mp3tunes_locker_album_list_add(mp3tunes_locker_album_list_t **list, mp3tunes_locker_album_t *album) {
    return mp3tunes_locker_list_add((struct mp3tunes_locker_list_s**)list, (void*)album);
}

int mp3tunes_locker_playlist_list_add(mp3tunes_locker_playlist_list_t **list, mp3tunes_locker_playlist_t *album) {
    return mp3tunes_locker_list_add((struct mp3tunes_locker_list_s**)list, (void*)album);
}

int mp3tunes_locker_list_deinit(struct mp3tunes_locker_list_s **list) {
    struct mp3tunes_locker_list_s *l = *list;
    mp3tunes_locker_list_item_t *list_item = l->first;
    if (l) {
        while(l->first) {
            list_item = l->first->next;
            free(l->first);
            l->first = list_item;
        }
        free(l);
        return 0;
    }
    return -1;
}

int mp3tunes_locker_track_list_deinit(mp3tunes_locker_track_list_t **track_list) {
    mp3tunes_locker_track_list_t *list = *track_list;
    mp3tunes_locker_list_item_t *track_item = list->first;
    mp3tunes_locker_track_t *track;

    while (track_item != NULL) {
        track = (mp3tunes_locker_track_t*)track_item->value;
        free(track->trackTitle);
        free(track->trackFileName);
        free(track->trackFileKey);
        free(track->downloadURL);
        free(track->playURL);
        free(track->albumTitle);
        free(track->artistName);

        free(track);
        track_item = track_item->next;
    }
    return mp3tunes_locker_list_deinit((struct mp3tunes_locker_list_s**)track_list);
}

int mp3tunes_locker_artist_list_deinit(mp3tunes_locker_artist_list_t **artist_list) {
    mp3tunes_locker_artist_list_t *list = *artist_list;
    mp3tunes_locker_list_item_t *artist_item = list->first;
    mp3tunes_locker_artist_t *artist;

    while (artist_item != NULL) {
        artist = (mp3tunes_locker_artist_t*)artist_item->value;
        free(artist->artistName);

        free(artist);
        artist_item = artist_item->next;
    }
    return mp3tunes_locker_list_deinit((struct mp3tunes_locker_list_s**)artist_list);
}

int mp3tunes_locker_album_list_deinit(mp3tunes_locker_album_list_t **album_list) {
    mp3tunes_locker_album_list_t *list = *album_list;
    mp3tunes_locker_list_item_t *album_item = list->first;
    mp3tunes_locker_album_t *album;

    while (album_item != NULL) {
        album = (mp3tunes_locker_album_t*)album_item->value;
        free(album->albumTitle);
        free(album->artistName);

        free(album);
        album_item = album_item->next;
    }
    return mp3tunes_locker_list_deinit((struct mp3tunes_locker_list_s**)album_list);
}

int mp3tunes_locker_playlist_list_deinit(mp3tunes_locker_playlist_list_t **playlist_list) {
    mp3tunes_locker_playlist_list_t *list = *playlist_list;
    mp3tunes_locker_list_item_t *playlist_item = list->first;
    mp3tunes_locker_playlist_t *playlist;

    while (playlist_item != NULL) {
        playlist = (mp3tunes_locker_playlist_t*)playlist_item->value;
        free(playlist->playlistId);
        free(playlist->playlistTitle);
        free(playlist->title);
        free(playlist->fileName);

        free(playlist);
        playlist_item = playlist_item->next;
    }
    return mp3tunes_locker_list_deinit((struct mp3tunes_locker_list_s**)playlist_list);
}

static int _mp3tunes_locker_tracks(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, int artist_id, int album_id, const char* playlist_id) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;
    char artist_id_s[15];
    char album_id_s[15];

    if (playlist_id == NULL) {
        if (artist_id == -1 && album_id == -1) {
            xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", NULL);
        } else if (artist_id != -1 && album_id == -1) {
            snprintf(artist_id_s, 15, "%d", artist_id);
            xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "artist_id", artist_id_s, NULL);
        } else if (artist_id == -1 && album_id != -1) {
            snprintf(album_id_s, 15, "%d", album_id);
            xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "album_id", album_id_s, NULL);
        } else {
            snprintf(artist_id_s, 15, "%d", artist_id);
            snprintf(album_id_s, 15, "%d", album_id);
            xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "artist_id", artist_id_s, "album_id", album_id_s, NULL);
        }
    } else {
        xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "playlist_id", playlist_id, NULL);
    }

    mp3tunes_locker_track_list_init(tracks);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_track_t *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));
        memset(track, 0, sizeof(mp3tunes_locker_track_t));

        track->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
        track->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
        track->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
        track->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
        track->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
        track->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
        track->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
        track->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
        track->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
        track->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        track->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        track->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
        track->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        track->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

        mp3tunes_locker_track_list_add(tracks, track);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}

int mp3tunes_locker_tracks_search( mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, char *search) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerSearch", "type", "track", "s", search, NULL);

    mp3tunes_locker_track_list_init(tracks);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_track_t *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));
        memset(track, 0, sizeof(mp3tunes_locker_track_t));

        track->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
        track->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
        track->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
        track->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
        track->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
        track->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
        track->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
        track->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
        track->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
        track->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        track->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        track->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
        track->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        track->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

        mp3tunes_locker_track_list_add(tracks, track);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}


int mp3tunes_locker_tracks(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks) {
    return _mp3tunes_locker_tracks(obj, tracks, -1, -1, NULL);
}

int mp3tunes_locker_tracks_with_artist_id(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, int artist_id) {
    return _mp3tunes_locker_tracks(obj, tracks, artist_id, -1, NULL);
}

int mp3tunes_locker_tracks_with_album_id(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, int album_id) {
    return _mp3tunes_locker_tracks(obj, tracks, -1, album_id, NULL);
}

int mp3tunes_locker_tracks_with_artist_id_and_album_id(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, int artist_id, int album_id) {
    return _mp3tunes_locker_tracks(obj, tracks, artist_id, album_id, NULL);
}

int mp3tunes_locker_tracks_with_playlist_id(mp3tunes_locker_object_t *obj, mp3tunes_locker_track_list_t **tracks, const char* playlist_id) {
    return _mp3tunes_locker_tracks(obj, tracks, -1, -1, playlist_id);
}

int mp3tunes_locker_tracks_with_file_key( mp3tunes_locker_object_t *obj, const char *file_keys, mp3tunes_locker_track_list_t **tracks ) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "key", file_keys, NULL);

    mp3tunes_locker_track_list_init(tracks);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_track_t *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));
        memset(track, 0, sizeof(mp3tunes_locker_track_t));

        track->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
        track->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
        track->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
        track->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
        track->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
        track->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
        track->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
        track->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
        track->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
        track->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        track->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        track->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
        track->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        track->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

        mp3tunes_locker_track_list_add(tracks, track);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;   

}

int mp3tunes_locker_track_with_file_key( mp3tunes_locker_object_t *obj, const char *file_key, mp3tunes_locker_track_t **track ) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "key", file_key, NULL);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;
    if ( nodeset->nodeNr == 1) {
        node = nodeset->nodeTab[0];

        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_track_t *t = *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));

        t->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
        t->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
        t->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
        t->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
        t->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
        t->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
        t->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
        t->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
        t->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
        t->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        t->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        t->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
        t->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        t->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

        xml_xpath_deinit(xml_xpath_context);
        xmlXPathFreeObject(xpath_obj);
        xml_xpath_deinit(xml_xpath);
        return 0;
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return -1;   
}

int mp3tunes_locker_artists(mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "artist", NULL);

    mp3tunes_locker_artist_list_init(artists);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/artistList/item");
    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_artist_t *artist = (mp3tunes_locker_artist_t*)malloc(sizeof(mp3tunes_locker_artist_t));
        memset(artist, 0, sizeof(mp3tunes_locker_artist_t));

        artist->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
        artist->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        artist->artistSize = xml_xpath_get_integer(xml_xpath_context, "artistSize");
        artist->albumCount = xml_xpath_get_integer(xml_xpath_context, "albumCount");
        artist->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");

        mp3tunes_locker_artist_list_add(artists, artist);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}

int mp3tunes_locker_artists_search( mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists, char *search) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerSearch", "type", "artist", "s", search, NULL);
    mp3tunes_locker_artist_list_init(artists);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/artistList/item");
    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_artist_t *artist = (mp3tunes_locker_artist_t*)malloc(sizeof(mp3tunes_locker_artist_t));
        memset(artist, 0, sizeof(mp3tunes_locker_artist_t));

        artist->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
        artist->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        artist->artistSize = xml_xpath_get_integer(xml_xpath_context, "artistSize");
        artist->albumCount = xml_xpath_get_integer(xml_xpath_context, "albumCount");
        artist->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");

        mp3tunes_locker_artist_list_add(artists, artist);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}


int mp3tunes_locker_albums_with_artist_id(mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums, int artist_id) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;
    char artist_id_string[15];

    if (artist_id == -1) {
        xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "album", NULL);
    } else {
        snprintf(artist_id_string, 15, "%d", artist_id);
        xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "album", "artist_id", artist_id_string, NULL);
    }

    mp3tunes_locker_album_list_init(albums);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/albumList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_album_t *album = (mp3tunes_locker_album_t*)malloc(sizeof(mp3tunes_locker_album_t));
        memset(album, 0, sizeof(mp3tunes_locker_album_t));

        album->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        album->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        album->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
        album->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        album->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");
        album->albumSize = xml_xpath_get_integer(xml_xpath_context, "albumSize");
        album->hasArt = xml_xpath_get_integer(xml_xpath_context, "hasArt");

        mp3tunes_locker_album_list_add(albums, album);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}

int mp3tunes_locker_albums(mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums) {
    return mp3tunes_locker_albums_with_artist_id(obj, albums, -1);
}

int mp3tunes_locker_albums_search(  mp3tunes_locker_object_t *obj, mp3tunes_locker_album_list_t **albums, char *search) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerSearch", "type", "album", "s", search, NULL);

    mp3tunes_locker_album_list_init(albums);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/albumList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_album_t *album = (mp3tunes_locker_album_t*)malloc(sizeof(mp3tunes_locker_album_t));
        memset(album, 0, sizeof(mp3tunes_locker_album_t));

        album->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        album->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        album->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
        album->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        album->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");
        album->albumSize = xml_xpath_get_integer(xml_xpath_context, "albumSize");
        album->hasArt = xml_xpath_get_integer(xml_xpath_context, "hasArt");

        mp3tunes_locker_album_list_add(albums, album);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}




int mp3tunes_locker_playlists(mp3tunes_locker_object_t *obj, mp3tunes_locker_playlist_list_t **playlists) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "playlist", NULL);

    mp3tunes_locker_playlist_list_init(playlists);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj =xml_xpath_query(xml_xpath, "/mp3tunes/playlistList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_playlist_t *playlist = (mp3tunes_locker_playlist_t*)malloc(sizeof(mp3tunes_locker_playlist_t));
        memset(playlist, 0, sizeof(mp3tunes_locker_playlist_t));

        playlist->playlistId = xml_xpath_get_string(xml_xpath_context, "playlistId");
        playlist->playlistTitle = xml_xpath_get_string(xml_xpath_context, "playlistTitle");
        playlist->title = xml_xpath_get_string(xml_xpath_context, "title");
        playlist->fileName = xml_xpath_get_string(xml_xpath_context, "fileName");
        playlist->fileCount = xml_xpath_get_integer(xml_xpath_context, "fileCount");
        playlist->playlistSize = xml_xpath_get_integer(xml_xpath_context, "playlistSize");

        mp3tunes_locker_playlist_list_add(playlists, playlist);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;
}

int mp3tunes_locker_search(mp3tunes_locker_object_t *obj, mp3tunes_locker_artist_list_t **artists, mp3tunes_locker_album_list_t **albums, mp3tunes_locker_track_list_t **tracks, const char *query) {
    xml_xpath_t* xml_xpath;

    char type[20] = "";
    if( artists != NULL ) {
      strcat( type, "artist," );
    }
    if( albums != NULL ) {
      strcat( type, "album," );
    }
    if( tracks != NULL ) {
      strcat( type, "track," );
    }
    if( strlen(type) == 0 ) {
      return -1;
    }
    /*printf("type: '%s' query: '%s'\n", placeholder, query);*/

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerSearch", "type", type, "s", query, NULL);

    if(artists != NULL) {
        xmlXPathObjectPtr xpath_obj;
        xmlNodeSetPtr nodeset;
        xmlNodePtr node;
        int i;
        mp3tunes_locker_artist_list_init(artists);

        if (xml_xpath == NULL) {
            return -1;
        }

        xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/artistList/item");
        if (xpath_obj == NULL) {
            return -1;
        }

        nodeset = xpath_obj->nodesetval;

        for (i = 0; i < nodeset->nodeNr; i++) {
            node = nodeset->nodeTab[i];
            xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
            mp3tunes_locker_artist_t *artist = (mp3tunes_locker_artist_t*)malloc(sizeof(mp3tunes_locker_artist_t));
            memset(artist, 0, sizeof(mp3tunes_locker_artist_t));

            artist->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
            artist->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
            artist->artistSize = xml_xpath_get_integer(xml_xpath_context, "artistSize");
            artist->albumCount = xml_xpath_get_integer(xml_xpath_context, "albumCount");
            artist->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");

            mp3tunes_locker_artist_list_add(artists, artist);
            xml_xpath_deinit(xml_xpath_context);
        }
        xmlXPathFreeObject(xpath_obj);
    }

    if( albums != NULL ) {
        xmlXPathObjectPtr xpath_obj;
        xmlNodeSetPtr nodeset;
        xmlNodePtr node;
        int i;

        mp3tunes_locker_album_list_init(albums);

        xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/albumList/item");

        if (xpath_obj == NULL) {
            return -1;
        }

        nodeset = xpath_obj->nodesetval;

        for (i = 0; i < nodeset->nodeNr; i++) {
            node = nodeset->nodeTab[i];
            xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
            mp3tunes_locker_album_t *album = (mp3tunes_locker_album_t*)malloc(sizeof(mp3tunes_locker_album_t));
            memset(album, 0, sizeof(mp3tunes_locker_album_t));

            album->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
            album->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
            album->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");
            album->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
            album->trackCount = xml_xpath_get_integer(xml_xpath_context, "trackCount");
            album->albumSize = xml_xpath_get_integer(xml_xpath_context, "albumSize");
            album->hasArt = xml_xpath_get_integer(xml_xpath_context, "hasArt");

            mp3tunes_locker_album_list_add(albums, album);
            xml_xpath_deinit(xml_xpath_context);
        }
        xmlXPathFreeObject(xpath_obj);
    }
    if( tracks != NULL) {
        xmlXPathObjectPtr xpath_obj;
        xmlNodeSetPtr nodeset;
        xmlNodePtr node;
        int i;

        mp3tunes_locker_track_list_init(tracks);

        xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

        if (xpath_obj == NULL) {
            return -1;
        }

        nodeset = xpath_obj->nodesetval;

        for (i = 0; i < nodeset->nodeNr; i++) {
            node = nodeset->nodeTab[i];
            xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
            mp3tunes_locker_track_t *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));
            memset(track, 0, sizeof(mp3tunes_locker_track_t));

            track->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
            track->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
            track->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
            track->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
            track->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
            track->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
            track->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
            track->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
            track->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
            track->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
            track->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
            track->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
            track->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
            track->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

            mp3tunes_locker_track_list_add(tracks, track);
            xml_xpath_deinit(xml_xpath_context);
        }
        xmlXPathFreeObject(xpath_obj);
    }
    xml_xpath_deinit(xml_xpath);
    return 0;
}

int mp3tunes_locker_sync_down(mp3tunes_locker_object_t *obj, char* type, char* bytes_local, char* files_local, char* keep_local_files, char* playlist_id) {
    xml_xpath_t* xml_xpath;
    xmlBufferPtr buf;
    xmlTextWriterPtr writer;

    buf = xmlBufferCreate();
    if (buf == NULL) {
        return -1;
    }

    writer = xmlNewTextWriterMemory(buf, 0);

    if (writer == NULL) {
        return -1;
    }

    if (xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL) < 0) {
        return -1;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "sync") < 0) {
        return -1;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "options") < 0) {
        return -1;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "direction") < 0) {
        return -1;
    }

    if (xmlTextWriterWriteAttribute(writer, BAD_CAST "sync_down", BAD_CAST "1") < 0) {
        return -1;
    }

    if (xmlTextWriterEndElement(writer) < 0) {
        return -1;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "file_sync") < 0) {
        return -1;
    }

    if (xmlTextWriterWriteAttribute(writer, BAD_CAST "type", BAD_CAST type) < 0) {
        return -1;
    }

    if (xmlTextWriterEndElement(writer) < 0) {
        return -1;
    }

    if (xmlTextWriterStartElement(writer, BAD_CAST "max") < 0) {
        return -1;
    }

    if (bytes_local) {
        if (xmlTextWriterWriteAttribute(writer, BAD_CAST "bytes_local", BAD_CAST bytes_local) < 0) {
            return -1;
        }
    }

    if (files_local) {
        if (xmlTextWriterWriteAttribute(writer, BAD_CAST "files_local", BAD_CAST files_local) < 0) {
            return -1;
        }
    }

    if (keep_local_files) {
        if (xmlTextWriterWriteAttribute(writer, BAD_CAST "keep_local_files", BAD_CAST files_local) < 0) {
            return -1;
        }
    }

    if (xmlTextWriterEndElement(writer) < 0) {
        return -1;
    }

    if (playlist_id) {
        if (xmlTextWriterStartElement(writer, BAD_CAST "playlist") < 0) {
            return -1;
        }

        if (xmlTextWriterWriteAttribute(writer, BAD_CAST "id", BAD_CAST playlist_id) < 0) {
            return -1;
        }

        if (xmlTextWriterEndElement(writer) < 0) {
            return -1;
        }
    }

    if (xmlTextWriterEndDocument(writer) < 0) {
        return -1;
    }

    xmlFreeTextWriter(writer);

    xml_xpath = mp3tunes_locker_api_post_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerSync/", (char*)buf->content);
    if( xml_xpath == NULL)
        return -1;

    printf("Sync:\n%s\n", (const char *) buf->content);

    free(xml_xpath);
    xmlBufferFree(buf);
    return 0;
}

int mp3tunes_locker_generate_track_from_file_key(mp3tunes_locker_object_t *obj, char *file_key, mp3tunes_locker_track_list_t **tracks ) {
    xml_xpath_t* xml_xpath;
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;
    int i;

    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_API, "api/v1/lockerData/", "type", "track", "key", file_key, NULL);

    mp3tunes_locker_track_list_init(tracks);

    if (xml_xpath == NULL) {
        return -1;
    }

    xpath_obj = xml_xpath_query(xml_xpath, "/mp3tunes/trackList/item");

    if (xpath_obj == NULL) {
        return -1;
    }

    nodeset = xpath_obj->nodesetval;

    for (i = 0; i < nodeset->nodeNr; i++) {
        node = nodeset->nodeTab[i];
        xml_xpath_t* xml_xpath_context = xml_xpath_context_init(xml_xpath, node);
        mp3tunes_locker_track_t *track = (mp3tunes_locker_track_t*)malloc(sizeof(mp3tunes_locker_track_t));
        memset(track, 0, sizeof(mp3tunes_locker_track_t));

        track->trackId = xml_xpath_get_integer(xml_xpath_context, "trackId");
        track->trackTitle = xml_xpath_get_string(xml_xpath_context, "trackTitle");
        track->trackNumber = xml_xpath_get_integer(xml_xpath_context, "trackNumber");
        track->trackLength = xml_xpath_get_float(xml_xpath_context, "trackLength");
        track->trackFileName = xml_xpath_get_string(xml_xpath_context, "trackFileName");
        track->trackFileKey = xml_xpath_get_string(xml_xpath_context, "trackFileKey");
        track->trackFileSize = xml_xpath_get_integer(xml_xpath_context, "trackFileSize");
        track->downloadURL = xml_xpath_get_string(xml_xpath_context, "downloadURL");
        track->playURL = xml_xpath_get_string(xml_xpath_context, "playURL");
        track->albumId = xml_xpath_get_integer(xml_xpath_context, "albumId");
        track->albumTitle = xml_xpath_get_string(xml_xpath_context, "albumTitle");
        track->albumYear = xml_xpath_get_integer(xml_xpath_context, "albumYear");
        track->artistName = xml_xpath_get_string(xml_xpath_context, "artistName");
        track->artistId = xml_xpath_get_integer(xml_xpath_context, "artistId");

        mp3tunes_locker_track_list_add(tracks, track);
        xml_xpath_deinit(xml_xpath_context);
    }
    xmlXPathFreeObject(xpath_obj);
    xml_xpath_deinit(xml_xpath);
    return 0;   

}

char* mp3tunes_locker_generate_filekey(const char *filename) {
  return md5_calc_file_signature(filename);
}

int mp3tunes_locker_upload_track(mp3tunes_locker_object_t *obj, const char *path) {
    request_t *request;
    CURLcode res;
    FILE * hd_src ;
    int hd ;
    struct stat file_info;
    char* file_key = mp3tunes_locker_generate_filekey(path);

    if (file_key == NULL)
        return -1;

    /* get the file size of the local file */
    hd = open(path, O_RDONLY);
    if (hd == -1) {
        free(file_key);
        return -1;
    }

    fstat(hd, &file_info);
    close(hd);
    /* get a FILE * of the same file*/
    hd_src = fopen(path, "rb");

    /* create the request url */
    char *url = malloc(256*sizeof(char));
    snprintf(url, 256, "storage/lockerput/%s", file_key);
    free(file_key);
    request = mp3tunes_locker_api_generate_request(obj, MP3TUNES_SERVER_CONTENT, url, NULL);
    if (request == NULL) {
        fclose(hd_src);
        return -1;
    }

    /*chunk_init(&chunk);*/
    /*curl_easy_setopt( request->curl, CURLOPT_READFUNCTION, read_callback);*/
    curl_easy_setopt( request->curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt( request->curl, CURLOPT_PUT, 1L);
    curl_easy_setopt( request->curl, CURLOPT_URL, request->url);
    curl_easy_setopt( request->curl, CURLOPT_READDATA, hd_src);
    curl_easy_setopt( request->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
    curl_easy_setopt( request->curl, CURLOPT_USERAGENT, "liboboe/1.0" );
    /*printf("uploading...\n");*/
    res = curl_easy_perform(request->curl);
    curl_easy_cleanup(request->curl);
    free(request);
    free(url);

    fclose(hd_src); /* close the local file */
    return 0;
}

int mp3tunes_locker_load_track(mp3tunes_locker_object_t *obj, const char *url) {
    xml_xpath_t* xml_xpath;
    char *status;
    xml_xpath = mp3tunes_locker_api_simple_fetch(obj, MP3TUNES_SERVER_LOGIN, "api/v0/lockerLoad/", "email", obj->username, "url", url, "sid", obj->session_id, NULL);

    if (xml_xpath == NULL) {
        return -2;
    }

    status = xml_xpath_get_string(xml_xpath, "/mp3tunes/status");

    if (status[0] != '1') {
        /*printf("status is %s\n", status);*/
        char* error = xml_xpath_get_string(xml_xpath, "/mp3tunes/errorMessage");
        /*printf("error is %s\n", error);*/
        obj->error_message = error;
        free(status);
        xml_xpath_deinit(xml_xpath);
        return -1;
    }
    free(status);
    xml_xpath_deinit(xml_xpath);

    return 0;

}
