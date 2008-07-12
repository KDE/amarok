/***************************************************************************
 *   Copyright (c) 2007, 2008  Casey Link <unnamedrambler@gmail.com>       *
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

#include "Debug.h"

GMainLoop * Mp3tunesHarmonyDaemon::m_main_loop = g_main_loop_new(0, FALSE);

Mp3tunesHarmonyDaemon::Mp3tunesHarmonyDaemon(char* identifier ) :
    m_identifier( identifier )
   , m_gerr( 0 )
   , m_error( QString() )
   , m_state( Mp3tunesHarmonyDaemon::DISCONNECTED )
{
    /* g_type_init required for using the GObjects for Harmony. */
    g_type_init();

    m_harmony = mp3tunes_harmony_new();

    /* Set the error signal handler. */
    g_signal_connect(m_harmony, "error", G_CALLBACK(signalErrorHandler), 0);
    /* Set the state change signal handler. */
    g_signal_connect(m_harmony, "state_change", G_CALLBACK(signalStateChangeHandler), 0);
    /* Set the download signal handler. */
    g_signal_connect(m_harmony, "download_ready", G_CALLBACK(signalDownloadReadyHandler), 0);
    g_signal_connect(m_harmony, "download_pending", G_CALLBACK(signalDownloadPendingHandler), 0);

    mp3tunes_harmony_set_identifier(m_harmony, m_identifier);

    mp3tunes_harmony_set_device_attribute(m_harmony, "device-description", "Amarok 2 Test Daemon");

}

Mp3tunesHarmonyDaemon::Mp3tunesHarmonyDaemon( char* identifier, char* email, char* pin ) :
    m_identifier( identifier )
   , m_gerr( 0 )
   , m_error( QString() )
   , m_state( Mp3tunesHarmonyDaemon::DISCONNECTED )
{
    DEBUG_BLOCK
    /* g_type_init required for using the GObjects for Harmony. */
    g_type_init();

    m_harmony = mp3tunes_harmony_new();

    /* Set the error signal handler. */
    g_signal_connect(m_harmony, "error", G_CALLBACK(signalErrorHandler), 0);
    /* Set the state change signal handler. */
    g_signal_connect(m_harmony, "state_change", G_CALLBACK(signalStateChangeHandler), 0);
    /* Set the download signal handler. */
    g_signal_connect(m_harmony, "download_ready", G_CALLBACK(signalDownloadReadyHandler), 0);
    g_signal_connect(m_harmony, "download_pending", G_CALLBACK(signalDownloadPendingHandler), 0);

    mp3tunes_harmony_set_identifier(m_harmony, m_identifier);
    mp3tunes_harmony_set_email( m_harmony, email );
    mp3tunes_harmony_set_pin( m_harmony, pin );

    mp3tunes_harmony_set_device_attribute(m_harmony, "device-description", "Amarok 2 Test Daemon");

}


Mp3tunesHarmonyDaemon::~Mp3tunesHarmonyDaemon()
{

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
    mp3tunes_harmony_connect(m_harmony, &m_gerr);
    /* Check for errors on the connection */
    if (m_gerr) {
        g_print("Error: %s\n", m_gerr->message);
    }

    /* Run the main loop */
    g_main_loop_run(m_main_loop);

}

QString
Mp3tunesHarmonyDaemon::pin() const
{
    return QString( mp3tunes_harmony_get_pin( m_harmony ) );
}

QString
Mp3tunesHarmonyDaemon::email() const
{
    return QString( mp3tunes_harmony_get_email( m_harmony ) );
}


QString
Mp3tunesHarmonyDaemon::error() const
{
    return m_error;
}

void
Mp3tunesHarmonyDaemon::setState( HarmonyState state )
{
    m_state = state;
}

void
Mp3tunesHarmonyDaemon::setError( const QString &error )
{
    m_error = error;
}
void
Mp3tunesHarmonyDaemon::emitError()
{
   emit( signalError( m_error ) );
}

