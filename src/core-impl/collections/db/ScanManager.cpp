/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "ScanManager"

#include "core-impl/collections/db/ScanManager.h"

#include "amarokconfig.h"
#include "App.h"
#include "ScanResultProcessor.h"
#include "sql/SqlCollection.h"
#include "sql/MountPointManager.h"
#include "statusbar/StatusBar.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"

// include files from the collection scanner utility
#include <collectionscanner/BatchFile.h>
#include <collectionscanner/Directory.h>

#include <QFileInfo>
#include <QListIterator>
#include <QStringList>
#include <QTextCodec>
#include <QSharedMemory>

#include <KMessageBox>
#include <KStandardDirs>

#include <threadweaver/ThreadWeaver.h>

#include <unistd.h>

static const int MAX_RESTARTS = 80;
static const int MAX_FAILURE_PERCENTAGE = 5;
static const int WATCH_INTERVAL = 60 * 1000; // = 60 seconds

static const int SHARED_MEMORY_SIZE = 1024 * 1024; // 1 MB shared memory

ScanManager::ScanManager( Collections::DatabaseCollection *collection, QObject *parent )
    : QObject( parent )
    , m_collection( static_cast<Collections::SqlCollection*>( collection ) )
    , m_scanner( 0 )
    , m_scannerStateMemory( 0 )
    , m_parser( 0 )
    , m_restartCount( 0 )
    , m_blockCount( 0 )
    , m_fullScanRequested( false )
    , m_fullScan( false )
    , m_mutex( QMutex::Recursive )
{
   // Using QTimer, so that we won't block the GUI
    QTimer::singleShot( WATCH_INTERVAL / 2, this, SLOT( slotCheckScannerVersion() ) );

   // -- rescan continuously
    QTimer *watchFoldersTimer = new QTimer( this );
    connect( watchFoldersTimer, SIGNAL( timeout() ), SLOT( slotWatchFolders() ) );
    watchFoldersTimer->start( WATCH_INTERVAL );
}

ScanManager::~ScanManager()
{
    abort();
}

void
ScanManager::blockScan()
{
    {
        QMutexLocker locker( &m_mutex );

        m_blockCount ++;

        m_fullScanRequested |= m_fullScan;
        m_scanDirsRequested.unite( m_scanDirs );
    }
    abort( "Scan blocked" );
}

void
ScanManager::unblockScan()
{
    {
        QMutexLocker locker( &m_mutex );
        m_blockCount --;
    }
    startScanner();
}

bool
ScanManager::isRunning()
{
    return m_parser || m_scanner;
}


void
ScanManager::requestFullScan()
{
    {
        QMutexLocker locker( &m_mutex );
        m_fullScanRequested = true;
        m_scanDirsRequested.unite( m_collection->mountPointManager()->collectionFolders().toSet() );
    }
    startScanner();
}


void ScanManager::requestIncrementalScan( const QString &directory )
{
    {
        QMutexLocker locker( &m_mutex );

        if( directory.isEmpty() )
            m_scanDirsRequested.unite( m_collection->mountPointManager()->collectionFolders().toSet() );
        else
        {
            if( m_collection->isDirInCollection( directory ) )
                m_scanDirsRequested.insert( directory );
            else
            {
                ; // the CollectionManager asks every collection for the scan. No harm done.
            }
        }
    }
    startScanner();
}


