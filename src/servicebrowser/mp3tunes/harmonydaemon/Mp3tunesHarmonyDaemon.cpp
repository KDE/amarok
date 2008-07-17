/***************************************************************************
 *   Copyright (c) 2008  Casey Link <unnamedrambler@gmail.com>             *
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
#include "mp3tunesharmonydaemonadaptor.h"

#include <kcmdlineargs.h>
#include <QtDebug>

Mp3tunesHarmonyDaemon::Mp3tunesHarmonyDaemon( QString identifier )
   : QCoreApplication( KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() )
   , m_identifier( identifier )
   , m_email( QString() )
   , m_pin( QString() )
   , m_gerr( 0 )
   , m_error( QString() )
   , m_started( false )
   , m_inited( false )
   , m_state( Mp3tunesHarmonyDaemon::DISCONNECTED )
{
    allAboardTheDBus();
    init();
}

Mp3tunesHarmonyDaemon::Mp3tunesHarmonyDaemon( QString identifier, QString email, QString pin )
   : QCoreApplication( KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() )
   , m_identifier( identifier )
   , m_email( email )
   , m_pin( pin )
   , m_gerr( 0 )
   , m_error( QString() )
   , m_started( false )
   , m_inited( false )
   , m_state( Mp3tunesHarmonyDaemon::DISCONNECTED )
{
    allAboardTheDBus();
    init();
}


Mp3tunesHarmonyDaemon::~Mp3tunesHarmonyDaemon()
{}

bool
Mp3tunesHarmonyDaemon::allAboardTheDBus()
{

    setOrganizationName( "Amarok" );
    setOrganizationDomain( "amarok.kde.org" );
    setApplicationName( "Mp3tunes Harmony Daemon" );

    QDBusConnectionInterface *bus = 0;
    bus = QDBusConnection::sessionBus().interface();
    if( !bus ) {
        qFatal("No dbus!!");
        ::exit(126);
        return false;
    }
    QStringList parts = this->organizationDomain().split(QLatin1Char('.'), QString::SkipEmptyParts);
    QString reversedDomain;
    if (parts.isEmpty())
        reversedDomain = QLatin1String("local.");
    else
        foreach (const QString& s, parts)
        {
            reversedDomain.prepend(QLatin1Char('.'));
            reversedDomain.prepend(s);
        }
    const QString pidSuffix = QString::number( applicationPid() ).prepend( QLatin1String("-") );
    const QString serviceName = reversedDomain + this->applicationName().remove( " " ) + pidSuffix;
    if ( bus->registerService(serviceName) == QDBusConnectionInterface::ServiceNotRegistered ) {
        qDebug() << "FATAL: Couldn't register   name '" << serviceName << "' with DBUS - another process owns it already!" << endl;;
        ::exit(126);
        return false;
    }
    qDebug() << "Registered service: " << serviceName;
    new Mp3tunesHarmonyDaemonAdaptor( this );
    if ( QDBusConnection::sessionBus().registerObject( QLatin1String("/Mp3tunesHarmonyDaemon"), this ) )
    {
        qDebug()  << "Dbus registered";
        return true;
    } else {
        qDebug()  << "Dbus not registered";
        return false;
    }

}

bool
Mp3tunesHarmonyDaemon::daemonConnected()
{
    return m_started;
}

bool
Mp3tunesHarmonyDaemon::breakConnection()
{
    if( !daemonConnected() ) {
        qDebug()  << "Daemon not connected";
        return true;
    }
    qDebug()  << "Disconnecting Harmony";
    GError *err;
    mp3tunes_harmony_disconnect(m_harmony, &err);
    if (err) {
        qDebug()  << "Error disconnecting:  " << err->message;
        /* If there is an error disconnecting something has probably gone
        * very wrong and reconnection should not be attempted till the user
        * re-initiates it */
    }
    return true;
}

