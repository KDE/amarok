/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
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

#include "ScanManager.h"

#include "App.h"
#include "Debug.h"
#include "ScanResultProcessor.h"
#include "SqlCollection.h"
#include "SqlCollectionDBusHandler.h"
#include "amarokconfig.h"
#include "meta/MetaConstants.h"
#include "meta/MetaUtility.h"
#include "playlistmanager/PlaylistManager.h"
#include "statusbar/StatusBar.h"

#include <QFileInfo>
#include <QListIterator>
#include <QStringList>
#include <QVariant>
#include <QTextCodec>
#include <QXmlStreamAttributes>
#include <QDir>         //QDir::separator

#include <KMessageBox>
#include <KStandardDirs>

#include <threadweaver/ThreadWeaver.h>

#include <unistd.h>

static const int MAX_RESTARTS = 80;
static const int MAX_FAILURE_PERCENTAGE = 5;
static const int WATCH_INTERVAL = 60 * 1000; // = 60 seconds


ScanManager::ScanManager( QObject *parent )
    : QObject( parent )
    , m_collection( 0 )
    , m_dbusHandler( 0 )
    , m_storage( 0 )
    , m_scanner( 0 )
    , m_parser( 0 )
    , m_restartCount( 0 )
    , m_isIncremental( false )
    , m_blockScan( false )
{
    DEBUG_BLOCK

    // If Amarok is not installed in standard directory
    m_amarokCollectionScanDir = App::instance()->applicationDirPath() + QDir::separator();

    m_dbusHandler = new SqlCollectionDBusHandler( this );

    QTimer *watchFoldersTimer = new QTimer( this );
    connect( watchFoldersTimer, SIGNAL( timeout() ), SLOT( slotWatchFolders() ) );
    watchFoldersTimer->start( WATCH_INTERVAL );
}

ScanManager::~ScanManager()
{
    DEBUG_BLOCK

    stopParser();

    if( m_dbusHandler )
    {
        m_dbusHandler->deleteLater();
        m_dbusHandler = 0;
    }
}


void
ScanManager::startFullScan()
{
    DEBUG_BLOCK
    if( m_parser )
    {
        debug() << "scanner already running";
        return;
    }
    if( m_blockScan )
    {
        debug() << "scanning currently blocked";
        return;
    }

    checkTables( true );
    cleanTables();

    if( m_parser )
    {
        stopParser();
    }
    m_parser = new XmlParseJob( this, m_collection );
    m_parser->setIsIncremental( false );
    m_isIncremental = false;
    connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );

    const QString batchfileLocation( KGlobal::dirs()->saveLocation( "data", QString("amarok/"), false ) + "amarokcollectionscanner_batchfullscan.xml" );
    debug() << "Checking for batch file in " << batchfileLocation;

    if( !QFile::exists( batchfileLocation ) || !readBatchFile( batchfileLocation )  )
    {
        m_scanner = new AmarokProcess( this );
        *m_scanner << m_amarokCollectionScanDir + "amarokcollectionscanner" << "-p";
        if( AmarokConfig::scanRecursively() )
            *m_scanner << "-r";
        if( AmarokConfig::useCharsetDetector() )
            *m_scanner << "-c";
        *m_scanner << "--savelocation" << KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true );

        QStringList collectionFolders = m_collection->mountPointManager()->collectionFolders();
        *m_scanner << collectionFolders;
        m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
        connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
        connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
        connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
        m_scanner->start();
    }
}

