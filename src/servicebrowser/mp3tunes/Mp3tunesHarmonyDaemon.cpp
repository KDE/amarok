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

#include "Mp3tunesHarmonyDaemon.h"

GMainLoop * Mp3tunesHarmonyDaemon::m_main_loop = g_main_loop_new(0, FALSE);

Mp3tunesHarmonyDaemon::Mp3tunesHarmonyDaemon(char* identifier )
{
    m_harmony = mp3tunes_harmony_new();
    m_err = 0;
    m_identifier = identifier;

    
    /* g_type_init required for using the GObjects for Harmony. */
    g_type_init();

    /* Set the error signal handler. */
    g_signal_connect(m_harmony, "error", G_CALLBACK(signalErrorHandler), 0);    
    /* Set the state change signal handler. */
    g_signal_connect(&m_harmony, "state_change", G_CALLBACK(signalStateChangeHandler), 0);
    /* Set the download signal handler. */
    g_signal_connect(m_harmony, "download-ready", G_CALLBACK(signalDownloadReady), NULL);
    g_signal_connect(m_harmony, "download-pending", G_CALLBACK(signalDownloadPending), NULL);

    mp3tunes_harmony_set_identifier(m_harmony, m_identifier);
    
    mp3tunes_harmony_set_device_attribute(m_harmony, "device-description", "Example Daemon");

}

void
Mp3tunesHarmonyDaemon::init()
{
    /* Linux specific variable for getting total and available sizes for the
     * file system
     */
    struct statfs fsstats;
    unsigned long long total_bytes;
    unsigned long long available_bytes;

    if (statfs(".", &fsstats) != 0) {
        perror("statfs failed");
        return;
    }

    total_bytes = fsstats.f_bsize * fsstats.f_blocks;
    available_bytes = fsstats.f_bsize * fsstats.f_bavail;
    mp3tunes_harmony_set_device_attribute(m_harmony, "total-bytes", &total_bytes);
    mp3tunes_harmony_set_device_attribute(m_harmony, "available-bytes", &available_bytes);
    
    /* Configure main loop */
    
    /* Start the connection */
    mp3tunes_harmony_connect(m_harmony, &m_err);
    /* Check for errors on the connection */
    if (m_err) {
        g_error("Error: %s\n", m_err->message);
    }

    /* Run the main loop */
    g_main_loop_run(m_main_loop);

}


void
Mp3tunesHarmonyDaemon::signalErrorHandler(MP3tunesHarmony* harmony, gpointer null_pointer )
{
      GError *err;
      null_pointer = null_pointer;
      g_error("Error: %s\n", harmony->error->message);
      mp3tunes_harmony_disconnect(harmony, &err);
      if (err) {
          g_error("Error disconnecting: %s\n", err->message);
          /* If there is an error disconnecting something has probably gone
           * very wrong and reconnection should not be attempted till the user
           * re-initiates it */
          return;
      }
}
void
Mp3tunesHarmonyDaemon::signalStateChangeHandler( MP3tunesHarmony* harmony, guint32 state,  gpointer null_pointer )
{
    null_pointer = null_pointer;
    switch (state) {
        case MP3TUNES_HARMONY_STATE_DISCONNECTED:
            g_print("Disconnected.\n");
            /* Do nothing here */
            break;
        case MP3TUNES_HARMONY_STATE_CONNECTED:
            g_print("Connected! Waiting for download requests!\n");
            /* At this point, it would be best to store the pin, if you haven't
               * already, and the email in some somewhat permenant storage for
               * when reauthenticating.
               */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN:
            g_print("Connection in process!\n");
            /* At this point, just update the user status. */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL:
            g_print("Please login to mp3tunes.com and add the pin '%s' to your devices.\n", mp3tunes_harmony_get_pin(harmony));
            /* At this point, it would be best to store the pin in case the
             * network connection drops. As well, display to the user a status
             * message to have them perform the website authentication action.
             */
            break;
    }
}

void
Mp3tunesHarmonyDaemon::signalDownloadReady( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer )
{
    mp3tunes_harmony_download_t *download = (mp3tunes_harmony_download_t*)void_mp3tunes_harmony_download;
    harmony = harmony;
    null_pointer = null_pointer;
    g_print("Got message about %s by %s on %s\n", download->track_title, download->artist_name, download->album_title);
}

void
Mp3tunesHarmonyDaemon::signalDownloadPending( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer )
{
    mp3tunes_harmony_download_t *download = (mp3tunes_harmony_download_t*)void_mp3tunes_harmony_download;
    harmony = harmony;
    null_pointer = null_pointer;
    g_print("Downloading %s by %s on %s from URL: %s.\n", download->track_title, download->artist_name, download->album_title, download->url);
    if (strcmp(download->file_key, "dummy_file_key_5") == 0) {
        g_main_loop_quit(m_main_loop);
    }
    mp3tunes_harmony_download_deinit(&download);
}
