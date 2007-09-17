/***************************************************************************
 *   Copyright (C) 2003-2005 by The Amarok Developers                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "ScanController"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "scancontroller.h"
#include "statusbar.h"

#include <qdeepcopy.h>
#include <qfileinfo.h>
#include <qtextcodec.h>

#include <dcopref.h>
#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>

////////////////////////////////////////////////////////////////////////////////
// class ScanController
////////////////////////////////////////////////////////////////////////////////

ScanController* ScanController::currController = 0;

ScanController* ScanController::instance()
{
    return currController;
}

void ScanController::setInstance( ScanController* curr )
{
    currController = curr;
}

ScanController::ScanController( CollectionDB* parent, bool incremental, const QStringList& folders )
    : DependentJob( parent, "CollectionScanner" )
    , QXmlDefaultHandler()
    , m_scanner( new Amarok::ProcIO() )
    , m_folders( QDeepCopy<QStringList>( folders ) )
    , m_incremental( incremental )
    , m_hasChanged( false )
    , m_source( new QXmlInputSource() )
    , m_reader( new QXmlSimpleReader() )
    , m_waitingBundle( 0 )
    , m_lastCommandPaused( false )
    , m_isPaused( false )
    , m_tablesCreated( false )
    , m_scanCount( 0 )
{
    DEBUG_BLOCK

    ScanController::setInstance( this );
    m_reader->setContentHandler( this );
    m_reader->parse( m_source, true );

    connect( this, SIGNAL( scanDone( bool ) ), MountPointManager::instance(), SLOT( updateStatisticsURLs( bool ) ) );

    connect( m_scanner, SIGNAL( readReady( KProcIO* ) ), SLOT( slotReadReady() ) );

    *m_scanner << "amarokcollectionscanner";
    *m_scanner << "--nocrashhandler"; // We want to be able to catch SIGSEGV

    // KProcess must be started from the GUI thread, so we're invoking the scanner
    // here in the ctor:
    if( incremental )
    {
        setDescription( i18n( "Updating Collection" ) );
        initIncremental();
    }
    else
    {
        setDescription( i18n( "Building Collection" ) );
        *m_scanner << "-p";
        if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
        *m_scanner << m_folders;
        m_scanner->start();
    }
}


ScanController::~ScanController()
{
    DEBUG_BLOCK

    if( !isAborted() && !m_crashedFiles.empty() ) {
        KMessageBox::information( 0, i18n( "<p>The Collection Scanner was unable to process these files:</p>" ) +
                                  "<i>" + m_crashedFiles.join( "<br>" ) + "</i>",
                                  i18n( "Collection Scan Report" ) );
    }
    else if( m_crashedFiles.size() >= MAX_RESTARTS ) {
        KMessageBox::error( 0, i18n( "<p>Sorry, the Collection Scan was aborted, since too many problems were encountered.</p>" ) +
                            "<p>Advice: A common source for this problem is a broken 'TagLib' package on your computer. Replacing this package may help fixing the issue.</p>"
                            "<p>The following files caused problems:</p>" +
                            "<i>" + m_crashedFiles.join( "<br>" ) + "</i>",
                            i18n( "Collection Scan Error" ) );
    }

    m_scanner->kill();
    delete m_scanner;
    delete m_reader;
    delete m_source;
    ScanController::setInstance( 0 );
}


// Cause the CollectionDB to emit fileDeleted() signals
void
ScanController::completeJob( void )
{
    m_fileMapsMutex.lock();

    QMap<QString,QString>::Iterator it;
    if( !m_incremental )
    {
        CollectionDB::instance()->emitFilesAdded( m_filesAdded );
    }
    else
    {
        for( it = m_filesAdded.begin(); it != m_filesAdded.end(); ++it )
        {
            if( m_filesDeleted.contains( it.key() ) )
                m_filesDeleted.remove( it.key() );
        }
        for( it = m_filesAdded.begin(); it != m_filesAdded.end(); ++it )
            CollectionDB::instance()->emitFileAdded( it.data(), it.key() );
        for( it = m_filesDeleted.begin(); it != m_filesDeleted.end(); ++it )
            CollectionDB::instance()->emitFileDeleted( it.data(), it.key() );
    }

    m_fileMapsMutex.unlock();

    emit scanDone( !m_incremental || m_hasChanged );

    ThreadManager::DependentJob::completeJob();
}


/**
 * The Incremental Scanner works as follows: Here we check the mtime of every directory in the "directories"
 * table and store all changed directories in m_folders.
 *
 * These directories are then scanned in CollectionReader::doJob(), with m_recursively set according to the
 * user's preference, so the user can add directories or whole directory trees, too. Since we don't want to
 * rescan unchanged subdirectories, CollectionReader::readDir() checks if we are scanning recursively and
 * prevents that.
 */