void ScanManager::startIncrementalScan( const QString &directory )
{
    DEBUG_BLOCK
    if( m_parser )
    {
        debug() << "scanner already running";
        return;
    }
    if( m_blockScan )
    {
        debug() << "scanning currently blocked";
        return;
    }

    checkTables( false );

    QStringList dirs;
    QString batchfileLocation;
    bool batchfileExists = false;

    if( !directory.isEmpty() )
    {
        debug() << "Scanning directory " << directory;
        if( isDirInCollection( directory ) )
            dirs << directory;
        else
        {
            debug() << "Directory not in collection!";
            return;
        }
    }
    else
    {
        batchfileLocation = KGlobal::dirs()->saveLocation( "data", QString("amarok/"), false ) + "amarokcollectionscanner_batchincrementalscan.xml";
        batchfileExists = QFile::exists( batchfileLocation );
        if( batchfileExists )
            debug() << "Found batchfile in " << batchfileLocation;

        if( !batchfileExists )
        {
            dirs = getDirsToScan();

            debug() << "GOING TO SCAN:";
            if( dirs.size() > 30 )
                debug() << "(a *lot*)";
            else
            {
                foreach( const QString &dir, dirs )
                    debug() << "    " << dir;
            }

            writeBatchIncrementalInfoFile();
            if( dirs.isEmpty() )
            {
                debug() << "Scanning nothing, return.";
                return;
            }
        }
    }

    if( m_parser )
    {
        stopParser();
    }
    m_parser = new XmlParseJob( this, m_collection );
    m_parser->setIsIncremental( true );
    m_isIncremental = true;
    connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );

    if( !batchfileExists || !readBatchFile( batchfileLocation )  )
    {
        m_scanner = new AmarokProcess( this );
        *m_scanner << m_amarokCollectionScanDir + "amarokcollectionscanner" << "-i"
                << "--collectionid" << m_collection->collectionId() << "-p";
        if( AmarokConfig::scanRecursively() && directory.isEmpty() )
            *m_scanner << "-r";
        if( AmarokConfig::useCharsetDetector() )
            *m_scanner << "-c";
        *m_scanner << "--savelocation" << KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true );
        if( pApp->isNonUniqueInstance() )
            *m_scanner << "--pid" << QString::number( QApplication::applicationPid() );
        *m_scanner << "--mtime-file" << KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true )
                + "amarokcollectionscanner_batchincrementalinput.data";
        m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
        connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
        connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished() ) );
        connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
        m_scanner->start();
    }
}

bool
ScanManager::isDirInCollection( QString path )
{
    // In the database all directories have a trailing slash, so we must add that
    if( !path.endsWith( '/' ) )
        path += '/';

    const int deviceid = m_collection->mountPointManager()->getIdForUrl( path );
    const QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, path );

    const QStringList values =
            m_storage->query( QString( "SELECT changedate FROM directories WHERE dir = '%2' AND deviceid = %1;" )
            .arg( QString::number( deviceid ), m_storage->escape( rpath ) ) );

    //debug() << "dir " << rpath << " is in collection? " << !values.isEmpty();
    return !values.isEmpty();
}

bool
ScanManager::isFileInCollection( const QString &url  )
{
    const int deviceid = m_collection->mountPointManager()->getIdForUrl( url );
    const QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, url );

    QString sql = QString( "SELECT id FROM urls WHERE rpath = '%2' AND deviceid = %1" )
            .arg( QString::number( deviceid ),  m_storage->escape( rpath ) );

    if ( deviceid == -1 )
    {
        sql += ';';
    }
    else
    {
        QString rpath2 = '.' + url;
        sql += QString( " OR rpath = '%1' AND deviceid = -1;" )
                .arg( m_storage->escape( rpath2 ) );
    }

    const QStringList values = m_storage->query( sql );

    //debug() << "File " << rpath << " is in collection? " << !values.isEmpty();
    return !values.isEmpty();
}

void
ScanManager::setBlockScan( bool blockScan )
{
    m_blockScan = blockScan;
    if( m_parser )
    {
        warning() << "Scanner is running while scan got blocked";
    }
    //TODO what happens if the collection scanner is currently running?
}

void
ScanManager::setCollection( SqlCollection *collection )
{
    m_collection = collection;
    m_dbusHandler->setCollection( collection );
}

void
ScanManager::slotWatchFolders()
{
    if( AmarokConfig::monitorChanges() )
        startIncrementalScan();
}

void
ScanManager::slotReadReady()
{
    QByteArray line;
    QString newData;
    if( m_scanner )
        line = m_scanner->readLine();

    while( !line.isEmpty() )
    {
        // amarokcollectionscanner outputs UTF-8 regardless of local encoding
        QString data = QTextCodec::codecForName( "UTF-8" )->toUnicode( line );
        if( !data.startsWith( "exepath=" ) ) // skip binary location info from scanner
            newData += data;
        line = m_scanner->readLine();
    }
    //debug() << "Parsing all the following data:\n" << newData;
    if( m_parser )
        m_parser->addNewXmlData( newData );
}

void
ScanManager::slotFinished( )
{
    DEBUG_BLOCK

    //make sure that we read the complete buffer
    slotReadReady();
    m_scanner->deleteLater();
    m_scanner = 0;
    m_restartCount = 0;
}

void
ScanManager::slotError( QProcess::ProcessError error )
{
    DEBUG_BLOCK

    debug() << "Error: " << error;

    if( error == QProcess::Crashed )
    {
        handleRestart();
    }
    else
        abort( "Unknown error: reseting scan manager state" );
}

