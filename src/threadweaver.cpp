//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#include "config.h"	// Has USE_MYSQL

#include "collectionbrowser.h"   //CollectionReader::readTags()
#include "collectiondb.h"        //needed for query()
#include "metabundle.h"
#include "playlistitem.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <qfile.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>       //KGlobal::dirs()

#include <taglib/tstring.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2framefactory.h>


ThreadWeaver::ThreadWeaver( QObject *w )
        : m_parent( w )
        , m_bool( true )
        , m_currentJob( 0 )
{}

void
ThreadWeaver::append( Job* const job, bool priorityJob )
{
    //intended to be used by GUI thread, but is thread-safe

    mutex.lock();
    if ( priorityJob )
        m_Q.prepend( job );
    else
        m_Q.append( job );

    if ( !running() )
        start( QThread::LowestPriority );

    mutex.unlock();
}

bool
ThreadWeaver::remove( Job* const job )
{
    //TODO you need to be able to only remove jobs of a certain type frankly.
    //TODO this is no good when you say need to remove a job that will act on a playlistitem etc.
    //maybe above is void* and you make operator== pure virtual?

    bool b;

    mutex.lock();
    //TODO we delete or user deletes is yet undecided
    b = m_Q.remove( job );
    mutex.unlock();

    //TODO inform users of thread that you have to postpone deletion of stuff that may be in event loop
    return b;
}

void
ThreadWeaver::cancel()
{
    m_Q.setAutoDelete( true );
    mutex.lock();
    m_currentJob = 0; //FIXME will never be deleted!
    m_Q.clear();
    mutex.unlock();
    m_Q.setAutoDelete( false );

    //TODO inform users of thread that you have to postpone deletion of stuff that may be in event loop
}

void
ThreadWeaver::run()
{
    msleep( 200 ); //this is an attempt to encourage the queue to be filled with more than 1 item before we
    //start processing, and thus prevent unecessary stopping and starting of the thread

    kdDebug() << "[weaver] Started..\n";
    QApplication::postEvent( m_parent, new QCustomEvent( ThreadWeaver::Started ) );

    while ( m_bool ) {
        mutex.lock();
        if ( m_Q.isEmpty() ) { mutex.unlock(); break; } //we exit the loop here
        m_currentJob = m_Q.getFirst();
        m_Q.removeFirst();
        mutex.unlock();

        bool b = m_currentJob->doJob();

        mutex.lock();
        if ( m_currentJob ) {
            if ( b ) m_currentJob->postJob(); //Qt will delete the job for us
            else delete m_currentJob;     //we need to delete the job
        }
        mutex.unlock();
    }

    kdDebug() << "[weaver] Done!\n";
    QApplication::postEvent( m_parent, new QCustomEvent( ThreadWeaver::Done ) );
}



ThreadWeaver::Job::Job( QObject *obj, JobType type )
        : QCustomEvent( type )
        , m_target( obj )
{}

inline void
ThreadWeaver::Job::postJob()
{
    QApplication::postEvent( m_target, this );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SearchPath
//////////////////////////////////////////////////////////////////////////////////////////

bool SearchModule::m_stop;

SearchModule::SearchModule( QObject* parent, QString path, QString token, KListView* resultView, KListViewItem* historyItem )
        : Job( parent, Job::SearchModule )
        , resultCount( 0 )
        , m_parent( parent )
        , m_path( path )
        , m_token( token )
        , m_resultView( resultView )
        , m_historyItem( historyItem )
{}

bool SearchModule::doJob()
{
    m_stop = false;

    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Start, m_historyItem ) );
    searchDir( m_path );
    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Stop, m_historyItem ) );

    return TRUE;
}

void SearchModule::searchDir( QString path )
{
    DIR * d = opendir( QFile::encodeName( path ) );
    if ( d ) {
        dirent * ent;
        while ( ( ent = readdir( d ) ) ) {
            // Check if we shall abort
            if ( m_stop ) return;

            QString file( ent->d_name );

            if ( file != "." && file != ".." ) {
                DIR * t = opendir( QFile::encodeName( path ) + QFile::encodeName( file ) + "/" );
                if ( t ) {
                    closedir( t );
                    searchDir( path + file + "/" );
                } else
                    if ( file.contains( m_token, FALSE ) ) {
                        QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Progress, m_historyItem, m_resultView, ++resultCount, path, file ) );
                        m_resultList.append( path + file );
                    }
            }
        }
        closedir( d );
        free( ent );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionReader
