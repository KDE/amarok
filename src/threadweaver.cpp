//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#include "collectionbrowser.h"   //CollectionReader::readTags()
#include "collectiondb.h"        //needed for execSql()
#include "metabundle.h"
#include "playlistitem.h"
#include "threadweaver.h"

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <qfile.h>

#include <kapplication.h>
#include <kdebug.h>

#include <taglib/tstring.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>


static inline const TagLib::String LocaleAwareTString( const QString &s )
{
    // return TagLib::String( s.local8Bit().data(), TagLib::String::Latin1 );
    return QStringToTString( s );
}


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
                                    const QStringList& folders, bool recursively, bool incremental )
        : Job( parent, Job::CollectionReader )
        , m_parent( parent )
        , m_playlistBrowser( playlistBrowser )
        , m_folders( folders )
        , m_recursively( recursively )
        , m_incremental( incremental )
{}

bool
CollectionReader::doJob()
{
    m_stop = false;

    QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Start ) );

    if ( !m_incremental )
        m_parent->purgeDirCache();

    QStringList entries;
    //iterate over all folders
    for ( uint i = 0; i < m_folders.count(); i++ )
        readDir( m_folders[ i ], entries );

    if ( !entries.empty() ) {
        QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Total, entries.count() ) );
        readTags( entries );
    }
    QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Stop ) );

    return !entries.empty();
}

void
CollectionReader::readDir( const QString& dir, QStringList& entries )
{
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
            kdWarning() << "Skipping non-readable dir " << dir << endl;
        return;
    }

    while ( ( ent = readdir( d ) ) ) {
        QCString entry = ent->d_name;

        if ( entry == "." || entry == ".." )
            continue;
        entry.prepend( QFile::encodeName( dir.endsWith( "/" ) ? dir : dir + "/" ) );

        if ( stat( entry, &statBuf ) == 0 )
        {
            if ( S_ISDIR( statBuf.st_mode ) ) {
                if ( m_recursively )
                    //call ourself recursively for each subdir
                    if ( !m_incremental || !m_parent->isDirInCollection( entry ) )
                        readDir( QFile::decodeName( entry ), entries );
            } else if ( S_ISREG( statBuf.st_mode ) ) {
                 //if a playlist is found it will send a PlaylistFoundEvent to PlaylistBrowser
                    QString file = QString::fromLocal8Bit( entry );
                    QString ext = file.right( 4 ).lower();
                    if( ext == ".m3u" || ext == ".pls" )
                        QApplication::postEvent( m_playlistBrowser, new PlaylistFoundEvent( file ) );
                    entries <<  file ;
            }
        }

    }
    closedir( d );
}

