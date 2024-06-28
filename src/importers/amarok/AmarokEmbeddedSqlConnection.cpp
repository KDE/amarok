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

#include "AmarokEmbeddedSqlConnection.h"

#include "core/support/Debug.h"

#include <ThreadWeaver/Thread>

#include <QEventLoop>
#include <QFileSystemWatcher>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QStringList>
#include <QTemporaryFile>

using namespace StatSyncing;

AmarokEmbeddedSqlConnection::AmarokEmbeddedSqlConnection( const QFileInfo &mysqld,
                                                          const QDir &datadir )
    : ImporterSqlConnection()
    , m_mysqld( mysqld )
    , m_datadir( datadir )
{
    connect( &m_shutdownTimer, &QTimer::timeout,
             this, &AmarokEmbeddedSqlConnection::stopServer );
    m_shutdownTimer.setSingleShot( true );
}

AmarokEmbeddedSqlConnection::~AmarokEmbeddedSqlConnection()
{
    if( isTransaction() )
        rollback();
    stopServer();
}

QSqlDatabase
AmarokEmbeddedSqlConnection::connection()
{
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    QMutexLocker lock( &m_srvMutex );

    // The server's already running; only refresh its shutdown timer
    if( m_srv.state() == QProcess::Running )
    {
        m_shutdownTimer.start( SERVER_SHUTDOWN_AFTER );
        return QSqlDatabase::database( m_connectionName );
    }

    QTemporaryFile pidFile( QDir::temp().filePath( "amarok_importer-XXXXXX.pid" ) );
    QTemporaryFile socket( QDir::temp().filePath( "amarok_importer-XXXXXX.socket" ) );
    pidFile.open();
    socket.open();

    // Get random port in range 3307 - 65535
    const int port = ( QRandomGenerator::global()->generate() % ( 65536 - 3307 ) ) + 3307;

    QSqlDatabase::removeDatabase( m_connectionName );
    QSqlDatabase db = QSqlDatabase::addDatabase( "QMYSQL", m_connectionName );
    db.setDatabaseName  ( "amarok"    );
    db.setHostName      ( "localhost" );
    db.setUserName      ( "root"      );
    db.setPassword      ( ""          );
    db.setPort          ( port        );
    db.setConnectOptions( "UNIX_SOCKET=" + QFileInfo( socket ).absoluteFilePath() );

    if( startServer( port, QFileInfo( socket ).absoluteFilePath(),
                     QFileInfo( pidFile ).absoluteFilePath() ) )
    {
        // Give tempfiles ownership over to mysqld
        pidFile.setAutoRemove( false );
        socket.setAutoRemove( false );

        m_shutdownTimer.start( SERVER_SHUTDOWN_AFTER );
    }

    db.open();
    return db;
}

bool
AmarokEmbeddedSqlConnection::startServer( const int port, const QString &socketPath,
                                          const QString &pidPath )
{
    DEBUG_BLOCK
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    if( !m_mysqld.isExecutable() )
    {
        warning() << __PRETTY_FUNCTION__ << m_mysqld.absoluteFilePath()
                  << "is not executable";
        return false;
    }

    if( !m_datadir.isReadable() )
    {
        warning() << __PRETTY_FUNCTION__ << m_datadir.absolutePath() << "is not readable";
        return false;
    }

    QEventLoop loop;
    QFileSystemWatcher watcher;
    QTimer timer;

    // Set conditions on which we stop waiting for the startup
    connect( &timer,   &QTimer::timeout,
             &loop,    &QEventLoop::quit, Qt::QueuedConnection );
    connect( &watcher, &QFileSystemWatcher::fileChanged,
             &loop,    &QEventLoop::quit, Qt::QueuedConnection );
    connect( &m_srv,   QOverload<QProcess::ProcessError>::of( &QProcess::errorOccurred ),
             &loop,    &QEventLoop::quit, Qt::QueuedConnection );

    // Important: we use modification of pidfile as a cue that the server is ready
    // This is consistent with behavior of mysqld startup scripts
    watcher.addPath( pidPath );
    timer.start( SERVER_START_TIMEOUT );

    const QStringList args = QStringList()
         << QStringLiteral("--no-defaults")
         << QStringLiteral("--port=") + QString::number( port )
         << QStringLiteral("--datadir=") + m_datadir.absolutePath()
         << QStringLiteral("--default-storage-engine=MyISAM")
         << QStringLiteral("--skip-grant-tables")
         << QStringLiteral("--myisam-recover-options=FORCE")
         << QStringLiteral("--key-buffer-size=16777216")
         << QStringLiteral("--character-set-server=utf8")
         << QStringLiteral("--collation-server=utf8_bin")
         << QStringLiteral("--skip-innodb")
         << QStringLiteral("--bind-address=localhost")
         << QStringLiteral("--socket=") + socketPath
         << QStringLiteral("--pid-file=") + pidPath;

    m_srv.start( m_mysqld.absoluteFilePath(), args );
    debug() << __PRETTY_FUNCTION__ << m_mysqld.absoluteFilePath() + ' ' + args.join(QLatin1Char(' '));

    // Wait for any of the startup conditions to be true
    loop.exec();

    if( m_srv.state() != QProcess::Running )
    {
        warning() << __PRETTY_FUNCTION__ << "error starting server application:"
                  << m_srv.errorString();
        return false;
    }

    return true;
}

void
AmarokEmbeddedSqlConnection::stopServer()
{
    DEBUG_BLOCK
    Q_ASSERT( this->thread() == ThreadWeaver::Thread::currentThread() );

    QMutexLocker lock( &m_srvMutex );
    if( isTransaction() || m_srv.state() == QProcess::NotRunning )
        return;

    m_shutdownTimer.stop();
    QSqlDatabase::removeDatabase( m_connectionName );

    m_srv.terminate();
    if( !m_srv.waitForFinished() )
    {
        m_srv.kill();
        m_srv.waitForFinished();
    }
}