void
ScanManager::abort( const QString &reason )
{
    if( !reason.isEmpty() )
        debug() << "Scan error: " << reason;
    else
        debug() << "Unknown error: reseting scan manager state";

    slotReadReady(); //make sure that we read the complete buffer

    disconnect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    disconnect( m_scanner, SIGNAL( finished( int ) ), this, SLOT( slotFinished(  ) ) );
    disconnect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), this, SLOT( slotError( QProcess::ProcessError ) ) );
    m_scanner->deleteLater();
    m_scanner = 0;

    stopParser();
}

bool
ScanManager::readBatchFile( QString fileLocation )
{
    DEBUG_BLOCK
    QFile file( fileLocation );
    if( !file.open( QIODevice::ReadOnly ) )
    {
        debug() << "Couldn't open batchscan file, which does exist";
        return false;
    }

    QByteArray data;
    data = file.readAll();

    if( !data.isEmpty() )
    {
        // amarokcollectionscanner outputs UTF-8 regardless of local encoding
        QString newData = QTextCodec::codecForName( "UTF-8" )->toUnicode( data );
        QStringList splitData = newData.split( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>", QString::SkipEmptyParts );
        debug() << "splitData.size = " << splitData.size();
        if( splitData.size() > 1 )
        {
            splitData.first().chop( 11 );
            int n = 1;
            while( n < splitData.size() - 1 )
            {
                splitData[n].remove( 0, 9 );
                splitData[n].chop( 11 );
                n++;
            }
            splitData[splitData.size()-1].remove( 0, 9 );
        }
        splitData.first().prepend( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" );
        QString joinedData = splitData.join( QString() );
        if( m_parser )
            m_parser->addNewXmlData( joinedData );
    }
    else
        debug() << "Empty read from file!";

    file.close();
    QFile::remove( fileLocation );
    return true;
}

QStringList
ScanManager::getDirsToScan()
{
    DEBUG_BLOCK

    m_incrementalDirs.clear();
    const IdList list = m_collection->mountPointManager()->getMountedDeviceIds();
    QString deviceIds;
    foreach( int id, list )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number( id );
    }

    const QStringList values = m_storage->query(
            QString( "SELECT id, deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" )
            .arg( deviceIds ) );

    QList<int> changedFolderIds;
    QList<int> deletedFolderIds;

    QStringList result;
    for( QListIterator<QString> iter( values ); iter.hasNext(); )
    {
        int id = iter.next().toInt();
        int deviceid = iter.next().toInt();
        const QString folder = m_collection->mountPointManager()->getAbsolutePath( deviceid, iter.next() );
        const uint mtime = iter.next().toUInt();

        QFileInfo info( folder );
        if( info.exists() )
        {
            m_incrementalDirs << QString( folder + "_AMAROKMTIME_" + QString::number( mtime ) );
            if( info.lastModified().toTime_t() != mtime )
                changedFolderIds << id;
        }
        else
        {
            // this folder has been removed
            changedFolderIds << id;
            deletedFolderIds << id;
        }
    }
    {
        QString ids;
        foreach( int id, changedFolderIds )
        {
            if( !ids.isEmpty() )
                ids += ',';
            ids += QString::number( id );
        }
        if( !ids.isEmpty() )
        {
            QString query = QString( "SELECT id FROM urls WHERE directory IN ( %1 );" ).arg( ids );
            QStringList urlIds = m_storage->query( query );
            ids.clear();
            foreach( const QString &id, urlIds )
            {
                if( !ids.isEmpty() )
                    ids += ',';
                ids += id;
            }
            if( !ids.isEmpty() )
            {
                QString sql = QString( "DELETE FROM tracks WHERE url IN ( %1 );" ).arg( ids );
                m_storage->query( sql );
            }
        }
    }
    {
        QString ids;
        foreach( int id, deletedFolderIds )
        {
            if( !ids.isEmpty() )
                ids += ',';
            ids += QString::number( id );
        }
        if( !ids.isEmpty() )
        {
            QString sql = QString( "DELETE FROM directories WHERE id IN ( %1 );" ).arg( ids );
            m_storage->query( sql );
        }
    }
    //debug() << "Scanning the following dirs: " << result;
    return m_incrementalDirs;
}

void
ScanManager::slotJobDone()
{
    m_parser->deleteLater();
    m_parser = 0;
}

void
ScanManager::handleRestart()
{
    DEBUG_BLOCK

    m_restartCount++;
    debug() << "Collection scanner crashed, restart count is " << m_restartCount;

    slotReadReady(); //make sure that we read the complete buffer

    disconnect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    disconnect( m_scanner, SIGNAL( finished( int ) ), this, SLOT( slotFinished(  ) ) );
    disconnect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), this, SLOT( slotError( QProcess::ProcessError ) ) );
    m_scanner->deleteLater();
    m_scanner = 0;

    if( m_restartCount >= MAX_RESTARTS )
    {
        KMessageBox::error( 0, i18n( "<p>Sorry, the collection scan had to be aborted.</p><p>Too many errors were encountered during the scan.</p>" ),
                            i18n( "Collection Scan Error" ) );
        stopParser();
        return;
    }

    QTimer::singleShot( 0, this, SLOT( restartScanner() ) );
}