int
Mp3tunesHarmonyDaemon::init()
{

    qDebug()  << "Begin initing";
        /* g_type_init required for using the GObjects for Harmony. */
    g_type_init();

    m_harmony = mp3tunes_harmony_new();

    /* Set the error signal handler. */
    g_signal_connect( m_harmony, "error",
                      G_CALLBACK( signalErrorHandler ), this );
    /* Set the state change signal handler. */
    g_signal_connect( m_harmony, "state_change",
                      G_CALLBACK(signalStateChangeHandler ), this );
    /* Set the download signal handler. */
    g_signal_connect( m_harmony, "download_ready",
                      G_CALLBACK(signalDownloadReadyHandler ), this );
    g_signal_connect( m_harmony, "download_pending",
                      G_CALLBACK(signalDownloadPendingHandler ), this );

    qDebug()  << "Initing 1";

    mp3tunes_harmony_set_identifier( m_harmony, convertToChar( m_identifier ) );

    if( !m_email.isEmpty() )
        mp3tunes_harmony_set_email( m_harmony, convertToChar( m_email ) );
    if( !m_pin.isEmpty() )
        mp3tunes_harmony_set_pin( m_harmony, convertToChar( m_pin ) );

    mp3tunes_harmony_set_device_attribute( m_harmony, "device-description",
                                           "Amarok 2 Test Daemon");


    /* Linux specific variable for getting total and available sizes for the
     * file system
     */
    qDebug()  << "Initing 2";

    struct statfs fsstats;
    unsigned long long total_bytes;
    unsigned long long available_bytes;

    if ( statfs( ".", &fsstats ) != 0 ) {
        perror( "statfs failed" );
        return -1;
    }

    total_bytes = fsstats.f_bsize * fsstats.f_blocks;
    available_bytes = fsstats.f_bsize * fsstats.f_bavail;
    mp3tunes_harmony_set_device_attribute( m_harmony, "total-bytes", &total_bytes );
    mp3tunes_harmony_set_device_attribute( m_harmony, "available-bytes",
                                           &available_bytes );

    qDebug()  << "Done initing";
    m_inited = true;
    return 0;
}

QString
Mp3tunesHarmonyDaemon::makeConnection()
{
    if( !m_inited )
        return "Daemon not init()ed yet. Connection not attempted.";

    m_gerr = 0;
    mp3tunes_harmony_connect( m_harmony, &m_gerr );
    /* Check for errors on the connection */
    if ( m_gerr ) {
        g_print( "Error: %s\n", m_gerr->message );
    }

    if ( m_gerr ) {
        m_started = false;
        return "Error: " + QString( m_gerr->message );
    } else {
        m_started = true;
        return "All good!";
    }
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
   emit( errorSignal( m_error ) );
}

void
Mp3tunesHarmonyDaemon::emitWaitingForEmail()
{
    emit( waitingForEmail() );
}

void
Mp3tunesHarmonyDaemon::emitWaitingForPin()
{
    emit( waitingForPin() );
}

void
Mp3tunesHarmonyDaemon::emitConnected()
{
    emit( connected() );
}

void
Mp3tunesHarmonyDaemon::emitDisconnected()
{
    emit( disconnected() );
}

void
Mp3tunesHarmonyDaemon::emitDownloadReady( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about ready: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
    emit( downloadReady( download ) );
}
void
Mp3tunesHarmonyDaemon::emitDownloadPending( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about pending: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
    emit( downloadPending( download ) );
}