//////////////////////////////////////////////////////////////////////////////////////////

bool CollectionReader::m_stop;

CollectionReader::CollectionReader( CollectionDB* parent, QObject *playlistBrowser,
                                    const QStringList& folders, bool recursively, bool importPlaylists, bool incremental )
        : Job( parent, Job::CollectionReader )
        , m_parent( parent )
        , m_playlistBrowser( playlistBrowser )
        , m_folders( folders )
        , m_recursively( recursively )
        , m_importPlaylists( importPlaylists )
        , m_incremental( incremental )
{}


bool
CollectionReader::doJob()
{
    m_stop = false;

    QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Start ) );

    const QString logPath = KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "collection_scan.log";
    std::ofstream log(  QFile::encodeName( logPath ) );
    log << "Collection Scan logfile\n";
    log << "=======================\n";
    log << i18n( "Last processed file is at the bottom. Report this file in case of crashes while building the Collection.\n\n\n" ).local8Bit();

    if ( !m_incremental )
        m_parent->purgeDirCache();

    QStringList entries;
    //iterate over all folders
    for ( uint i = 0; i < m_folders.count(); i++ )
    {
        QString dir = m_folders[ i ];

        if ( !dir.endsWith( "/" ) )
            dir += '/';

        readDir( dir, entries );
    }

    if ( !entries.empty() ) {
        QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Total, entries.count() ) );
        readTags( entries, log );
    }

    QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Stop ) );

    log.close();
    return !entries.empty();
}

void
CollectionReader::readDir( const QString& dir, QStringList& entries )
{
    if ( m_processedDirs.contains( dir ) ) {
        kdDebug() << "[CollectionReader] Already scanned: " << dir << endl;
        return;
    }

    m_processedDirs << dir;
    struct stat statBuf;

    //update dir statistics for rescanning purposes
    if ( stat( dir.local8Bit(), &statBuf ) == 0 )
        m_parent->updateDirStats( dir, ( long ) statBuf.st_mtime );
    else
    {
        if ( m_incremental )
        {
            m_parent->removeSongsInDir( dir );
            m_parent->removeDirFromCollection( dir );
        }
        return;
    }

    DIR * d = opendir( QFile::encodeName( dir ) );
    dirent *ent;

    if (d == NULL)
    {
        if (errno == EACCES)
            kdWarning() << "[CollectionReader] Skipping non-readable dir " << dir << endl;
        return;
    }

    while ( ( ent = readdir( d ) ) )
    {
        QCString entry = ent->d_name;

        if ( entry == "." || entry == ".." )
            continue;
        entry.prepend( QFile::encodeName( dir ) );

        if ( stat( entry, &statBuf ) == 0 )
        {
            if ( S_ISDIR( statBuf.st_mode ) )
            {
                if ( m_recursively ) {
                    // Check for symlink recursion
                    QFileInfo info( entry );
                    if ( info.isSymLink() && m_processedDirs.contains( info.readLink() ) ) {
                        kdWarning() << "[CollectionReader] Skipping recursive symlink.\n";
                        continue;
                    }
                    if ( !m_incremental || !m_parent->isDirInCollection( entry ) )
                        // we MUST add a '/' after the dirname
                        readDir( QFile::decodeName( entry ) + '/', entries );
                }
            }
            else if ( S_ISREG( statBuf.st_mode ) )
            {
                //if a playlist is found it will send a PlaylistFoundEvent to PlaylistBrowser
                QString file = QString::fromLocal8Bit( entry );
                QString ext = file.right( 4 ).lower();
                if( m_importPlaylists && (ext == ".m3u" || ext == ".pls") )
                    QApplication::postEvent( m_playlistBrowser, new PlaylistFoundEvent( file ) );
                entries <<  file ;
            }
        }
    }

    closedir( d );
}


