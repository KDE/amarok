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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harmony.h"

G_DEFINE_TYPE(MP3tunesHarmony, mp3tunes_harmony, G_TYPE_OBJECT);

char *str_replace(const char *search, const char *replace, char *subject);
gboolean harmony_format_email(char **email);

void state_change_emit(MP3tunesHarmony *harmony, guint32 state);
void error_emit(MP3tunesHarmony *harmony, gint code, const gchar* message, GError* err);

LmHandlerResult harmony_download_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony);

LmHandlerResult harmony_get_device_pin_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony);
void harmony_get_device_pin(MP3tunesHarmony *harmony);

LmHandlerResult harmony_get_device_email_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony);
void harmony_get_device_email(MP3tunesHarmony *harmony);

void authenticate_known_callback(LmConnection* connection, gboolean success, gpointer void_harmony);
void authenticate_new_callback(LmConnection* connection, gboolean success, gpointer void_harmony);
void authenticate_unknown_callback(LmConnection* connection, gboolean success, gpointer void_harmony);

void rebuild_connection(MP3tunesHarmony* harmony);

void open_connection_callback(LmConnection* connection, gboolean success, gpointer void_harmony);
gboolean open_connection(MP3tunesHarmony *harmony);

gboolean close_connection(MP3tunesHarmony *harmony);

char *str_replace(const char *search, const char *replace, char *subject) {
    char *result, *tmp, *needle;
    int count, length;

    length = strlen(subject);
    if (strlen(search) < strlen(replace)) {
        /* We need to increase the buffer size for result a bit */
        count = 0;
        tmp = subject;

        while((needle = strstr(tmp, search))) {
            tmp = needle + strlen(search);
            count++;
        }

        length += (strlen(replace) - strlen(search)) * count;
    }

    result = (char *)malloc(sizeof(char)*(length+1));
    memset(result, 0, sizeof(char) * (length + 1));

    tmp = subject;
    while((needle = strstr(tmp, search))) {
        length = needle-tmp;           /* Length from current index in buffer to a match */
        strncat(result, tmp, length);  /* put normal (unreplaced) string data into the new buffer */
        strcat(result, replace);       /* then the replacement */
        tmp = needle + strlen(search); /* Jump past the replaced text and continue */
    }

    strcat(result, tmp);               /* remaining data after the last match */
    return result;
}

gboolean harmony_format_email(char **email) {
    char *tmp;
    char *tmp_two;

    if (*email == NULL) return FALSE;

    tmp = str_replace("@", "_AT_", *email);
    free(*email);

    tmp_two = str_replace(".", "_DOT_", tmp);
    free(tmp);

    *email = tmp_two;

    return TRUE;
}

void state_change_emit(MP3tunesHarmony *harmony, guint32 state) {
    g_signal_emit(harmony, MP3TUNES_HARMONY_GET_CLASS(harmony)->state_change_signal_id, 0, state);
}

void error_emit(MP3tunesHarmony *harmony, gint code, const gchar* message, GError* err) {
    if (err) {
        g_propagate_error(&harmony->error, g_error_new(MP3TUNES_HARMONY_ERROR_DOMAIN, code, "%s: %s", message, err->message));
    } else {
        g_propagate_error(&harmony->error, g_error_new(MP3TUNES_HARMONY_ERROR_DOMAIN, code, "%s", message));
    }
    g_signal_emit(harmony, MP3TUNES_HARMONY_GET_CLASS(harmony)->error_signal_id, 0);
}

void download_pending_emit(MP3tunesHarmony *harmony, mp3tunes_harmony_download_t* harmony_download) {
    g_print("Signal ID: %d, Download: %p\n", MP3TUNES_HARMONY_GET_CLASS(harmony)->download_pending_signal_id, harmony_download);
    g_signal_emit(harmony, MP3TUNES_HARMONY_GET_CLASS(harmony)->download_pending_signal_id, 0, harmony_download);
}