void
Mp3tunesHarmonyDaemon::signalErrorHandler(MP3tunesHarmony* harmony, gpointer null_pointer )
{

    GError *err = 0;
    Q_UNUSED( null_pointer )
    g_print("Fatal Error: %s\n", harmony->error->message );
    theDaemon->setError( QString( harmony->error->message ) );
    theDaemon->emitError();
    mp3tunes_harmony_disconnect(harmony, &err);
    if( err ) {
        g_print( "Error disconnecting: %s\n", err->message );
        /* If there is an error disconnecting something has probably gone
        * very wrong and reconnection should not be attempted till the user
        * re-initiates it */
        return;
    }
}
void
Mp3tunesHarmonyDaemon::signalStateChangeHandler( MP3tunesHarmony* harmony, guint32 state,
                                                 gpointer null_pointer )
{

    Q_UNUSED( null_pointer )
    switch ( state ) {
        case MP3TUNES_HARMONY_STATE_DISCONNECTED:
            g_print( "Disconnected.\n" );
            theDaemon->setState( Mp3tunesHarmonyDaemon::DISCONNECTED );
            theDaemon->emitDisconnected();
            /* Do nothing here */
            break;
        case MP3TUNES_HARMONY_STATE_CONNECTED:
            g_print( "Connected! Waiting for download requests!\n" );
            theDaemon->setState( Mp3tunesHarmonyDaemon::CONNECTED );
            theDaemon->emitConnected();
            /* At this point, it would be best to store the pin, if you haven't
               * already, and the email in some somewhat permenant storage for
               * when reauthenticating.
               */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_PIN:
            g_print( "Connection in process!\n" );
            theDaemon->setState( Mp3tunesHarmonyDaemon::WAITING_FOR_PIN );
            theDaemon->emitWaitingForPin();
            /* At this point, just update the user status. */
            break;
        case MP3TUNES_HARMONY_STATE_WAITING_FOR_EMAIL:
            g_print( "Please login to mp3tunes.com and add the pin '%s' to your devices.\n",
                     mp3tunes_harmony_get_pin( harmony ) );
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
Mp3tunesHarmonyDaemon::signalDownloadReadyHandler( MP3tunesHarmony* harmony,
                                                   gpointer void_mp3tunes_harmony_download,
                                                   gpointer null_pointer )
{

    Q_UNUSED( harmony )
    Q_UNUSED( null_pointer )
    mp3tunes_harmony_download_t *download = ( mp3tunes_harmony_download_t* )
                                            void_mp3tunes_harmony_download;
    Mp3tunesHarmonyDownload wrappedDownload( download );
    theDaemon->emitDownloadReady( wrappedDownload );
    mp3tunes_harmony_download_deinit( &download );
}

void
Mp3tunesHarmonyDaemon::signalDownloadPendingHandler( MP3tunesHarmony* harmony,
                                                     gpointer void_mp3tunes_harmony_download,
                                                     gpointer null_pointer )
{

    Q_UNUSED( harmony )
    Q_UNUSED( null_pointer )
    mp3tunes_harmony_download_t *download = ( mp3tunes_harmony_download_t* )
                                            void_mp3tunes_harmony_download;
    Mp3tunesHarmonyDownload wrappedDownload( download );
    theDaemon->emitDownloadPending( wrappedDownload );

}

char *
Mp3tunesHarmonyDaemon::convertToChar ( const QString &source ) const
{
    QByteArray b = source.toAscii();
    const char *c_tok = b.constData();
    char * ret = ( char * ) malloc ( strlen ( c_tok ) + 1 );
    strcpy ( ret, c_tok );
    return ret;
}

/* Harmony Download Type Wrapper */
Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload()
{}
Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download )
{

    m_fileKey = download->file_key;
    m_fileName = download->file_name;
    m_fileFormat = download->file_format;
    m_fileSize = download->file_size;
    m_artistName = download->artist_name ;
    if( download->album_title )
        m_albumTitle = download->album_title;
    else
        m_albumTitle = QString();
    m_trackTitle = download->track_title;
    m_trackNumber = download->track_number;
    m_deviceBitrate = download->device_bitrate;
    m_fileBitrate = download->file_bitrate;
    if( download->url )
        m_url = download->url;
    else
        m_url = QString();
}

Mp3tunesHarmonyDownload::~Mp3tunesHarmonyDownload()
{}

QString
Mp3tunesHarmonyDownload::fileKey() const
{
    return m_fileKey;
}

QString
Mp3tunesHarmonyDownload::fileName() const
{
    return m_fileName;
}

QString
Mp3tunesHarmonyDownload::fileFormat() const
{
    return m_fileFormat;
}

unsigned int
Mp3tunesHarmonyDownload::fileSize() const
{
    return m_fileSize;
}

QString
Mp3tunesHarmonyDownload::artistName() const
{
    return m_artistName;
}

QString
Mp3tunesHarmonyDownload::albumTitle() const
{
    return m_albumTitle;
}

QString
Mp3tunesHarmonyDownload::trackTitle() const
{
    return m_trackTitle;
}

int
Mp3tunesHarmonyDownload::trackNumber() const
{
    return m_trackNumber;
}

QString
Mp3tunesHarmonyDownload::deviceBitrate() const
{
    return m_deviceBitrate;
}

QString
Mp3tunesHarmonyDownload::fileBitrate() const
{
    return m_fileBitrate;
}

QString
Mp3tunesHarmonyDownload::url() const
{
    return m_url;
}