void ScanManager::startScanner()
{
    QMutexLocker locker( &m_mutex );

    if( !m_fullScanRequested && m_scanDirsRequested.isEmpty() )
        return; // nothing to do

    if( isRunning() )
    {
        debug() << "scanner already running";
        return;
    }
    if( m_blockCount > 0 )
    {
        debug() << "scanning currently blocked";
        return;
    }
    if( pApp->isNonUniqueInstance() )
        warning() << "Running scanner from Amarok while another Amarok instance is also running is dangerous.";

    // -- write the batch file
    m_batchfilePath = KGlobal::dirs()->saveLocation( "data", QString("amarok/"), false ) + "amarokcollectionscanner_batchscan.xml";

    while( QFile::exists( m_batchfilePath ) )
        m_batchfilePath += "_";

    CollectionScanner::BatchFile batchfile;
    batchfile.setTimeDefinitions( getKnownDirs() );
    batchfile.setDirectories( m_scanDirsRequested.toList() );
    if( !batchfile.write( m_batchfilePath ) )
    {
        warning() << "Failed to write file" << m_batchfilePath;
        m_batchfilePath.clear();
        return;
    }

    // -- what kind of scan are we actually doing?
    ScanResultProcessor::ScanType scanType;
    if( m_fullScanRequested )
        scanType = ScanResultProcessor::FullScan;
    else if( m_scanDirsRequested == m_collection->mountPointManager()->collectionFolders().toSet() )
        scanType = ScanResultProcessor::UpdateScan;
    else
        scanType = ScanResultProcessor::PartialUpdateScan;

    // -- Create the parser job
    m_parser = new XmlParseJob( this, m_collection, scanType );

    connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );

    // - connect the status bar
    if( The::statusBar() )
    {
        The::statusBar()->newProgressOperation( m_parser, i18n( "Scanning music" ) )
            ->setAbortSlot( this, SLOT( abort() ) );

        connect( m_parser, SIGNAL( totalSteps(const QObject*, int) ),
                 The::statusBar(), SLOT( setProgressTotalSteps(const QObject*, int) ),
                 Qt::QueuedConnection );
        connect( m_parser, SIGNAL( step(const QObject*) ),
                 The::statusBar(), SLOT( incrementProgress(const QObject*) ),
                 Qt::QueuedConnection );
    }

    m_restartCount = 0;

    // -- Create the scanner process
    startScannerProcess( false );

    // -- Remember the current settings
    m_fullScan = m_fullScanRequested;
    m_scanDirs = m_scanDirsRequested;

    m_fullScanRequested = false;
    m_scanDirsRequested.clear();
}

void
ScanManager::startScannerProcess( bool restart )
{
    Q_ASSERT( m_parser );
    Q_ASSERT( !m_scanner );

    // -- create the shared memory
    if( !m_scannerStateMemory && !restart )
    {
        m_sharedMemoryKey = "AmarokScannerMemory"+QDateTime::currentDateTime().toString();
        m_scannerStateMemory = new QSharedMemory( m_sharedMemoryKey, this );
        if( !m_scannerStateMemory->create( SHARED_MEMORY_SIZE ) )
        {
            warning() << "Unable to create shared memory for collection scanner";
            delete m_scannerStateMemory;
            m_scannerStateMemory = 0;
        }
    }

    // -- create the scanner process
    m_scanner = new AmarokProcess( this );
    *m_scanner << scannerPath() << "--idlepriority";
    if( !m_fullScanRequested )
        *m_scanner << "-i";

    if( AmarokConfig::scanRecursively() )
        *m_scanner << "-r";

    if( AmarokConfig::useCharsetDetector() )
        *m_scanner << "-c";

    if( restart )
        *m_scanner << "-s";

    if( m_scannerStateMemory )
        *m_scanner << "--sharedmemory" << m_sharedMemoryKey;

    *m_scanner << "--batch" << m_batchfilePath;

    m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
    connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( slotFinished( int, QProcess::ExitStatus ) ) );
    connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );

    m_scanner->start();
}

void
ScanManager::slotCheckScannerVersion()
{
    if( !AmarokConfig::monitorChanges() )
        return;

    QProcess scanner;

    scanner.start( scannerPath(), QStringList( "--version" ) );
    scanner.waitForFinished();

    const QString version = scanner.readAllStandardOutput().trimmed();
    if( version != AMAROK_VERSION  )
    {
        KMessageBox::error( 0, i18n( "<p>The version of the 'amarokcollectionscanner' tool\n"
                                     "does not match your Amarok version.</p>"
                                     "<p>Please note that Collection Scanning may not work correctly.</p>" ) );
    }
}

void
ScanManager::slotWatchFolders()
{
    if( !AmarokConfig::monitorChanges() )
        return;

    requestIncrementalScan();
}

