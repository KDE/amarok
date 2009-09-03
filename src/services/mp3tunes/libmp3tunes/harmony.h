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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU Library General Public License along with *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef __HARMONY_H
#define __HARMONY_H

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include "locker.h"
#include "loudmouth/loudmouth.h"

G_BEGIN_DECLS


#define MP3TUNES_HARMONY_HOST "harmony.mp3tunes.com"
#define MP3TUNES_HARMONY_JID_HOST "mp3tunes.com"
#define MP3TUNES_HARMONY_PORT 5222
#define MP3TUNES_HARMONY_CONDUCTOR "harmony_conductor@mp3tunes.com/harmony"
#define MP3TUNES_HARMONY_DEFAULT_PASSWORD "harmony"
#define MP3TUNES_HARMONY_XMLNS "http://www.mp3tunes.com/api/harmony"

enum {
    MP3TUNES_HARMONY_ERROR_MISC
};

typedef enum {
    MP3TUNES_HARMONY_STATE_DISCONNECTED,
    MP3TUNES_HARMONY_STATE_CONNECTED,
    MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN,
    MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL
} mp3tunes_harmony_state_t;

typedef enum {
    MP3TUNES_HARMONY_SID_STATE_NONE,
    MP3TUNES_HARMONY_SID_STATE_WAITING,
    MP3TUNES_HARMONY_SID_STATE_READY
} mp3tunes_harmony_sid_state_t;

typedef enum {
    MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_STRING,
    MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_INT
} mp3tunes_harmony_device_attribute_type_t;

typedef struct {
    char* attribute_name;
    char* attribute_string_value;
    long long int attribute_int_value;
    mp3tunes_harmony_device_attribute_type_t attribute_value_type;
} mp3tunes_harmony_device_attribute_t;

typedef struct {
    char* file_key;
    char* file_name;
    char* file_format;
    unsigned int file_size;
    char* artist_name;
    char* album_title;
    char* track_title;
    int track_number;
    char* device_bitrate;
    char* file_bitrate;
    char* url;
} mp3tunes_harmony_download_t;

int mp3tunes_harmony_download_init(mp3tunes_harmony_download_t **harmony_download_t);
int mp3tunes_harmony_download_deinit(mp3tunes_harmony_download_t **harmony_download_t);

typedef struct {
    GObject parent;
    LmConnection *connection;
    LmMessageHandler *harmony_iq_message_handler;

    mp3tunes_locker_object_t* mp3tunes_locker;

    GQueue *download_queue;

    gboolean connected;
    char *device_identifier;
    char *device_pin;
    char *device_email;
    char *device_formatted_email;
    char *host;
    int port;
    mp3tunes_harmony_sid_state_t sid_state;
    GList *device_attributes;
    GError *error;
} MP3tunesHarmony;

typedef struct {
    GObjectClass parent;

    guint state_change_signal_id;
    guint error_signal_id;
    guint download_pending_signal_id;
    guint download_ready_signal_id;
} MP3tunesHarmonyClass;

#define MP3TUNES_HARMONY_ERROR_DOMAIN g_quark_from_string("MP3TUNES_HARMONY")

#define MP3TUNES_TYPE_HARMONY            (mp3tunes_harmony_get_type ())
#define MP3TUNES_HARMONY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MP3TUNES_TYPE_HARMONY, MP3tunesHarmony))
#define MP3TUNES_HARMONY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MP3TUNES_TYPE_HARMONY, MP3tunesHarmonyClass))
#define MP3TUNES_IS_HARMONY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),MP3TUNES_TYPE_HARMONY))
#define MP3TUNES_IS_HARMONY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MP3TUNES_TYPE_HARMONY))
#define MP3TUNES_HARMONY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MP3TUNES_TYPE_HARMONY, MP3tunesHarmonyClass))

MP3tunesHarmony* mp3tunes_harmony_new(void);
GType mp3tunes_harmony_get_type(void);

void mp3tunes_harmony_set_pin(MP3tunesHarmony *harmony, const char *pin);
void mp3tunes_harmony_set_email(MP3tunesHarmony *harmony, char *email);
void mp3tunes_harmony_set_identifier(MP3tunesHarmony *harmony, char *email);
void mp3tunes_harmony_set_device_attribute(MP3tunesHarmony *harmony, const char *attribute, ...);

char *mp3tunes_harmony_get_pin(MP3tunesHarmony *harmony);
char *mp3tunes_harmony_get_email(MP3tunesHarmony *harmony);
char *mp3tunes_harmony_get_identifier(MP3tunesHarmony *harmony);
char *mp3tunes_harmony_get_jid(MP3tunesHarmony *harmony);

gboolean mp3tunes_harmony_connect(MP3tunesHarmony *harmony, GError** err);
gboolean mp3tunes_harmony_disconnect(MP3tunesHarmony *harmony, GError** err);

void mp3tunes_harmony_send_device_status(MP3tunesHarmony *harmony, GError **err);

mp3tunes_harmony_download_t* mp3tunes_harmony_download_queue_pop(MP3tunesHarmony *harmony);

G_END_DECLS

#endif
