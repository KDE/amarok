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

#include <QObject>
#include <QString>
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

/**
 * A wrapper class for the libmp3tunes harmony download object.
 * It contains metadata for new tracks.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesHarmonyDownload {
    public:
        Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download );
        ~Mp3tunesHarmonyDownload();

        QString fileKey() const;
        QString fileName() const;
        QString fileFormat() const;
        unsigned int fileSize() const;
        QString artistName() const;
        QString albumTitle() const;
        QString trackTitle() const;
        int trackNumber() const;
        QString deviceBitrate() const;
        QString fileBitrate() const;
        QString url() const;
    private:
        mp3tunes_harmony_download_t *m_harmony_download_t;
};
/**
 * A daemon that receives notfications from mp3tunes'
 * servers about new/changed tracks that can be synced.
 *
 * Because this classes implements libmp3tunes which uses
 * GLIB's c-style callbacks, all the callbacks must be static.
 * This requires some black magic on my part to keep the OO
 * facade intact. The solution is a global variable: theDaemon
 * delcared after this class. Your code must #define DEFINE_HARMONY
 * and instantiate a new Mp3tunesHarmonyDaemon for theDaemon.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class Mp3tunesHarmonyDaemon: public QObject
{
    Q_OBJECT

  public:
    Mp3tunesHarmonyDaemon( char* identifier);
    ~Mp3tunesHarmonyDaemon();

    /**
     * Stats the daemon by intiating the connection Harmony connection.
     */
    void init();

    /**
     * Returns the pin
     */
    QString pin() const;

    /**
     * Returns the latest error message.
     */
    QString error() const;

    /**
     * The possible states the daemon can be in.
     * Before init() it is DISONNECTED
     */
    enum HarmonyState {
        DISCONNECTED,
        CONNECTED,
        WAITING_FOR_PIN,
        WAITING_FOR_EMAIL
    };

    /*
     * Used by the static callbacks
     * DO NOT CALL THESE METHODS
     */
    void setState( HarmonyState state );
    void setError( const QString &error );

    /*
     * Used by the static callbacks
     * DO NOT CALL THESE METHODS
     */
    void emitError();
    void emitWaitingForEmail();
    void emitWaitingForPin();
    void emitConnected();
    void emitDisconnected();
    void emitDownloadReady( Mp3tunesHarmonyDownload download );
    void emitDownloadPending( Mp3tunesHarmonyDownload download );

  signals:
      /* The actual signals that get emitted */
      void signalWaitingForEmail();
      void signalWaitingForPin();
      void signalConnected();
      void signalDisconnected();
      void signalError( const QString &error );

     /* signalDownloadReady
      * this signal is emitted when a track is ready to be downloaded.
      */
      void signalDownloadReady( Mp3tunesHarmonyDownload download );

      /* signalDownloadPending
       * this signal is emitted as soon as a download message is received.
       * it may or may not be ready. the library sends this signal before
       * adding the download to its own queue
       */
      void signalDownloadPending( Mp3tunesHarmonyDownload download );

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
    static void signalErrorHandler( MP3tunesHarmony* harmony, gpointer null_pointer );

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
    static void signalStateChangeHandler( MP3tunesHarmony* harmony, guint32 state,  gpointer null_pointer );


    /* signalDownloadReadyHandler
     * this signal is emitted when a track is ready to be downloaded.
     */
    static void signalDownloadReadyHandler( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer );

    /* signalDownloadPendingHandler
     * this signal is emitted as soon as a download message is received.
     * it may or may not be ready. the library sends this signal before
     * adding the download to its own queue
     */
    static void signalDownloadPendingHandler( MP3tunesHarmony* harmony, gpointer void_mp3tunes_harmony_download, gpointer null_pointer );

    MP3tunesHarmony* m_harmony;
    static GMainLoop * m_main_loop;
    char* m_identifier;
    GError *m_gerr;

    QString m_error;

    HarmonyState m_state;
};


/*
 * The global variable used by the static callbacks.
 * G_CALLBACK() requires a pointer to a member function,
 * and because this is c++ that member function must be static.
 * However, since I want to edit non-static members in those
 * callbacks I created a workaround by defining a global variable,
 * 'theDaemon', which is an instantiation of a Mp3tunesHarmonyDaemon
 * and call mutators on it from the static callbacks.
 */
#ifdef DEFINE_HARMONY
#define HARMONY
#else
#define HARMONY extern
#endif

HARMONY Mp3tunesHarmonyDaemon* theDaemon;

#endif