void
ScanController::initIncremental()
{
    DEBUG_BLOCK

    connect( CollectionDB::instance(),
             SIGNAL( fileMoved( const QString &, const QString & ) ),
             SLOT( slotFileMoved( const QString &, const QString & ) ) );
    connect( CollectionDB::instance(),
             SIGNAL( fileMoved( const QString &, const QString &, const QString & ) ),
             SLOT( slotFileMoved( const QString &, const QString & ) ) );

    IdList list = MountPointManager::instance()->getMountedDeviceIds();
    QString deviceIds;
    foreachType( IdList, list )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number(*it);
    }

    const QStringList values = CollectionDB::instance()->query(
            QString( "SELECT deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" )
            .arg( deviceIds ) );

    foreach( values )
    {
        int id = (*it).toInt();
        const QString folder = MountPointManager::instance()->getAbsolutePath( id, (*++it) );
        const QString mtime  = *++it;

        const QFileInfo info( folder );
        if( info.exists() )
        {
            if( info.lastModified().toTime_t() != mtime.toUInt() )
            {
                m_folders << folder;
                debug() << "Collection dir changed: " << folder << endl;
            }
        }
        else
        {
            // this folder has been removed
            m_folders << folder;
            debug() << "Collection dir removed: " << folder << endl;
        }

        kapp->processEvents(); // Don't block the GUI
    }

    if ( !m_folders.isEmpty() )
    {
        debug() << "Collection was modified." << endl;
        m_hasChanged = true;
        Amarok::StatusBar::instance()->shortMessage( i18n( "Updating Collection..." ) );

        // Start scanner process
        if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
        *m_scanner << "-i";
        *m_scanner << m_folders;
        m_scanner->start();
    }
}


bool
ScanController::doJob()
{
    DEBUG_BLOCK

    if( !CollectionDB::instance()->isConnected() )
        return false;
    if( m_incremental && !m_hasChanged )
        return true;

    CollectionDB::instance()->createTables( true );
    m_tablesCreated = true;

    //For a full rescan, we might not have cleared tags table (for devices not plugged
    //in), so preserve the necessary other tables (eg artist)
    CollectionDB::instance()->prepareTempTables();

    CollectionDB::instance()->invalidateArtistAlbumCache();

main_loop:
    uint delayCount = 100;

    /// Main Loop
    while( !isAborted() ) {
        if( m_xmlData.isNull() ) {
            if( !m_scanner->isRunning() )
                delayCount--;
            // Wait a bit after process has exited, so that we have time to parse all data
            if( delayCount == 0 )
                break;
            msleep( 15 );
        }
        else {
            m_dataMutex.lock();

            QDeepCopy<QString> data = m_xmlData;
            m_source->setData( data );
            m_xmlData = QString::null;

            m_dataMutex.unlock();

            if( !m_reader->parseContinue() )
                ::warning() << "parseContinue() failed: " << errorString() << endl << data << endl;
        }
    }

    if( !isAborted() ) {
        if( m_scanner->normalExit() && !m_scanner->signalled() ) {
            CollectionDB::instance()->sanitizeCompilations();
            if ( m_incremental ) {
                m_foldersToRemove += m_folders;
                foreach( m_foldersToRemove ) {
                    m_fileMapsMutex.lock();
                    CollectionDB::instance()->removeSongsInDir( *it, &m_filesDeleted );
                    m_fileMapsMutex.unlock();
                    CollectionDB::instance()->removeDirFromCollection( *it );
                }
                CollectionDB::instance()->removeOrphanedEmbeddedImages();
            }
            else
                CollectionDB::instance()->clearTables( false ); // empty permanent tables

            CollectionDB::instance()->copyTempTables(); // copy temp into permanent tables

            //Clean up unused entries in the main tables (eg artist, composer)
            CollectionDB::instance()->deleteAllRedundant( "artist" );
            CollectionDB::instance()->deleteAllRedundant( "composer" );
            CollectionDB::instance()->deleteAllRedundant( "year" );
            CollectionDB::instance()->deleteAllRedundant( "genre" );
            CollectionDB::instance()->deleteAllRedundant( "album" );

            //Remove free space and fragmentation in the DB. Used to run on shutdown, but
            //that took too long, sometimes causing Amarok to be killed.
            CollectionDB::instance()->vacuum();

        }
        else {
            if( m_crashedFiles.size() <= MAX_RESTARTS ||
                    m_crashedFiles.size() <= (m_scanCount * MAX_FAILURE_PERCENTAGE) / 100 ) {
                kapp->postEvent( this, new RestartEvent() );
                sleep( 3 );
            }
            else
                m_aborted = true;

            goto main_loop;
        }
    }

    if( CollectionDB::instance()->isConnected() )
    {
        m_tablesCreated = false;
        CollectionDB::instance()->dropTables( true ); // drop temp tables
    }

    setProgress100Percent();
    return !isAborted();
}