void
Mp3tunesHarmonyDaemon::emitWaitingForEmail()
{
    emit( signalWaitingForEmail() );
}

void
Mp3tunesHarmonyDaemon::emitWaitingForPin()
{
    emit( signalWaitingForPin() );
}

void
Mp3tunesHarmonyDaemon::emitConnected()
{
    emit( signalConnected() );
}

void
Mp3tunesHarmonyDaemon::emitDisconnected()
{
    emit( signalDisconnected() );
}

void
Mp3tunesHarmonyDaemon::emitDownloadReady( Mp3tunesHarmonyDownload download )
{
    DEBUG_BLOCK
    emit( signalDownloadReady( download ) );
}
void
Mp3tunesHarmonyDaemon::emitDownloadPending( Mp3tunesHarmonyDownload download )
{
    DEBUG_BLOCK
    emit( signalDownloadPending( download ) );
}

void
Mp3tunesHarmonyDaemon::signalErrorHandler(MP3tunesHarmony* harmony, gpointer null_pointer )
{
    DEBUG_BLOCK
    GError *err;
    null_pointer = null_pointer;
    g_print("Fatal Error: %s\n", harmony->error->message);
    theDaemon->setError( QString( harmony->error->message ) );
    theDaemon->emitError();
    mp3tunes_harmony_disconnect(harmony, &err);
    if (err) {
        g_print("Error disconnecting: %s\n", err->message);
        /* If there is an error disconnecting something has probably gone
        * very wrong and reconnection should not be attempted till the user
        * re-initiates it */
        return;
    }
}
void
Mp3tunesHarmonyDaemon::signalStateChangeHandler( MP3tunesHarmony* harmony, guint32 state,  gpointer null_pointer )
{
    DEBUG_BLOCK
    null_pointer = null_pointer;
    switch (state) {
        case MP3TUNES_HARMONY_STATE_DISCONNECTED:
            g_print("Disconnected.\n");
            theDaemon->setState( Mp3tunesHarmonyDaemon::DISCONNECTED );
            theDaemon->emitDisconnected();
            /* Do nothing here */
            break;
        case MP3TUNES_HARMONY_STATE_CONNECTED:
            g_print("Connected! Waiting for download requests!\n");
            theDaemon->setState( Mp3tunesHarmonyDaemon::CONNECTED );
            theDaemon->emitConnected();
            /* At this point, it would be best to store the pin, if you haven't
               * already, and the email in some somewhat permenant storage for
               * when reauthenticating.
               */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN:
            g_print("Connection in process!\n");
            theDaemon->setState( Mp3tunesHarmonyDaemon::WAITING_FOR_PIN );
            theDaemon->emitWaitingForPin();
            /* At this point, just update the user status. */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL:
            g_print("Please login to mp3tunes.com and add the pin '%s' to your devices.\n", mp3tunes_harmony_get_pin(harmony));
            theDaemon->setState( Mp3tunesHarmonyDaemon::WAITING_FOR_EMAIL );
            theDaemon->emitWaitingForEmail();
            /* At this point, it would be best to store the pin in case the
             * network connection drops. As well, display to the user a status
             * message to have them perform the website authentication action.
             */
            break;
    }
}

void
Mp3tunesHarmonyDaemon::signalDownloadReadyHandler( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer )
{
    DEBUG_BLOCK
    mp3tunes_harmony_download_t *download = (mp3tunes_harmony_download_t*)void_mp3tunes_harmony_download;
    Mp3tunesHarmonyDownload wrappedDownload( download );
    theDaemon->emitDownloadReady( wrappedDownload );
    harmony = harmony;
    null_pointer = null_pointer;
    mp3tunes_harmony_download_deinit(&download);
}

