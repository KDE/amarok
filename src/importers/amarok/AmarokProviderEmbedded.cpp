/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "AmarokProviderEmbedded.h"

#include "MetaValues.h"
#include "core/support/Debug.h"

#include <QDir>
#include <QMutexLocker>
#include <QProcess>
#include <QSqlDatabase>
#include <QWaitCondition>

using namespace StatSyncing;

AmarokProviderEmbedded::AmarokProviderEmbedded( const QVariantMap &config, ImporterManager *importer )
    : AmarokProvider( config, importer )
    , m_socket( QDir::temp().filePath( "amarok_importer-XXXXXX.socket" ) )
    , m_pidFile( QDir::temp().filePath( "amarok_importer-XXXXXX.pid" ) )
    , m_port( (qrand() % (65536 - 3307)) + 3307 ) // Get random port in range 3307 - 65535
    , m_srv( 0 )
{
    m_socket.open();
    m_pidFile.open();

    QSqlDatabase db = QSqlDatabase::database( m_config.value( "uid" ).toString(), /*open*/ false );
    db.setConnectOptions( "UNIX_SOCKET=" + QFileInfo( m_socket ).absoluteFilePath() );
    db.setDatabaseName  ( "amarok" );
    db.setHostName      ( "localhost" );
    db.setUserName      ( "root" );
    db.setPassword      ( "" );
    db.setPort          ( m_port );

    connect( &m_shutdownTimer, SIGNAL(timeout()), SLOT(stopServer()) );
    m_shutdownTimer.setSingleShot( true );
}

AmarokProviderEmbedded::~AmarokProviderEmbedded()
{
    stopServer();
}

QSet<QString>
AmarokProviderEmbedded::artists()
{
    startServer();
    return AmarokProvider::artists();
}

TrackList
AmarokProviderEmbedded::artistTracks( const QString &artistName )
{
    startServer();
    return AmarokProvider::artistTracks( artistName );
}

void
AmarokProviderEmbedded::startServer()
{
    DEBUG_BLOCK
    QMutexLocker lock( &m_srvMutex );

    // The server's already running
    if( m_srv )
    {
        // Restart timer
        m_shutdownTimer.start( SERVER_SHUTDOWN_AFTER );
        return;
    }

    QFileInfo mysqld( m_config["mysqlBinary"].toString() );
    QDir datadir( m_config["dbPath"].toString() );

    if( !mysqld.isExecutable() || !datadir.isReadable() )
        return;

    QStringList args;
    args << "--no-defaults"
         << "--port=" + QString::number( m_port )
         << "--datadir=" + datadir.absolutePath()
         << "--default-storage-engine=MyISAM"
         << "--skip-grant-tables"
         << "--myisam-recover-options=FORCE"
         << "--key-buffer-size=16777216"
         << "--character-set-server=utf8"
         << "--collation-server=utf8_bin"
         << "--bind-address=localhost"
         << "--socket=" + QFileInfo( m_socket ).absoluteFilePath()
         << "--pid-file=" + QFileInfo( m_pidFile ).absoluteFilePath();

    m_srv = new QProcess;
    m_srv->start( mysqld.absoluteFilePath(), args );
    debug() << __PRETTY_FUNCTION__ << mysqld.absoluteFilePath() + " " + args.join(" ");

    if( !m_srv->waitForStarted() )
    {
        warning() << __PRETTY_FUNCTION__ << m_srv->errorString();
        delete m_srv;
        m_srv = 0;
    }
    else // give the server a moment to initialize
    {
        QMutex mutex;
        QWaitCondition condition;
        condition.wait( &mutex, SERVER_START_WAIT );
        m_shutdownTimer.start( SERVER_SHUTDOWN_AFTER );
    }
}

void
AmarokProviderEmbedded::stopServer()
{
    DEBUG_BLOCK
    QMutexLocker lock( &m_srvMutex );

    if( !m_srv )
        return;

    m_srv->terminate();
    if( !m_srv->waitForFinished() )
        m_srv->kill();

    delete m_srv;
    m_srv = 0;
}