void
ScanController::slotReadReady()
{
    QString line;

    m_dataMutex.lock();

    while( m_scanner->readln( line, true, 0 ) != -1 ) {
        if( !line.startsWith( "exepath=" ) ) // skip binary location info from scanner
            m_xmlData += line;
    }
    m_dataMutex.unlock();
}

bool
ScanController::requestPause()
{
    DEBUG_BLOCK
    debug() << "Attempting to pause the collection scanner..." << endl;
    DCOPRef dcopRef( "amarokcollectionscanner", "scanner" );
    m_lastCommandPaused = true;
    return dcopRef.send( "pause" );
}

bool
ScanController::requestUnpause()
{
    DEBUG_BLOCK
    debug() << "Attempting to unpause the collection scanner..." << endl;
    DCOPRef dcopRef( "amarokcollectionscanner", "scanner" );
    m_lastCommandPaused = false;
    return dcopRef.send( "unpause" );
}

void
ScanController::requestAcknowledged()
{
    DEBUG_BLOCK
    if( m_waitingBundle )
        m_waitingBundle->scannerAcknowledged();
    if( m_lastCommandPaused )
        m_isPaused = true;
    else
        m_isPaused = false;
}

void
ScanController::slotFileMoved( const QString &/*src*/, const QString &/*dest*/)
{
    //why is this needed?  QBob, take a look at this
    /*
    if( m_incremental ) // pedantry
      {
        m_fileMapsMutex.lock();
        m_filesFound[ src ] = true;
        m_fileMapsMutex.unlock();
      }
    */
}

void
ScanController::notifyThisBundle( MetaBundle* bundle )
{
    DEBUG_BLOCK
    m_waitingBundle = bundle;
    debug() << "will notify " << m_waitingBundle << endl;
}