void
ScanManager::slotReadReady()
{
    QMutexLocker locker( &m_mutex );

    if( !m_scanner || !m_parser )
        return;

    QByteArray buffer = m_scanner->readAll();

    m_parser->addNewXmlData( QTextCodec::codecForName( "UTF-8" )->toUnicode( buffer ) );
}

void
ScanManager::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED( exitCode );
    Q_ASSERT( m_scanner );

    if( exitStatus != QProcess::NormalExit )
    {
        handleRestart();
        return;
    }

    slotReadReady(); //make sure that we read the complete buffer

    {
        QMutexLocker locker( &m_mutex );
        if( m_scanner )
        {
            stopScanner();

            m_restartCount = 0;
            m_fullScan = false;
            m_scanDirs.clear();
        }
    }
}

void
ScanManager::slotError( QProcess::ProcessError error )
{
    warning() << "Error: " << error;

    if( error == QProcess::Crashed )
    {
        handleRestart();
        return;
    }

    abort( "Unknown error: reseting scan manager state" );
    // this might be QProcess::FailedToStart which is unrecoverable
}

void
ScanManager::abort( const QString &reason )
{
    if( !reason.isEmpty() )
        debug() << "Abort scan: " << reason;

    {
        QMutexLocker locker( &m_mutex );
        if( m_scanner )
        {
            stopScanner();

            m_fullScan = false;
            m_scanDirs.clear();
        }
    }

    stopParser();
}


QString
ScanManager::scannerPath() const
{
    QString path = KStandardDirs::locate( "exe", "amarokcollectionscanner" );

    // If the binary is not in $PATH, then search in the application folder too
    if( path.isEmpty() )
        path = App::applicationDirPath() + QDir::separator() + "amarokcollectionscanner";

    return path;
}


QList< QPair<QString, uint> >
ScanManager::getKnownDirs()
{
    QList< QPair<QString, uint> > result;

    // -- get all mount points
    QList<int> idlist = m_collection->mountPointManager()->getMountedDeviceIds();

    //expects a stringlist in order deviceid, dir, changedate
    QStringList values = m_collection->getDatabaseDirectories( idlist );
    for( QListIterator<QString> iter( values ); iter.hasNext(); )
    {
        int deviceid = iter.next().toInt();
        QString dir = iter.next();
        uint mtime = iter.next().toUInt();

        QString folder = m_collection->mountPointManager()->getAbsolutePath( deviceid, dir );
        result.append( QPair<QString, uint>( folder, mtime ) );
    }

    return result;
}

void
ScanManager::slotJobDone()
{
    QMutexLocker locker( &m_mutex );
    m_parser->deleteLater();
    m_parser = 0;
    stopScanner();
}

void
ScanManager::handleRestart()
{
    {
        QMutexLocker locker( &m_mutex );
        m_restartCount++;
        debug() << "Collection scanner crashed, restart count is " << m_restartCount;

        slotReadReady(); //make sure that we read the complete buffer

        disconnect( m_scanner, 0, this, 0 );
        m_scanner->deleteLater();
        m_scanner = 0;
    }

    if( m_restartCount >= MAX_RESTARTS )
    {
        KMessageBox::error( 0, i18n( "<p>Sorry, the collection scan had to be aborted.</p><p>Too many errors were encountered during the scan.</p>" ),
                            i18n( "Collection Scan Error" ) );
        stopParser();
        return;
    }

    startScannerProcess( true );
}

void
ScanManager::stopScanner()
{
    if( !m_scanner )
        return;

    // stop the scanner
    disconnect( m_scanner, 0, this, 0 );
    m_scanner->terminate();
    m_scanner->deleteLater();
    m_scanner = 0;

    // free it's shared memory.
    delete m_scannerStateMemory;
    m_scannerStateMemory = 0;

    // remove the batch file
    QFile::remove( m_batchfilePath );
}

void
ScanManager::stopParser()
{
    QMutexLocker locker( &m_mutex );
    if( m_parser )
    {
        // disconnect( m_parser, 0, this, 0 );
        m_parser->requestAbort();
        /*
        m_parser->deleteLater();
        ThreadWeaver::Weaver::instance()->dequeue( m_parser );
        m_parser = 0;
        */
   }
}