void
CollectionReader::readTags( const QStringList& entries, std::ofstream& log )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    KURL url;
    m_parent->createTables( true );

    QStringList validImages, validMusic;
    validImages << "jpg" << "png" << "gif" << "jpeg";
    validMusic  << "mp3" << "ogg" << "wav" << "flac";

    for ( uint i = 0; i < entries.count(); i++ )
    {
        // Check if we shall abort the scan
        if ( m_stop ) return;

        if ( !( i % 20 ) ) //don't post events too often since this blocks amaroK
            QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Progress, i ) );

        url.setPath( entries[ i ] );

        // Append path to logfile
        log << url.path().local8Bit() << "\n";
        log.flush();

        TagLib::FileRef f( QFile::encodeName( url.path() ), false );  //false == don't read audioprops
        if ( !f.isNull() )
        {
            MetaBundle bundle( url, f.tag(), 0 );
            m_parent->addSong( bundle, m_incremental );
        }
        // Add tag-less tracks to database
        else if ( validMusic.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ).lower() ) )
        {
            MetaBundle bundle( url, false, false );
            m_parent->addSong( bundle, m_incremental );
        }
        // Add images to the cover database
        else if ( validImages.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ).lower() ) )
            m_parent->addImageToPath( url.directory(), url.filename(), true );
    }

    // let's lock the database (will block other threads)
#ifdef USE_MYSQL
//    m_parent->query( "START TRANSACTION;" );
#else
//    m_parent->query( "BEGIN TRANSACTION;" );
#endif

    // remove tables and recreate them (quicker than DELETE FROM)
    if ( !m_incremental )
    {
        m_parent->dropTables();
        m_parent->createTables();
    } else
    {
        // remove old entries from database, only
        for ( uint i = 0; i < m_folders.count(); i++ )
            m_parent->removeSongsInDir( m_folders[ i ] );
    }

    // rename tables
    m_parent->moveTempTables();

    // remove temp tables and unlock database
    m_parent->dropTables( true );

#ifdef USE_MYSQL
//    m_parent->query( "COMMIT;" );
#else
//    m_parent->query( "END TRANSACTION;" );
#endif

    QStringList albums;
    albums = m_parent->albumList();
    for ( uint i = 0; i < albums.count(); i++ )
        m_parent->isSamplerAlbum( albums[i] );

    kdDebug() << "END " << k_funcinfo << endl;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS TagWriter
//////////////////////////////////////////////////////////////////////////////////////////

TagWriter::TagWriter( QObject *o, PlaylistItem *item, const QString &oldTag, const QString &newTag, const int col, const bool updateView )
        : Job( o )
        , m_item( item )
        , m_failed( true )
        , m_oldTagString( oldTag )
        , m_newTagString( newTag )
        , m_tagType( col )
        , m_updateView( updateView )
{
    item->setText( col, i18n( "Writing tag..." ) );
}

bool
TagWriter::doJob()
{
    const QString path = m_item->url().path();
    //TODO I think this causes problems with writing and reading tags for non latin1 people
    //check with wheels abolut what it does and why to do it
    //TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding( TagLib::String::UTF8 );
    TagLib::FileRef f( QFile::encodeName( path ), false );

    if ( !f.isNull() ) {
        TagLib::Tag *t = f.tag();
        QString field;

        switch ( m_tagType ) {
        case PlaylistItem::Title:
            t->setTitle( QStringToTString( m_newTagString ));
            field = "title";
            break;
        case PlaylistItem::Artist:
            t->setArtist( QStringToTString( m_newTagString ) );
            field = "artist";
            break;
        case PlaylistItem::Album:
            t->setAlbum( QStringToTString( m_newTagString ) );
            field = "album";
            break;
        case PlaylistItem::Year:
            t->setYear( m_newTagString.toInt() );
            field = "year";
            break;
        case PlaylistItem::Comment:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            t->setComment( QStringToTString( m_newTagString ) );
            field = "comment";
            break;
        case PlaylistItem::Genre:
            t->setGenre( QStringToTString( m_newTagString ) );
            field = "genre";
            break;
        case PlaylistItem::Track:
            t->setTrack( m_newTagString.toInt() );
            field = "track";
            break;

        default:
            return true;
        }

        if( f.save() )
        {
           // Update the collection db.
           // Hopefully this does not cause concurreny issues with sqlite3, as we had in BR 87169.
           CollectionDB().updateURL( path, m_updateView );

           m_failed = false;
        }
    }

    return true;
}

void
TagWriter::completeJob()
{
    if( m_failed ) {
        m_item->setText( m_tagType, m_oldTagString.isEmpty() ? " " : m_oldTagString );
        amaroK::StatusBar::instance()->message( i18n( "The tag could not be changed." ) );
    }
    m_item->setText( m_tagType, m_newTagString.isEmpty() ? " " : m_newTagString );
}


//yes! no moc! There are no Q_OBJECTS