void download_ready_emit(MP3tunesHarmony *harmony, mp3tunes_harmony_download_t* harmony_download) {
    g_print("Signal ID: %d, Download: %p\n", MP3TUNES_HARMONY_GET_CLASS(harmony)->download_ready_signal_id, harmony_download);
    g_signal_emit(harmony, MP3TUNES_HARMONY_GET_CLASS(harmony)->download_ready_signal_id, 0, harmony_download);
}

gboolean mp3tunes_harmony_download_init(mp3tunes_harmony_download_t **harmony_download) {
    mp3tunes_harmony_download_t* hd = *harmony_download = malloc(sizeof(mp3tunes_harmony_download_t));
    if (hd == NULL) { return FALSE; }
    hd->file_key = NULL;
    hd->file_name = NULL;
    hd->file_format = NULL;
    hd->file_size = 0;
    hd->artist_name = NULL;
    hd->album_title = NULL;
    hd->track_title = NULL;
    hd->track_number = 0;
    hd->device_bitrate = NULL;
    hd->file_bitrate = NULL;
    hd->url = NULL;
    return TRUE;
}

gboolean mp3tunes_harmony_download_deinit(mp3tunes_harmony_download_t **harmony_download) {
    mp3tunes_harmony_download_t* hd = *harmony_download;
    if (hd->file_key) { free(hd->file_key); }
    if (hd->file_name) { free(hd->file_name); }    
    if (hd->file_format) { free(hd->file_format); }
    
    if (hd->artist_name) { free(hd->artist_name); }
    if (hd->album_title) { free(hd->album_title); }
    if (hd->track_title) { free(hd->track_title); }
    
    if (hd->device_bitrate) { free(hd->device_bitrate); }
    if (hd->file_bitrate) { free(hd->file_bitrate); }
    
    if (hd->url) { free(hd->url); }
    free(hd);
    return TRUE;
}

void mp3tunes_harmony_download_set_file_key(mp3tunes_harmony_download_t *harmony_download, char* file_key) {
    harmony_download->file_key = strdup(file_key);
}

void mp3tunes_harmony_download_set_file_name(mp3tunes_harmony_download_t *harmony_download, char* file_name) {
    harmony_download->file_name = strdup(file_name);
}

void mp3tunes_harmony_download_set_file_format(mp3tunes_harmony_download_t *harmony_download, char* file_format) {
    harmony_download->file_format = strdup(file_format);
}

void mp3tunes_harmony_download_set_file_size(mp3tunes_harmony_download_t *harmony_download, unsigned int file_size) {
    harmony_download->file_size = file_size;
}

void mp3tunes_harmony_download_set_artist_name(mp3tunes_harmony_download_t *harmony_download, char* artist_name) {
    harmony_download->artist_name = strdup(artist_name);
}

void mp3tunes_harmony_download_set_album_title(mp3tunes_harmony_download_t *harmony_download, char* album_title) {
    harmony_download->album_title = strdup(album_title);
}

void mp3tunes_harmony_download_set_track_title(mp3tunes_harmony_download_t *harmony_download, char* track_title) {
    harmony_download->track_title = strdup(track_title);
}

void mp3tunes_harmony_download_set_track_number(mp3tunes_harmony_download_t *harmony_download, int track_number) {
    harmony_download->track_number = track_number;
}

void mp3tunes_harmony_download_set_device_bitrate(mp3tunes_harmony_download_t *harmony_download, char* device_bitrate) {
    if (device_bitrate) {
        harmony_download->device_bitrate = strdup(device_bitrate);
    }
}

void mp3tunes_harmony_download_set_file_bitrate(mp3tunes_harmony_download_t *harmony_download, char* file_bitrate) {
    if (file_bitrate) {
        harmony_download->file_bitrate = strdup(file_bitrate);
    }
}

void mp3tunes_harmony_download_set_url_using_locker(mp3tunes_harmony_download_t *harmony_download, mp3tunes_locker_object_t* mp3tunes_locker) {
    if ((harmony_download->device_bitrate == NULL) || (atoi(harmony_download->device_bitrate)) == 0) {
        harmony_download->url = mp3tunes_locker_generate_download_url_from_file_key(mp3tunes_locker, harmony_download->file_key);
    } else {
        harmony_download->url = mp3tunes_locker_generate_download_url_from_file_key_and_bitrate(mp3tunes_locker, harmony_download->file_key, harmony_download->device_bitrate);
    }
}

