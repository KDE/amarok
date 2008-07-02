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
#ifndef MP3TUNESHARMONYDAEMON_H
#define MP3TUNESHARMONYDAEMON_H

extern "C" {
   // Get libmp3tunes declarations
    #include "libmp3tunes/harmony.h"
    #include "libmp3tunes/locker.h"
    #include <glib.h>
    #include <sys/vfs.h>
    #include <errno.h>
    #include <stdio.h>
    #include <string.h>
}

class Mp3tunesHarmonyDaemon {

  public:
    Mp3tunesHarmonyDaemon( char* identifier);
    ~Mp3tunesHarmonyDaemon();

    void init();

  private:
    
    /* Error signal handler.
     *
     * This signal is emitted whenever there is a user fixable error from inside the
     * library. Most of these errors are from inside of the Jabber library.
     *
     * Whenever this signal handler is called, harmony->error will be set to a valid
     * GError pointer. The message field of that structure will contain the error
     * and should be displayed to the user and the connection reset by calling
     * mp3tunes_harmony_disconnect and a reconnection user initiated.
     */
    void signalErrorHandler( MP3tunesHarmony* harmony, gpointer null_pointer );

    /* State change signal handler.
     *
     * This signal is emitted whenever the state of the connection changes during
     * the Harmony authentication process. The state variable will be set to one of
     * the values of harmony_state_t with the following meanings.
     *
     * MP3TUNES_HARMONY_STATE_DISCONNECTED:
     *     The connection to the server has been disconnected. Occurs a couple of
     *     times during the authentication process. Nothing to act on unless was
     *     already in the CONNECTED state.
     *
     * MP3TUNES_HARMONY_STATE_CONNECTED:
     *     The connection completed successfully. All to be done from here is to
     *     associate the download handler if needed and wait for download messages.
     *
     * MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN:
     *     The client has authenticated and is waiting for the response to a
     *     harmonyPin message. Unless there is a problem with the Conductor this
     *     state should be left almost immediately and moved back into disconnected
     *     before going to WAITING_FOR_EMAIL. Useful for having a progress message
     *     during authentication.
     *
     * MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL:
     *     The client has authenticated and is waiting for a response to the
     *     harmonyEmail message. In this state, the action to take is to display to
     *     the user the pin, found by calling mp3tunes_harmony_get_pin, and request
     *     for them to log into mp3tunes.com and have them add the pin to their
     *     devices tab. Upon receiving the reply to this you will be authenticated.
     */
    void signalStateChangeHandler( MP3tunesHarmony* harmony, guint32 state,  gpointer null_pointer );

    
    void signalDownloadReady( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer );
    void signalDownloadPending( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer );

    MP3tunesHarmony* m_harmony;
    GMainLoop *m_main_loop;
    GError *m_err;
    char* m_identifier;

};

#endif