///////////////////////////////////////////////////////////////////////////////
// class XmlParseJob
///////////////////////////////////////////////////////////////////////////////

XmlParseJob::XmlParseJob( QObject *parent, Collections::DatabaseCollection *collection,
                          ScanResultProcessor::ScanType scanType )
    : ThreadWeaver::Job( parent )
    , m_collection( collection )
    , m_scanType( scanType )
    , m_abortRequested( false )
{ }

XmlParseJob::~XmlParseJob()
{ }

void
XmlParseJob::run()
{
    ScanResultProcessor *processor = m_collection->getNewScanResultProcessor( m_scanType );
    connect( processor, SIGNAL( directoryCommitted() ),
             this, SLOT( directoryCommitted() ) );

    bool finished = false;
    int count = 0;
    do
    {
        // -- check if we were aborted, have finished or need to wait for new data
        m_mutex.lock();
        if( m_abortRequested )
        {
            m_mutex.unlock();
            break;
        }
        debug() << "Waiting";
        if( m_reader.atEnd() )
            m_wait.wait( &m_mutex );

        debug() << "Scanning";
        // -- scan as many directory tags as we added to the data
        while( !m_reader.atEnd() )
        {
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
                debug() << "XmlParseJob: got count:" << m_reader.attributes().value( "count" ).toString().toInt();
                    emit totalSteps( this,
                                     m_reader.attributes().value( "count" ).toString().toInt() * 2);
                }
                else if( name == "directory" )
                {
                    m_mutex.unlock();
                    CollectionScanner::Directory *dir = new CollectionScanner::Directory( &m_reader );
                    processor->addDirectory( dir );
                    m_mutex.lock();
                    debug() << "XmlParseJob: run:"<<count<<"current path"<<dir->rpath();
                    count++;

                    emit step( this );
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
        m_mutex.unlock();

    } while( !finished &&
             (!m_reader.hasError() || m_reader.error() == QXmlStreamReader::PrematureEndOfDocumentError) );

    if( m_reader.hasError() )
    {
        warning() << "Aborting ScanManager XmlParseJob with error"<<m_reader.errorString();
        processor->rollback();
    }
    else if( m_abortRequested )
    {
        debug() << "Aborting ScanManager XmlParseJob";
        processor->rollback();
    }
    else
    {
        processor->commit();
    }
    debug() << "XmlParseJob finished";

    delete processor;
}

void
XmlParseJob::addNewXmlData( const QString &data )
{
    QMutexLocker locker(&m_mutex);

    m_incompleteTagBuffer += data;

    int index = m_incompleteTagBuffer.lastIndexOf( "</scanner>" );
    if( index >= 0 )
    {
        // append new data (we need to be locked. the reader is probalby not thread save)
        m_reader.addData( m_incompleteTagBuffer.left( index + 10 ) );
        m_incompleteTagBuffer = m_incompleteTagBuffer.mid( index + 10 );
    }
    else
    {
        index = m_incompleteTagBuffer.lastIndexOf( "</directory>" );
        if( index >= 0 )
        {
            // append new data (we need to be locked. the reader is probalby not thread save)
            m_reader.addData( m_incompleteTagBuffer.left( index + 12 ) );
            m_incompleteTagBuffer = m_incompleteTagBuffer.mid( index + 12 );
        }
    }

    debug() << "Adding";
    /*
    debug() << "XmlParseJob: addNewXmlData, new:"<<data.length()<<"incomplete:"<<m_incompleteTagBuffer.length();
    debug() << "             "<<m_incompleteTagBuffer<<"*";
    */

    if( index >= 0 )
        m_wait.wakeOne();
}

void
XmlParseJob::requestAbort()
{
    m_mutex.lock();
    m_abortRequested = true;
    m_wait.wakeOne();
    m_mutex.unlock();
}

void
XmlParseJob::directoryCommitted()
{
    emit step( this );
}


#include "ScanManager.moc"