void
ScanManager::restartScanner()
{
    DEBUG_BLOCK

    m_scanner = new AmarokProcess( this );
    *m_scanner << m_amarokCollectionScanDir + "amarokcollectionscanner";
    if( m_isIncremental )
    {
        *m_scanner << "-i" << "--collectionid" << m_collection->collectionId();
        if( pApp->isNonUniqueInstance() )
            *m_scanner << "--pid" << QString::number( QApplication::applicationPid() );
    }

    *m_scanner << "-s"; // "--restart"
    m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
    connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
    connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
    m_scanner->start();
}

void
ScanManager::cleanTables()
{
    DEBUG_BLOCK
    m_storage->query( "DELETE FROM tracks;" );
    m_storage->query( "DELETE FROM genres;" );
    m_storage->query( "DELETE FROM years;" );
    m_storage->query( "DELETE FROM composers;" );
    m_storage->query( "DELETE FROM albums;" );
    m_storage->query( "DELETE FROM artists;" );
    //images table is deleted in DatabaseUpdater::copyToPermanentTables
}

void
ScanManager::checkTables( bool full )
{
    DEBUG_BLOCK

    int checkCount = 0; //0 is the default in amarokrc
    if( !full )
    {
        checkCount = Amarok::config( "MySQL" ).readEntry( "CheckCount" ).toInt();
        if( checkCount != 0 //always run a full one if you don't know the status
            && checkCount < 30 ) //every 30 mins seems like a good amount
        {
            checkCount++;
            Amarok::config( "MySQL" ).writeEntry( "CheckCount", QString::number( checkCount ) );
            return;
        }
    }

    DatabaseUpdater *dbUpdater = m_collection->dbUpdater();
    if( !dbUpdater )
    {
        debug() << "WOAH, why is there no DB updater?";
        return;
    }

    dbUpdater->checkTables( checkCount == 0 );

    Amarok::config( "MySQL" ).writeEntry( "CheckCount", 1 );
}

void
ScanManager::stopParser()
{
    DEBUG_BLOCK

    if( m_parser )
    {
        ThreadWeaver::Weaver::instance()->dequeue( m_parser );

        m_parser->requestAbort();
        while( !m_parser->isFinished() )
            usleep( 100000 ); // Sleep 100 msec

        m_parser->deleteLater();
        m_parser = 0;
    }
}

void
ScanManager::writeBatchIncrementalInfoFile()
{
    DEBUG_BLOCK
    QString fileName = KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true )
        + "amarokcollectionscanner_batchincrementalinput.data";
    QFile incrementalFile( fileName );
    if( incrementalFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        QTextStream stream( &incrementalFile );
        stream.setCodec( QTextCodec::codecForName("UTF-8") );
        stream << m_incrementalDirs.join( "\n" );
        incrementalFile.close();
    }
}

///////////////////////////////////////////////////////////////////////////////
// class XmlParseJob
///////////////////////////////////////////////////////////////////////////////

XmlParseJob::XmlParseJob( ScanManager *parent, SqlCollection *collection )
    : ThreadWeaver::Job( parent )
    , m_collection( collection )
    , m_abortRequested( false )
    , m_isIncremental( false )
{
    DEBUG_BLOCK
    if( The::statusBar() )
        The::statusBar()->newProgressOperation( this, i18n( "Scanning music" ) )
        ->setAbortSlot( parent, SLOT( abort() ) );

    connect( this, SIGNAL( incrementProgress() ), The::statusBar(), SLOT( incrementProgress() ), Qt::QueuedConnection );
}

XmlParseJob::~XmlParseJob()
{
    DEBUG_BLOCK
    if( The::statusBar() )
        The::statusBar()->endProgressOperation( this );
}