void
CollectionReader::readTags( const QStringList& entries )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    KURL url;
    m_parent->createTables( true );

    QStringList validImages;
    validImages << "jpg" << "png" << "gif" << "jpeg";
    QStringList validMusic;
    validMusic << "mp3" << "ogg" << "flac" << "wav";

    for ( uint i = 0; i < entries.count(); i++ ) {
        // Check if we shall abort the scan
        if ( m_stop ) return;

        if ( !( i % 20 ) ) { //don't post events too often since this blocks amaroK
            QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Progress, i ) );
        }

        url.setPath( entries[ i ] );
        TagLib::FileRef f( QFile::encodeName( url.path() ), false );  //false == don't read audioprops

        QString command = "INSERT INTO tags_temp "
                            "( url, dir, createdate, album, artist, genre, title, year, comment, track ) "
                            "VALUES('";

        if ( !f.isNull() ) {
            MetaBundle bundle( url, f.tag(), 0 );

            command += m_parent->escapeString( bundle.url().path() ) + "','";
            command += m_parent->escapeString( bundle.url().directory() ) + "',";
            command += "strftime('%s', 'now'),";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "album", bundle.album(), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "artist", bundle.artist(), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "genre", bundle.genre(), true, !m_incremental ) ) ) + ",'";
            command += m_parent->escapeString( bundle.title() ) + "','";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "year", bundle.year(), true, !m_incremental ) ) ) + "','";
            command += m_parent->escapeString( bundle.comment() ) + "','";
            command += m_parent->escapeString( bundle.track() ) + "');";

            m_parent->execSql( command );
        }
        // Add tag-less tracks to database
        else if ( validMusic.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ) ) ) {
            command += m_parent->escapeString( url.path() ) + "','";
            command += m_parent->escapeString( url.directory() ) + "',";
            command += "strftime('%s', 'now'),";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "album", i18n( "unknown" ), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "artist", i18n( "unknown" ), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "genre", i18n( "unknown" ), true, !m_incremental ) ) ) + ",'";
            command += m_parent->escapeString( url.fileName() ) + "','";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "year", i18n( "unknown" ), true, !m_incremental ) ) ) + "','";
            command += m_parent->escapeString( i18n( "unknown" ) ) + "','";
            command += m_parent->escapeString( i18n( "unknown" ) ) + "');";

            m_parent->execSql( command );
        }
        // Add images to the cover database
        else if ( validImages.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ) ) )
            m_parent->addImageToPath( url.directory(), url.filename(), true );
    }
    // let's lock the database (will block other threads)
    m_parent->execSql( "BEGIN TRANSACTION;" );

    // remove tables and recreate them (quicker than DELETE FROM)
    if ( !m_incremental ) {
        m_parent->dropTables();
        m_parent->createTables();
    } else {
        // remove old entries from database, only
        for ( uint i = 0; i < m_folders.count(); i++ )
            m_parent->removeSongsInDir( m_folders[ i ] );
    }

    // rename tables
    m_parent->moveTempTables();

    // remove temp tables and unlock database
    m_parent->dropTables( true );
    m_parent->execSql( "END TRANSACTION;" );

    kdDebug() << "END " << k_funcinfo << endl;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS TagWriter
//////////////////////////////////////////////////////////////////////////////////////////

TagWriter::TagWriter( QObject *o, PlaylistItem *pi, const QString &s, const int col )
        : Job( o )
        , m_item( pi )
        , m_tagString( s )
        , m_tagType( col )
{
    //TODO deepcopy?
    //TODO use a enum for TagType
    pi->setText( col, i18n( "Writing tag..." ) );
}

bool
TagWriter::doJob()
{
    const KURL url = m_item->url();
    TagLib::FileRef f( QFile::encodeName( url.path() ), false );

    if ( !f.isNull() ) {
        TagLib::Tag * t = f.tag();
        const TagLib::String s = LocaleAwareTString( m_tagString );
        QString field;

        switch ( m_tagType ) {
        case PlaylistItem::Title:
            t->setTitle( s );
            field = "title";
            break;
        case PlaylistItem::Artist:
            t->setArtist( s );
            field = "artist";
            break;
        case PlaylistItem::Album:
            t->setAlbum( s );
            field = "album";
            break;
        case PlaylistItem::Year:
            t->setYear( m_tagString.toInt() );
            field = "year";
            break;
        case PlaylistItem::Comment:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            t->setComment( s );
            field = "comment";
            break;
        case PlaylistItem::Genre:
            t->setGenre( s );
            field = "genre";
            break;
        case PlaylistItem::Track:
            t->setTrack( m_tagString.toInt() );
            field = "track";
            break;
        default:
            return false;
        }

        f.save(); //FIXME this doesn't always work, but! it returns void. Great huh?

        //update the collection db
        CollectionDB *db = new CollectionDB();
        db->updateTag( url.path(), field, m_tagString );
        delete db;
    }

    //TODO can solve by reading tags now. Sucks or what?

    return true;
}

void
TagWriter::completeJob()
{
    //FIXME see PlaylistItem::setText() for an explanation for this hack
    m_item->setText( m_tagType, m_tagString.isEmpty() ? " " : m_tagString );
}


//yes! no moc! There are no Q_OBJECTS
