/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2010-2011 Ralf Engels <ralf-engels@gmx.de>                             *
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "GenericScanManager.h"

#include "App.h"
#include <core/support/Debug.h>

// include files from the collection scanner utility
#include <utilities/collectionscanner/BatchFile.h>
#include <utilities/collectionscanner/CollectionScanner.h>

#include <KStandardDirs>
#include <threadweaver/ThreadWeaver.h>

#include <QFileInfo>
#include <QSharedMemory>
#include <QThread>

static const int SHARED_MEMORY_SIZE = 1024 * 1024; // 1 MB shared memory

GenericScanManager::GenericScanManager( QObject *parent )
    : QObject( parent )
    , m_scannerJob( 0 )
{
    //TODO: folder watching
    //TODO: incremental scanning
    //TODO: import
}

GenericScanManager::~GenericScanManager()
{
    blockScan();
}

void
GenericScanManager::blockScan()
{
    {
        QMutexLocker locker( &m_mutex );
        m_blockCount++;
    }
    abort( "Scan blocked" );
}

void
GenericScanManager::unblockScan()
{
    {
        QMutexLocker locker( &m_mutex );
        m_blockCount--;
    }
    //TODO: restart scanner
}

bool
GenericScanManager::isRunning()
{
    return m_scannerJob;
}

void
GenericScanManager::requestFullScan( QList<KUrl> directories )
{
    if( m_scannerJob )
    {
        //TODO: add to queue
        error() << "Scanner already running";
        return;
    }

    QSet<QString> scanDirsSet;
    foreach( const KUrl &url, directories )
    {
        if( !url.isLocalFile() )
        {
            error() << "scan of non-local directory requested. Skipping.";
            continue;
        }

        QString path = url.path( KUrl::RemoveTrailingSlash );
        if( !QFileInfo( path ).isDir() )
        {
            error() << "scan of a file requested. Skipping.";
            continue;
        }
        //TODO: most local path

        scanDirsSet << path;
    }

    m_scannerJob = new GenericScannerJob( this, scanDirsSet.toList() );

    //TODO:connect message signal
    connect( m_scannerJob, SIGNAL(done( ThreadWeaver::Job* )), SLOT(slotJobDone()) );
    connect( m_scannerJob, SIGNAL(failed( QString )), SIGNAL(failed( QString )) );
    connect( m_scannerJob, SIGNAL(directoryScanned( CollectionScanner::Directory * )),
             SIGNAL(directoryScanned( CollectionScanner::Directory * )),
             Qt::DirectConnection );

    ThreadWeaver::Weaver::instance()->enqueue( m_scannerJob );
}

void
GenericScanManager::abort( const QString &reason )
{
    QMutexLocker locker( &m_mutex );

    if( !reason.isEmpty() )
        debug() << "Abort scan: " << reason;

    if( m_scannerJob )
        m_scannerJob->requestAbort();
    // TODO: For testing it would be good to specify the scanner in the build directory
}