bool
ScanController::startElement( const QString&, const QString& localName, const QString&, const QXmlAttributes& attrs )
{
    // List of entity names:
    //
    // itemcount     Number of files overall
    // folder        Folder which is being processed
    // dud           Invalid audio file
    // tags          Valid audio file with metadata
    // playlist      Playlist file
    // image         Cover image
    // compilation   Folder to check for compilation
    // filesize      Size of the track in bytes


    if( localName == "dud" || localName == "tags" || localName == "playlist" ) {
        incrementProgress();
    }

    if( localName == "itemcount") {
        const int totalSteps = attrs.value( "count" ).toInt();
        debug() << "itemcount event: " << totalSteps << endl;
        setProgressTotalSteps( totalSteps );
    }

    else if( localName == "tags") {
        MetaBundle bundle;
        bundle.setPath      ( attrs.value( "path" ) );
        bundle.setTitle     ( attrs.value( "title" ) );
        bundle.setArtist    ( attrs.value( "artist" ) );
        bundle.setComposer  ( attrs.value( "composer" ) );
        bundle.setAlbum     ( attrs.value( "album" ) );
        bundle.setComment   ( attrs.value( "comment" ) );
        bundle.setGenre     ( attrs.value( "genre" ) );
        bundle.setYear      ( attrs.value( "year" ).toInt() );
        bundle.setTrack     ( attrs.value( "track" ).toInt() );
        bundle.setDiscNumber( attrs.value( "discnumber" ).toInt() );
        bundle.setBpm       ( attrs.value( "bpm" ).toFloat() );
        bundle.setFileType( attrs.value( "filetype" ).toInt() );
        bundle.setUniqueId( attrs.value( "uniqueid" ) );
        bundle.setCompilation( attrs.value( "compilation" ).toInt() );

        if( attrs.value( "audioproperties" ) == "true" ) {
            bundle.setBitrate   ( attrs.value( "bitrate" ).toInt() );
            bundle.setLength    ( attrs.value( "length" ).toInt() );
            bundle.setSampleRate( attrs.value( "samplerate" ).toInt() );
        }

        if( !attrs.value( "filesize" ).isNull()
                && !attrs.value( "filesize" ).isEmpty() )
        {
            bundle.setFilesize( attrs.value( "filesize" ).toInt() );
        }

        CollectionDB::instance()->addSong( &bundle, m_incremental );
        if( !bundle.uniqueId().isEmpty() )
        {
            m_fileMapsMutex.lock();
            m_filesAdded[bundle.uniqueId()] = bundle.url().path();
            m_fileMapsMutex.unlock();
        }

        m_scanCount++;
    }

    else if( localName == "folder" ) {
        const QString folder = attrs.value( "path" );
        const QFileInfo info( folder );

        // Update dir statistics for rescanning purposes
        if( info.exists() )
            CollectionDB::instance()->updateDirStats( folder, info.lastModified().toTime_t(), true);

        if( m_incremental ) {
            m_foldersToRemove += folder;
        }
    }

    else if( localName == "playlist" )
        QApplication::postEvent( PlaylistBrowser::instance(), new PlaylistFoundEvent( attrs.value( "path" ) ) );

    else if( localName == "compilation" )
        CollectionDB::instance()->checkCompilations( attrs.value( "path" ), !m_incremental);

    else if( localName == "image" ) {
        // Deserialize CoverBundle list
        QStringList list = QStringList::split( "AMAROK_MAGIC", attrs.value( "list" ), true );
        QValueList< QPair<QString, QString> > covers;

        for( uint i = 0; i < list.count(); ) {
            covers += qMakePair( list[i], list[i + 1] );
            i += 2;
        }

        CollectionDB::instance()->addImageToAlbum( attrs.value( "path" ), covers, CollectionDB::instance()->isConnected() );
    }

    else if( localName == "embed" ) {
        CollectionDB::instance()->addEmbeddedImage( attrs.value( "path" ), attrs.value( "hash" ), attrs.value( "description" ) );
    }

    return true;
}


void
ScanController::customEvent( QCustomEvent* e )
{
    if( e->type() == RestartEventType )
    {
        debug() << "RestartEvent received." << endl;

        QFile log( Amarok::saveLocation( QString::null ) + "collection_scan.log" );
        if ( !log.open( IO_ReadOnly ) )
        ::warning() << "Failed opening log file " << log.name() << endl;
        else {
            QCString path = QCString(log.readAll());
            m_crashedFiles << QString::fromUtf8( path, path.length() );

        }

        m_dataMutex.lock();
        m_xmlData = QString::null;
        delete m_source;
        m_source = new QXmlInputSource();
        m_dataMutex.unlock();

        delete m_reader;
        m_reader = new QXmlSimpleReader();

        m_reader->setContentHandler( this );
        m_reader->parse( m_source, true );

        delete m_scanner; // Reusing doesn't work, so we have to destroy and reinstantiate
        m_scanner = new Amarok::ProcIO();
        connect( m_scanner, SIGNAL( readReady( KProcIO* ) ), SLOT( slotReadReady() ) );

        *m_scanner << "amarokcollectionscanner";
        *m_scanner << "--nocrashhandler"; // We want to be able to catch SIGSEGV
        if( m_incremental )
            *m_scanner << "-i";

        *m_scanner << "-p";
        *m_scanner << "-s";
        m_scanner->start();
    }
    else
        ThreadManager::Job::customEvent( e );
}


#include "scancontroller.moc"
