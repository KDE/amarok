/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef MP3TUNESHARMONYDAEMON_H
#define MP3TUNESHARMONYDAEMON_H

#include "Mp3tunesHarmonyDownload.h"
#include "Mp3tunesHarmonyClient.h"
#include <QCoreApplication>

#include <QObject>
#include <QString>

extern "C" {
   // Get libmp3tunes declarations
    #include "../libmp3tunes/harmony.h"
    #include "../libmp3tunes/locker.h"
    #include <glib.h>
    #include <sys/statvfs.h>
    #include <errno.h>
    #include <stdio.h>
    #include <string.h>
}

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
class Mp3tunesHarmonyDaemon : public QCoreApplication
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.amarok.Mp3tunesHarmonyDaemon")

  public:
    /**
     * For the first time run, before we have an email and pin to authenticate
     */
    Mp3tunesHarmonyDaemon( QString identifier );
    /**
     * For subsequent logins
     */
    Mp3tunesHarmonyDaemon( QString identifier, QString email, QString pin );
    ~Mp3tunesHarmonyDaemon();

    /**
     * Stats the daemon by intiating the connection Harmony connection.
     */
    int init();

    /**
     * Sets the client type that the daemon will send signals to.
     */
    void setClient( Mp3tunesHarmonyClient *client );

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
    void emitWaitingForEmail( const QString &pin );
    void emitWaitingForPin();
    void emitConnected();
    void emitDisconnected();
    void emitDownloadReady( const Mp3tunesHarmonyDownload &download );
    void emitDownloadPending( const Mp3tunesHarmonyDownload &download );

  signals:
      /* The actual signals that get emitted */
      void waitingForEmail( const QString &pin );
      void waitingForPin();
      void connected();
      void disconnected();
      void errorSignal( const QString &error );

     /* signalDownloadReady
      * this signal is emitted when a track is ready to be downloaded.
      */
      void downloadReady( const Mp3tunesHarmonyDownload &download );

      /* signalDownloadPending
       * this signal is emitted as soon as a download message is received.
       * it may or may not be ready. the library sends this signal before
       * adding the download to its own queue
       */
      void downloadPending( const Mp3tunesHarmonyDownload &download );
  public slots:
      /**
     * Returns the pin
     */
    QString pin() const;

    /**
     * Returns the Harmony Email used for authentication
     */
    QString email() const;

    /**
     * Returns the latest error message.
     */
    QString error() const;

    /**
     * Determines if the daemon is currently connected to the harmony servers.
     * @return true if the daemon is connected.
     *         false if the daemon is not connected.
     */
    bool daemonConnected();

    /**
     * Disconnects the daemon if it is connected.
     * @return true if the daemon is disconnected OR if the daemon was disconnected
     *         false if the breaking the connect failed
     */
    bool breakConnection();

    QString makeConnection();

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

    /**
     * Converts a QString into a char*
     */
    char* convertToChar( const QString &source ) const;
    /**
     * Inits the D-Dbus interface.
     */
    bool allAboardTheDBus();

    MP3tunesHarmony * m_harmony;
    QString m_identifier; // the initial identifier used for authentication
    QString m_email; //used for repeat authentication
    QString m_pin; //used for repeat authentication
    GError *m_gerr; // master GError

    QString m_error; // error message to display to user
    bool m_started; // true if the connection has been established
    bool m_inited; // true if the daemon is ready to connect
    HarmonyState m_state; //current state of the harmony daemon
    Mp3tunesHarmonyClient *m_client; // the daemon client
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