void
XmlParseJob::setIsIncremental( bool incremental )
{
    m_isIncremental = incremental;
}

void
XmlParseJob::run()
{
    DEBUG_BLOCK

    QList<QVariantMap> directoryData;
    bool firstTrack = true;
    QString currentDir;

    ScanResultProcessor processor( m_collection );
    processor.setSqlStorage( m_collection->sqlStorage() );
    if( m_isIncremental )
        processor.setScanType( ScanResultProcessor::IncrementalScan );
    else
        processor.setScanType( ScanResultProcessor::FullScan );

    do
    {
        m_abortMutex.lock();
        const bool abort = m_abortRequested;
        m_abortMutex.unlock();

        if( abort )
            break;

        //debug() << "Get new xml data or wait till new xml data is available";

        m_mutex.lock();
        if( m_nextData.isEmpty() )
            m_wait.wait( &m_mutex );

        if( m_nextData.isEmpty() )
            break;

        m_reader.addData( m_nextData );
        m_nextData.clear();
        m_mutex.unlock();

        while( !m_reader.atEnd() )
        {
            m_reader.readNext();
            if( m_reader.isStartElement() )
            {
                QStringRef localname = m_reader.name();
                if( localname == "dud" || localname == "tags" || localname == "playlist" || localname == "image" )
                {
                    emit incrementProgress();
                }

                if( localname == "dud" || localname == "compilation" )
                    processor.doneWithImages();

                if( localname == "itemcount" )
                {
                    processor.doneWithImages();
//                     debug() << "Got an itemcount with value: " << localname.toString();
                    if( The::statusBar() )
                        The::statusBar()->incrementProgressTotalSteps( this, m_reader.attributes().value( "count" ).toString().toInt() );
                }
                else if( localname == "tags" )
                {
                    processor.doneWithImages();
//                     debug() << "Parsing FILE:\n";
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    QList<QXmlStreamAttribute> list = attrs.toList();

//                     foreach( QXmlStreamAttribute l, list )
//                        debug() << " TAG: " << l.name().toString() << '\t' << l.value().toString() << '\n';
//                     debug() << "End FILE";

                    //NOTE: compilation may not be used according to the comment below,
                    //but this is functionality getting moved from the collection scanner
                    //until (if ever) pure-Qt translations are supported
                    QString compilationValue = attrs.value( "compilation" ).toString();
                    if( compilationValue == "checkforvarious" )
                    {
                        if( attrs.value( "artist" ).toString() == i18n( "Various Artists" ) )
                            compilationValue = QString::number( 1 );
                        else
                            compilationValue.clear();
                    }

                    QVariantMap data;
                    data.insert( Meta::Field::URL, attrs.value( "path" ).toString() );
                    data.insert( Meta::Field::TITLE, attrs.value( "title" ).toString() );
                    data.insert( Meta::Field::ARTIST, attrs.value( "artist" ).toString() );
                    data.insert( Meta::Field::COMPOSER, attrs.value( "composer" ).toString() );
                    data.insert( Meta::Field::ALBUM, attrs.value( "album" ).toString() );
                    data.insert( Meta::Field::COMMENT, attrs.value( "comment" ).toString() );
                    data.insert( Meta::Field::GENRE, attrs.value( "genre" ).toString() );
                    data.insert( Meta::Field::YEAR, attrs.value( "year" ).toString() );
                    data.insert( Meta::Field::TRACKNUMBER, attrs.value( "track" ).toString() );
                    data.insert( Meta::Field::DISCNUMBER, attrs.value( "discnumber" ).toString() );
                    data.insert( Meta::Field::BPM, attrs.value( "bpm" ).toString() );
                    //filetype and uniqueid are missing in the fields, compilation is not used here

                    if( attrs.value( "audioproperties" ) == "true" )
                    {
                        data.insert( Meta::Field::BITRATE, attrs.value( "bitrate" ).toString() );
                        data.insert( Meta::Field::LENGTH, attrs.value( "length" ).toString() );
                        data.insert( Meta::Field::SAMPLERATE, attrs.value( "samplerate" ).toString() );
                    }
                    if( !attrs.value( "filesize" ).isEmpty() )
                        data.insert( Meta::Field::FILESIZE, attrs.value( "filesize" ).toString() );
                    if( !attrs.value( "albumgain" ).isEmpty() )
                    {
                        data.insert( Meta::Field::ALBUMGAIN, attrs.value( "albumgain" ).toString().toDouble() );
                        // peak gain is in decibels
                        if( !attrs.value( "albumpeakgain" ).isEmpty() )
                            data.insert( Meta::Field::ALBUMPEAKGAIN, attrs.value( "albumpeakgain" ).toString().toDouble() );
                    }
                    if( !attrs.value( "trackgain" ).isEmpty() )
                    {
                        data.insert( Meta::Field::TRACKGAIN, attrs.value( "trackgain" ).toString().toDouble() );
                        // peak gain is in decibels
                        if( !attrs.value( "trackpeakgain" ).isEmpty() )
                            data.insert( Meta::Field::TRACKPEAKGAIN, attrs.value( "trackpeakgain" ).toString().toDouble() );
                    }

                    data.insert( Meta::Field::UNIQUEID, attrs.value( "uniqueid" ).toString() );

                    KUrl url( data.value( Meta::Field::URL ).toString() );
                    if( firstTrack )
                    {
                        currentDir = url.directory();
                        firstTrack = false;
                    }

                    if( url.directory() == currentDir )
                    {
                        directoryData.append( data );
                    }
                    else
                    {
                        processor.processDirectory( directoryData );
                        directoryData.clear();
                        directoryData.append( data );
                        currentDir = url.directory();
                    }
                }
                else if( localname == "folder" )
                {
                    processor.doneWithImages();
//                     debug() << "Parsing FOLDER:\n";
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    QList<QXmlStreamAttribute> list = attrs.toList();

//                     foreach( QXmlStreamAttribute l, list )
//                        debug() << " ATTRIBUTE: " << l.name().toString() << '\t' << l.value().toString() << '\n';
//                     debug() << "End FOLDER";

                    const QString folder = attrs.value( "path" ).toString();
                    const QFileInfo info( folder );

                    processor.addDirectory( folder, info.lastModified().toTime_t() );
                }
                else if( localname == "playlist" )
                {
                    processor.doneWithImages();
                    //TODO check for duplicates
                    //debug() << "Saving playlist with path: " << m_reader.attributes().value( "path" ).toString();
                    The::playlistManager()->import( m_reader.attributes().value( "path" ).toString() );
                }
                else if( localname == "image" )
                {
                    //debug() << "Parsing IMAGE:\n";
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    QList<QXmlStreamAttribute> thisList = attrs.toList();

//                     foreach( QXmlStreamAttribute l, thisList )
//                        debug() << " ATTR: " << l.name().toString() << '\t' << l.value().toString() << '\n';
//                     debug() << "End IMAGE";

                    // Deserialize CoverBundle list
                    QStringList list = attrs.value( "list" ).toString().split( "AMAROK_MAGIC" );
                    QList<QPair<QString, QString> > covers;

                    // Don't iterate if the list only has one element
                    if( list.size() > 1 )
                    {
                        for( int i = 0; i < list.count(); i += 2 )
                            covers += qMakePair( list[i], list[i + 1] );

                        //debug() << "Adding image with path " << attrs.value( "path" );
                        processor.addImage( attrs.value( "path" ).toString(), covers );
                    }
                }
            }
        }
        if( m_reader.error() != QXmlStreamReader::PrematureEndOfDocumentError && m_reader.error() != QXmlStreamReader::NoError )
        {
            error() << "Collection scanner abort error: " << m_reader.error();
            //the error cannot be PrematureEndOfDocumentError, so handle an unrecoverable error here

            // At this point, most likely the scanner has crashed and is about to get restarted.
            // Reset the XML-reader and try to get new data:
            debug() << "Trying to restart the QXmlStreamReader..";
            m_reader.clear();
            continue;
        }
    } while( m_reader.error() == QXmlStreamReader::PrematureEndOfDocumentError );

    if( m_abortRequested )
    {
        debug() << "Abort requested.";
        processor.rollback();
    }
    else
    {
        debug() << "Success. Committing result to database.";
        if( !directoryData.isEmpty() )
            processor.processDirectory( directoryData );
        processor.commit();
    }
}

void
XmlParseJob::addNewXmlData( const QString &data )
{
    m_mutex.lock();
    //append the new xml data because the parser thread
    //might not have retrieved all xml data yet
    m_nextData += data;
    m_wait.wakeOne();
    m_mutex.unlock();
}

void
XmlParseJob::requestAbort()
{
    DEBUG_BLOCK

    m_abortMutex.lock();
    m_abortRequested = true;
    m_abortMutex.unlock();
    m_wait.wakeOne();
}

#include "ScanManager.moc"