void
Mp3tunesHarmonyDaemon::signalDownloadPendingHandler( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer )
{
    DEBUG_BLOCK
    mp3tunes_harmony_download_t *download = (mp3tunes_harmony_download_t*)void_mp3tunes_harmony_download;
    Mp3tunesHarmonyDownload wrappedDownload( download );
    theDaemon->emitDownloadPending( wrappedDownload );
    harmony = harmony;
    null_pointer = null_pointer;
    /*if (strcmp(download->file_key, "dummy_file_key_5") == 0) {
        g_main_loop_quit(m_main_loop);
    }*/
    /*mp3tunes_harmony_download_deinit(&download);*/
}


/* Harmony Download Type Wrapper */
Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download )
{
    DEBUG_BLOCK
    m_harmony_download_t = ( mp3tunes_harmony_download_t * ) malloc( sizeof( *download ) );
    memcpy( m_harmony_download_t, download, sizeof( *download ) );

    m_harmony_download_t->artist_name = ( char * ) malloc( strlen( download->artist_name ) );
    strcpy( m_harmony_download_t->artist_name, download->artist_name );

    m_harmony_download_t->album_title = ( char * ) malloc( strlen( download->album_title ) );
    strcpy( m_harmony_download_t->album_title, download->album_title );

    m_harmony_download_t->track_title = ( char * ) malloc( strlen( download->track_title ) );
    strcpy( m_harmony_download_t->track_title, download->track_title );

    m_harmony_download_t->file_name = ( char * ) malloc( strlen( download->file_name ) );
    strcpy( m_harmony_download_t->file_name, download->file_name );

    m_harmony_download_t->file_key = ( char * ) malloc( strlen( download->file_key ) );
    strcpy( m_harmony_download_t->file_key, download->file_key );

    m_harmony_download_t->file_format = ( char * ) malloc( strlen( download->file_format ) );
    strcpy( m_harmony_download_t->file_format, download->file_format );

     m_harmony_download_t->device_bitrate = ( char * ) malloc( strlen( download->device_bitrate ) );
    strcpy( m_harmony_download_t->device_bitrate, download->device_bitrate );

    m_harmony_download_t->file_bitrate = ( char * ) malloc( strlen( download->file_bitrate ) );
    strcpy( m_harmony_download_t->file_bitrate, download->file_bitrate );

    if( download->url ) {
        m_harmony_download_t->url = ( char * ) malloc( strlen( download->url ) );
        strcpy( m_harmony_download_t->url, download->url );
    }

}

Mp3tunesHarmonyDownload::~Mp3tunesHarmonyDownload()
{
    //mp3tunes_harmony_download_deinit( &m_harmony_download_t );
}

QString
Mp3tunesHarmonyDownload::fileKey() const
{
    return QString( m_harmony_download_t->file_key );
}

QString
Mp3tunesHarmonyDownload::fileName() const
{
    return QString( m_harmony_download_t->file_name );
}

QString
Mp3tunesHarmonyDownload::fileFormat() const
{
    return QString( m_harmony_download_t->file_format );
}

unsigned int
Mp3tunesHarmonyDownload::fileSize() const
{
    return m_harmony_download_t->file_size;
}

QString
Mp3tunesHarmonyDownload::artistName() const
{
    return QString( m_harmony_download_t->artist_name );
}

QString
Mp3tunesHarmonyDownload::albumTitle() const
{
    return QString( m_harmony_download_t->album_title );
}

QString
Mp3tunesHarmonyDownload::trackTitle() const
{
    return QString( m_harmony_download_t->track_title );
}

int
Mp3tunesHarmonyDownload::trackNumber() const
{
    return m_harmony_download_t->track_number;
}

QString
Mp3tunesHarmonyDownload::deviceBitrate() const
{
    return QString( m_harmony_download_t->device_bitrate );
}

QString
Mp3tunesHarmonyDownload::fileBitrate() const
{
    return QString( m_harmony_download_t->file_bitrate );
}

QString
Mp3tunesHarmonyDownload::url() const
{
    return QString( m_harmony_download_t->url );
}