void
GenericScanManager::slotJobDone()
{
    debug() << "Scanner job done";
    QMutexLocker locker( &m_mutex );
    if( !m_scannerJob )
        return;

    //TODO: reported error handling
    //if( errors )
    // emit failed()
    emit succeeded();

    m_scannerJob->deleteLater();
    m_scannerJob = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class GenericScannerJob
///////////////////////////////////////////////////////////////////////////////

QString
GenericScannerJob::scannerPath()
{
    QString path = KStandardDirs::locate( "exe", "amarokcollectionscanner" );

    // If the binary is not in $PATH, then search in the application folder too
    if( path.isEmpty() )
        path = App::applicationDirPath() + QDir::separator() + "amarokcollectionscanner";

    return path;
}

GenericScannerJob::GenericScannerJob( QObject *parent, QIODevice *input )
    : ThreadWeaver::Job( parent )
    , m_input( input )
    , m_abortRequested( false )
    , m_scannerStateMemory( 0 )
{

}

GenericScannerJob::GenericScannerJob( QObject *parent, QStringList scanDirsRequested,
                                      bool recursive, bool detectCharset )
    : ThreadWeaver::Job( parent )
    , m_scanDirsRequested( scanDirsRequested )
    , m_input( 0 )
    , m_abortRequested( false )
    , m_scannerStateMemory( 0 )
    , m_recursive( recursive )
    , m_charsetDetect( detectCharset )
{
}

GenericScannerJob::~GenericScannerJob()
{
}

void
GenericScannerJob::run()
{
    if( !m_input )
    {
        m_scanner = createScannerProcess();
        if( !m_scanner )
        {
            warning() << "Unable to start Amarok collection scanner.";
            emit failed( i18n("Unable to start Amarok collection scanner." ) );
            return;
        }
        m_input = qobject_cast<QIODevice *>( m_scanner );
    }

    m_reader.setDevice( m_input );

    m_scanner->start();

    //TODO: make incremental
    m_scanner->waitForFinished( -1 );

    parseScannerOutput();
}

void
GenericScannerJob::requestAbort()
{
    QMutexLocker locker( &m_mutex );
    m_abortRequested = true;
}

KProcess *
GenericScannerJob::createScannerProcess( bool restart )
{
    if( m_abortRequested )
        return false;

    // -- create the shared memory
    if( !m_scannerStateMemory && !restart )
    {
        m_sharedMemoryKey = "AmarokScannerMemory"+QDateTime::currentDateTime().toString();
        m_scannerStateMemory = new QSharedMemory( m_sharedMemoryKey );
        if( !m_scannerStateMemory->create( SHARED_MEMORY_SIZE ) )
        {
            warning() << "Unable to create shared memory for collection scanner";
            delete m_scannerStateMemory;
            m_scannerStateMemory = 0;
        }
    }

    // -- create the scanner process
    KProcess *scanner = new KProcess( this );
    scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );

    *scanner << GenericScannerJob::scannerPath() << "--idlepriority";
    if( !m_batchfilePath.isEmpty() )
    {
        *scanner << "-i";
        *scanner << "--batch" << m_batchfilePath;
    }

    if( m_recursive )
        *scanner << "-r";

    if( m_charsetDetect )
        *scanner << "-c";

    if( restart )
        *scanner << "-s";

    if( m_scannerStateMemory )
        *scanner << "--sharedmemory" << m_sharedMemoryKey;

    //TODO: use batchfile, needed for incremental scanning

    *scanner << m_scanDirsRequested;

    return scanner;
}

void
GenericScannerJob::parseScannerOutput()
{
    bool finished = false;
    int count = 0;
    while( !m_reader.atEnd() )
    {
        // -- check if we were aborted, have finished or need to wait for new data
        {
            QMutexLocker locker( &m_mutex );
            if( m_abortRequested )
                break;
        }

        m_reader.readNext();
        if( m_reader.hasError() )
        {
            break;
        }
        else if( m_reader.isStartElement() )
        {
            QStringRef name = m_reader.name();
            if( name == "scanner" )
            {
                debug() << "ScannerJob: got count:" << m_reader.attributes().value( "count" ).toString().toInt();
                emit message( i18np("Found one directory", "Found %1 directories",
                              m_reader.attributes().value( "count" ).toString()) );
                emit totalSteps( m_reader.attributes().value( "count" ).toString().toInt() * 2 );
            }
            else if( name == "directory" )
            {
                CollectionScanner::Directory *dir = new CollectionScanner::Directory( &m_reader );
                count++;
                emit directoryScanned( dir );

                emit message( i18n( "Got directory \"%1\" from scanner.", dir->rpath() ) );
                emit incrementProgress();
            }
            else
            {
                warning() << "Unexpected xml start element"<<name<<"in input";
                m_reader.skipCurrentElement();
            }

        }
        else if( m_reader.isEndElement() )
        {
            if( m_reader.name() == "scanner" ) // ok. finished
                finished = true;
        }
        else if( m_reader.isEndDocument() )
        {
            finished = true;
        }
    }
}