void harmony_reprocess_queue(MP3tunesHarmony *harmony) {
    mp3tunes_harmony_download_t *harmony_download = g_queue_peek_head(harmony->download_queue);
    while (harmony_download != NULL && harmony->sid_state == MP3TUNES_HARMONY_SID_STATE_READY) {
        harmony_download = g_queue_pop_head(harmony->download_queue);
        mp3tunes_harmony_download_set_url_using_locker(harmony_download, harmony->mp3tunes_locker);
        download_ready_emit(harmony, harmony_download);
        harmony_download = g_queue_peek_head(harmony->download_queue);
    }
}

LmHandlerResult harmony_get_session_id_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony) {
    char *session_id;
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    LmMessageNode *harmony_session_node;

    handler = handler;    
    connection = connection;

    harmony_session_node = lm_message_node_get_child(message->node, "sessionId");
    if (harmony_session_node) {
        session_id = g_strdup(lm_message_node_get_value(harmony_session_node));
        harmony->mp3tunes_locker->session_id = session_id;
        harmony->sid_state = MP3TUNES_HARMONY_SID_STATE_READY;
        
        harmony_reprocess_queue(harmony);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

void harmony_get_session_id(MP3tunesHarmony *harmony) {
    LmMessage *message_out;
    LmMessageHandler *handler;
    LmMessageNode *message_out_node;
    GError *err = NULL;
    handler = lm_message_handler_new(harmony_get_session_id_callback, (gpointer)harmony, NULL);

    message_out = lm_message_new(MP3TUNES_HARMONY_CONDUCTOR, LM_MESSAGE_TYPE_IQ);
    message_out_node = lm_message_node_add_child(message_out->node, "sessionId", NULL);
    lm_message_node_set_attribute(message_out_node, "xmlns", MP3TUNES_HARMONY_XMLNS);

    lm_connection_send_with_reply(harmony->connection,
                                  message_out,
                                  handler,
                                  &err);
    lm_message_unref(message_out);
    if (err != NULL) {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending session id request failed", err);
        return;
    }
    harmony->sid_state = MP3TUNES_HARMONY_SID_STATE_WAITING;
}

mp3tunes_harmony_download_t* mp3tunes_harmony_download_queue_pop(MP3tunesHarmony *harmony) {
    if (harmony->sid_state != MP3TUNES_HARMONY_SID_STATE_READY) {
        return NULL;
    }       
    return (mp3tunes_harmony_download_t*)g_queue_pop_head(harmony->download_queue);
}

void mp3tunes_harmony_download_failed(MP3tunesHarmony* harmony, mp3tunes_harmony_download_t* harmony_download) {
    harmony->mp3tunes_locker->session_id = NULL;
    harmony->sid_state = MP3TUNES_HARMONY_SID_STATE_NONE;
    g_queue_push_head(harmony->download_queue, harmony_download);
    if (harmony->sid_state == MP3TUNES_HARMONY_SID_STATE_READY) {
        harmony_get_session_id(harmony);
    }
}

void mp3tunes_harmony_download_cancel(MP3tunesHarmony* harmony, mp3tunes_harmony_download_t* harmony_download) {
    g_queue_remove(harmony->download_queue, harmony_download);
}

void mp3tunes_harmony_add_download_to_queue(MP3tunesHarmony *harmony, mp3tunes_harmony_download_t* harmony_download) {
    g_queue_push_tail(harmony->download_queue, harmony_download);
    if (harmony->sid_state == MP3TUNES_HARMONY_SID_STATE_NONE) {
        harmony_get_session_id(harmony);    
    } else if (harmony->sid_state == MP3TUNES_HARMONY_SID_STATE_READY) {
        harmony_download->url = mp3tunes_locker_generate_download_url_from_file_key(harmony->mp3tunes_locker, harmony_download->file_key);
        download_ready_emit(harmony, harmony_download);
    }
}

void harmony_success_reply(LmConnection *connection, LmMessage *message, GError **err) {
    LmMessage *message_out;
    LmMessageNode *harmony_download_node;
    LmMessageNode *message_out_node;
    
    message_out = lm_message_new_with_sub_type(MP3TUNES_HARMONY_CONDUCTOR, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_RESULT);
    lm_message_node_set_attribute(message_out->node, "id", lm_message_node_get_attribute(message->node, "id"));
    message_out_node = lm_message_node_add_child(message_out->node, "success", NULL);

    harmony_download_node = lm_message_node_get_child(message->node, "download");
    if (harmony_download_node) {
        lm_message_node_set_attribute(message_out_node, "messageId", lm_message_node_get_attribute(harmony_download_node, "messageId"));
    }
    lm_message_node_set_attribute(message_out_node, "xmlns", MP3TUNES_HARMONY_XMLNS);

    lm_connection_send(connection, message_out, err);
    lm_message_unref(message_out);
}

LmHandlerResult harmony_iq_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony) {
    GError *err = NULL;
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    LmMessageNode *harmony_download_node, *harmony_email_node;

    mp3tunes_harmony_download_t *download;
    gchar *email;
    
    handler = handler;

    harmony_download_node = lm_message_node_get_child(message->node, "download");
    if (harmony_download_node) {
        mp3tunes_harmony_download_init(&download);
        mp3tunes_harmony_download_set_file_key(download, (char*)lm_message_node_get_attribute(harmony_download_node, "fileKey"));
        mp3tunes_harmony_download_set_file_name(download, (char*)lm_message_node_get_attribute(harmony_download_node, "fileName"));
        mp3tunes_harmony_download_set_file_format(download, (char*)lm_message_node_get_attribute(harmony_download_node, "fileFormat"));
        mp3tunes_harmony_download_set_file_size(download, atoi(lm_message_node_get_attribute(harmony_download_node, "fileSize")));
        mp3tunes_harmony_download_set_track_title(download, (char*)lm_message_node_get_attribute(harmony_download_node, "trackTitle"));
        mp3tunes_harmony_download_set_artist_name(download, (char*)lm_message_node_get_attribute(harmony_download_node, "artistName"));
        mp3tunes_harmony_download_set_album_title(download, (char*)lm_message_node_get_attribute(harmony_download_node, "albumTitle"));
        mp3tunes_harmony_download_set_device_bitrate(download, (char*)lm_message_node_get_attribute(harmony_download_node, "deviceBitrate"));
        mp3tunes_harmony_download_set_file_bitrate(download, (char*)lm_message_node_get_attribute(harmony_download_node, "fileBitrate"));
        
        download_pending_emit(harmony, download);
        
        mp3tunes_harmony_add_download_to_queue(harmony, download);
     
        harmony_success_reply(connection, message, &err);
        
        if (err != NULL) {
           error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending success reply failed", err);
        }
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    
    harmony_email_node = lm_message_node_get_child(message->node, "email");
    if (harmony_email_node) {
        email = g_strdup(lm_message_node_get_value(harmony_email_node));
        mp3tunes_harmony_set_email(harmony, email);
        harmony_success_reply(connection, message, &err);
        if (err != NULL) {
           error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending success reply failed", err);
        }
        close_connection(harmony);
        open_connection(harmony);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }

    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

LmHandlerResult harmony_get_device_pin_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony) {
    char *pin;
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    LmMessageNode *harmony_pin_node;

    handler = handler;
    connection = connection;

    harmony_pin_node = lm_message_node_get_child(message->node, "pin");
    if (harmony_pin_node) {
        pin = g_strdup(lm_message_node_get_value(harmony_pin_node));
        mp3tunes_harmony_set_pin(harmony, pin);
        close_connection(harmony);
        open_connection(harmony);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

void harmony_get_device_pin(MP3tunesHarmony *harmony) {
    LmMessage *message_out;
    LmMessageNode *message_out_node;
    LmMessageHandler *handler;
    GError *err = NULL;
    handler = lm_message_handler_new(harmony_get_device_pin_callback, (gpointer)harmony, NULL);

    message_out = lm_message_new_with_sub_type(MP3TUNES_HARMONY_CONDUCTOR, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_GET);
    message_out_node = lm_message_node_add_child(message_out->node, "pin", NULL);
    lm_message_node_set_attribute(message_out_node, "xmlns", MP3TUNES_HARMONY_XMLNS);

    lm_connection_send_with_reply(harmony->connection,
                                  message_out,
                                  handler,
                                  &err);
    lm_message_unref(message_out);
    if (err != NULL) {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending device pin request failed", err);
        return;
    }

    state_change_emit(harmony, MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN);
}

LmHandlerResult harmony_get_device_email_callback(LmMessageHandler* handler, LmConnection *connection, LmMessage *message, gpointer void_harmony) {
    char *email;
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    LmMessageNode *harmony_email_node;

    handler = handler;
    connection = connection;

    harmony_email_node = lm_message_node_get_child(message->node, "email");
    if (harmony_email_node) {
        email = g_strdup(lm_message_node_get_value(harmony_email_node));
        sleep(2); /*
                     FIXME: This exists because mp3tunes website logins cannot 
                     exceed 1 per second. When a device connects that has been
                     fully authenticated previously it will rapidly reconnect
                     three times as it grabs pin, then email, then connects completely.
                  */
        mp3tunes_harmony_set_email(harmony, email);
        close_connection(harmony);
        open_connection(harmony);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    return LM_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

void harmony_get_device_email(MP3tunesHarmony *harmony) {
    LmMessage *message_out;
    LmMessageHandler *handler;
    LmMessageNode *message_out_node;
    GError *err = NULL;
    handler = lm_message_handler_new(harmony_get_device_email_callback, (gpointer)harmony, NULL);

    message_out = lm_message_new(MP3TUNES_HARMONY_CONDUCTOR, LM_MESSAGE_TYPE_IQ);
    message_out_node = lm_message_node_add_child(message_out->node, "email", NULL);
    lm_message_node_set_attribute(message_out_node, "xmlns", MP3TUNES_HARMONY_XMLNS);

    lm_connection_send_with_reply(harmony->connection,
                                  message_out,
                                  handler,
                                  &err);
    lm_message_unref(message_out);
    if (err != NULL) {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending device email request failed", err);
        return;
    }

    state_change_emit(harmony, MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL);
}

void authenticate_known_callback(LmConnection* connection, gboolean success, gpointer void_harmony) {
    GError *err = NULL;
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    connection = connection;
    if (success) {
        harmony->connected = TRUE;
        state_change_emit(harmony, MP3TUNES_HARMONY_STATE_CONNECTED);
        mp3tunes_harmony_send_device_status(harmony, &err);
        if (err) {
            error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending device status failed", err);
        }
    } else {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Authentication failed", NULL);
        close_connection(harmony);
    }
}

void authenticate_new_callback(LmConnection* connection, gboolean success, gpointer void_harmony) {
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    connection = connection;
    if (success) {
        harmony_get_device_pin(harmony);
    } else {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Authentication failed", NULL);
        close_connection(harmony);
    }
}

void authenticate_unknown_callback(LmConnection* connection, gboolean success, gpointer void_harmony) {
    MP3tunesHarmony *harmony = MP3TUNES_HARMONY(void_harmony);
    connection = connection;
    if (success) {
        harmony_get_device_email(harmony);
    } else {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Authentication failed", NULL);
        close_connection(harmony);
    }
}

void open_connection_callback(LmConnection* connection, gboolean success, gpointer void_harmony) {
    GError *err = NULL;
    MP3tunesHarmony* harmony = MP3TUNES_HARMONY(void_harmony);
    LmMessage *message;

    if (!success) {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Failed to open connection", err);
        return;
    }

    message = lm_message_new_with_sub_type(NULL,
                                           LM_MESSAGE_TYPE_PRESENCE,
                                           LM_MESSAGE_SUB_TYPE_AVAILABLE);
    lm_connection_send(connection, message, &err);
    lm_message_unref(message);
    if (err != NULL) {
        error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending presence message failed", err);
        return;
    }

    /* Authenticate, known */
    if (harmony->device_email != NULL && harmony->device_pin != NULL) {
        if (!lm_connection_authenticate(connection,
                                        harmony->device_formatted_email,
                                        g_strdup_printf("PIN-%s", harmony->device_pin),
                                        harmony->device_pin,
                                        (LmResultFunction) authenticate_known_callback,
                                        harmony,
                                        NULL,
                                        &err)) {
            if (err != NULL) {
                error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending authentication failed", err);
            }
            return;
        }
        return;
    }

    /* Authenticate, unknown */
    if (harmony->device_identifier != NULL && 
        harmony->device_pin        != NULL && 
        strncmp(harmony->device_pin, "NULL", 4) != 0) {
        if (!lm_connection_authenticate(connection,
                                        harmony->device_identifier,
                                        MP3TUNES_HARMONY_DEFAULT_PASSWORD,
                                        harmony->device_pin,
                                        (LmResultFunction) authenticate_unknown_callback,
                                        harmony,
                                        NULL,
                                        &err)) {
            if (err != NULL) {
                error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending authentication failed", err);
            }
            return;
        }
        return;
    }

    /* Authenticate, new */
    if (harmony->device_identifier != NULL) {
        if (!lm_connection_authenticate(connection,
                                        harmony->device_identifier,
                                        MP3TUNES_HARMONY_DEFAULT_PASSWORD,
                                        harmony->device_pin,
                                        (LmResultFunction) authenticate_new_callback,
                                        harmony,
                                        NULL,
                                        &err)) {
            if (err != NULL) {
                error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Sending authentication failed", err);
            }
            return;
        }
        return;
    }

    error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "A device identifier is required to authenticate", NULL);
    return;
}

void rebuild_connection(MP3tunesHarmony* harmony) {
    gchar* jid; 
    
    /*
    if (harmony->connection != NULL) {
        lm_connection_unref(harmony->connection);
        harmony->connection = NULL;
    }
    if (harmony->harmony_download_message_handler != NULL) {
        lm_message_handler_unref(harmony->harmony_download_message_handler);
        harmony->harmony_download_message_handler = NULL;
    }
    */
    harmony->connection = lm_connection_new(harmony->host);
    harmony->harmony_iq_message_handler = lm_message_handler_new(harmony_iq_callback, harmony, NULL);    

    lm_connection_set_port(harmony->connection, harmony->port);

    jid = mp3tunes_harmony_get_jid(harmony);
    g_debug("Logging in with: %s", jid);
    lm_connection_set_jid(harmony->connection, jid);
    free(jid);    
    lm_connection_register_message_handler(harmony->connection, harmony->harmony_iq_message_handler, LM_MESSAGE_TYPE_IQ, LM_HANDLER_PRIORITY_LAST);
}

gboolean open_connection(MP3tunesHarmony *harmony) {
    GError *err = NULL;

    rebuild_connection(harmony);

    if (!lm_connection_is_open(harmony->connection)) {
        lm_connection_open(harmony->connection, 
                           (LmResultFunction) open_connection_callback,
                           harmony,
                           NULL,
                           &err);

        if (err != NULL) {
            error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Opening the connection failed", err);
            return FALSE;
        }
    }

    return TRUE;
}

gboolean close_connection(MP3tunesHarmony *harmony) {
    GError *err = NULL;

    if (lm_connection_is_open(harmony->connection)) {
        lm_connection_close(harmony->connection, &err);
        lm_connection_unref(harmony->connection);
        if (err != NULL) {
            error_emit(harmony, MP3TUNES_HARMONY_ERROR_MISC, "Closing the connection failed", err);
            return FALSE;
        }
/*        return TRUE; */
/*        state_change_emit(harmony, MP3TUNES_HARMONY_STATE_DISCONNECTED); */
    }

    return TRUE;
}

gboolean mp3tunes_harmony_disconnect(MP3tunesHarmony *harmony, GError** err) {
    gboolean success = close_connection(harmony);
    harmony->connected = FALSE;
    if (success == FALSE) {
        err = &harmony->error;
        return success;
    }
    return success;
}

gboolean mp3tunes_harmony_connect(MP3tunesHarmony* harmony, GError** err) {
    gboolean success = FALSE;
    if (harmony->connected) {
        return TRUE;
    }

    success = open_connection(harmony);

    if (success == FALSE) {
        err = &harmony->error;
        mp3tunes_harmony_disconnect(harmony, err);
        return success;
    }
    return success;
}

void mp3tunes_harmony_send_device_status(MP3tunesHarmony *harmony, GError **err) {
    LmMessage *message_out;
    /*LmMessageHandler *handler;*/
    LmMessageNode* status_message;

    GList *current = NULL;
    mp3tunes_harmony_device_attribute_t *da;

    char* name = NULL;
    char* value = NULL;

    /*handler = lm_message_handler_new(harmony_get_device_email_callback, (gpointer)harmony, NULL);*/

    message_out = lm_message_new_with_sub_type(MP3TUNES_HARMONY_CONDUCTOR, LM_MESSAGE_TYPE_IQ, LM_MESSAGE_SUB_TYPE_SET);
    status_message = lm_message_node_add_child(message_out->node, "deviceStatus", NULL);
    lm_message_node_set_attribute(status_message, "xmlns", MP3TUNES_HARMONY_XMLNS);

    current = g_list_first(harmony->device_attributes);
    while (current != NULL) {
        da = (mp3tunes_harmony_device_attribute_t*) current->data;
        name = da->attribute_name;
        if (da->attribute_value_type == MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_STRING) {
            value = g_strdup(da->attribute_string_value);
        } else if (da->attribute_value_type == MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_INT) {
            value = g_strdup_printf("%lld", da->attribute_int_value);
        }
        lm_message_node_set_attribute(status_message, name, value);
        free(value);
        current = g_list_next(current);
    }

    lm_connection_send(harmony->connection, message_out, err);
    lm_message_unref(message_out);
    if (err != NULL) {
        return;
    }
}

void mp3tunes_harmony_set_identifier(MP3tunesHarmony *harmony, char *identifier) {
    if (harmony->device_identifier != NULL) {
        free(harmony->device_identifier);
    }
    harmony->device_identifier = g_strdup(identifier);
}

void mp3tunes_harmony_set_pin(MP3tunesHarmony *harmony, const char *pin) {
    if (harmony->device_pin != NULL) {
        free(harmony->device_pin);
    }
    harmony->device_pin = g_strdup(pin);
}

void mp3tunes_harmony_set_email(MP3tunesHarmony *harmony, char *email) {
    if (harmony->device_email != NULL) {
        free(harmony->device_email);
        free(harmony->device_formatted_email);
    }
    harmony->device_email = g_strdup(email);
    harmony->device_formatted_email = g_strdup(email);
    harmony_format_email(&harmony->device_formatted_email);
}

void mp3tunes_harmony_set_device_attribute(MP3tunesHarmony *harmony, const char *attribute, ...) {
    va_list argp;
    mp3tunes_harmony_device_attribute_t* da;

    GList *current = NULL;
    mp3tunes_harmony_device_attribute_t *current_da;

    da = (mp3tunes_harmony_device_attribute_t*)malloc(sizeof(mp3tunes_harmony_device_attribute_t));
    if (da == NULL)
        return;

    va_start(argp, attribute);

    if (strcmp(attribute, "device-description") == 0) {
        da->attribute_name = g_strdup("deviceDescription");
        da->attribute_string_value = g_strdup(va_arg(argp, char *));
        da->attribute_value_type = MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_STRING;
    } else if (strcmp(attribute, "total-bytes") == 0) {
        da->attribute_name = g_strdup("total-bytes");
        da->attribute_int_value = va_arg(argp, long long int);
        da->attribute_value_type = MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_INT;
    } else if (strcmp(attribute, "available-bytes") == 0) {
        da->attribute_name = g_strdup("available-bytes");
        da->attribute_int_value = va_arg(argp, long long int);
        da->attribute_value_type = MP3TUNES_HARMONY_DEVICE_ATTRIBUTE_TYPE_INT;
    } else {
        va_end(argp);
        free(da);
        return;
    }

    current = g_list_first(harmony->device_attributes);
    while (current != NULL) {
        current_da = (mp3tunes_harmony_device_attribute_t*) current->data;
        if (strcmp((current_da->attribute_name), da->attribute_name) == 0) {
            harmony->device_attributes = g_list_insert_before(harmony->device_attributes, current, da);
            harmony->device_attributes = g_list_remove(harmony->device_attributes, current_da);
            va_end(argp);
            return;
        }
        current = g_list_next(current);
    }

    harmony->device_attributes = g_list_prepend(harmony->device_attributes, da);
    va_end(argp);
    return;
}

char *mp3tunes_harmony_get_identifier(MP3tunesHarmony *harmony) {
    return harmony->device_identifier;
}

char *mp3tunes_harmony_get_pin(MP3tunesHarmony *harmony) {
    return harmony->device_pin;
}

char *mp3tunes_harmony_get_email(MP3tunesHarmony *harmony) {
    return harmony->device_email;
}

char *mp3tunes_harmony_get_jid(MP3tunesHarmony *harmony) {
    if (harmony->device_formatted_email != NULL && harmony->device_pin != NULL) {
        return g_strdup_printf("%s@%s/%s", harmony->device_formatted_email, MP3TUNES_HARMONY_JID_HOST, harmony->device_pin);
    }
    if (harmony->device_identifier != NULL && 
        harmony->device_pin        != NULL && 
        strncmp(harmony->device_pin, "NULL", 4) != 0) {
        return g_strdup_printf("%s@%s/%s", harmony->device_identifier, MP3TUNES_HARMONY_JID_HOST, harmony->device_pin);
    }
    if (harmony->device_identifier != NULL) {
        return g_strdup_printf("%s@%s/%s", harmony->device_identifier, MP3TUNES_HARMONY_JID_HOST, harmony->device_pin);
    }
    return NULL;
}

MP3tunesHarmony *mp3tunes_harmony_new(void) {
    return MP3TUNES_HARMONY(g_object_new(MP3TUNES_TYPE_HARMONY, NULL));
}

static void mp3tunes_harmony_class_init(MP3tunesHarmonyClass *klass) {
    klass->state_change_signal_id = g_signal_new("state_change",
                                                 G_TYPE_FROM_CLASS(klass),
                                                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 g_cclosure_marshal_VOID__UINT,
                                                 G_TYPE_NONE,
                                                 1,    
                                                 G_TYPE_UINT);

    klass->error_signal_id = g_signal_new("error",
                                          G_TYPE_FROM_CLASS(klass),
                                          G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                          0,
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);

    klass->download_pending_signal_id = g_signal_new("download_pending",
                                             G_TYPE_FROM_CLASS(klass),
                                             G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                             0,
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__POINTER,
                                             G_TYPE_NONE,
                                             1,
                                             G_TYPE_POINTER);

    klass->download_ready_signal_id = g_signal_new("download_ready",
                                             G_TYPE_FROM_CLASS(klass),
                                             G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                                             0,
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__POINTER,
                                             G_TYPE_NONE,
                                             1,
                                             G_TYPE_POINTER);                                                      
}

static void mp3tunes_harmony_init(MP3tunesHarmony *self) {
    char *port;
    self->connected = FALSE;

    mp3tunes_locker_init(&self->mp3tunes_locker, "7794175043");

    self->download_queue = g_queue_new();

    self->sid_state = MP3TUNES_HARMONY_SID_STATE_NONE;

    self->error = NULL;

    self->device_identifier = NULL;
    self->device_pin = NULL;
    mp3tunes_harmony_set_pin(self, "NULL");
    self->device_email = NULL;

    self->device_attributes = NULL;

    self->host = getenv("MP3TUNES_HARMONY_HOST");
    if(self->host == NULL) {
        self->host = MP3TUNES_HARMONY_HOST;
    }

    port = getenv("MP3TUNES_HARMONY_PORT");
    if(port == NULL) {
        self->port = MP3TUNES_HARMONY_PORT;
    } else {
        self->port = atoi(port);
        free(port);
    }
    
    self->connection = NULL;
    self->harmony_iq_message_handler = NULL;
}
