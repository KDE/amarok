//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#include "collectiondb.h"    //needed for execSql()
#include "metabundle.h"
#include "playlistitem.h"
#include "threadweaver.h"

#include <dirent.h>
#include <sys/stat.h>

#include <qfile.h>

#include <kapplication.h>
#include <kdebug.h>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>


static inline const TagLib::String LocaleAwareTString( const QString &s )
{
    return TagLib::String( s.local8Bit().data(), TagLib::String::Latin1 );
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
    if( priorityJob )
        m_Q.prepend( job );
    else
        m_Q.append( job );
    mutex.unlock();

    if( !running() )
    {
        start( QThread::LowestPriority );
    }
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

    while( m_bool )
    {
        mutex.lock();
        if( m_Q.isEmpty() ) { mutex.unlock(); break; } //we exit the loop here
        m_currentJob = m_Q.getFirst();
        m_Q.removeFirst();
        mutex.unlock();

        bool b = m_currentJob->doJob();

        mutex.lock();
        if( m_currentJob )
        {
            if( b ) m_currentJob->postJob(); //Qt will delete the job for us
            else    delete m_currentJob;     //we need to delete the job
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
// CLASS TagReader
//////////////////////////////////////////////////////////////////////////////////////////

TagReader::TagReader( QObject *o, PlaylistItem *pi )
   : Job( o, Job::TagReader )
   , m_item( pi )
   , m_url( pi->url() )
   , m_tags( 0 )
{}

TagReader::~TagReader()
{
    delete m_tags;
}

bool
TagReader::doJob()
{
    if( m_url.protocol() == "file" )
    {
        m_tags = readTags( m_url );
        return true;
    }

    return false;
}

MetaBundle*
TagReader::readTags( const KURL &url, bool /*readAudioProps */) //STATIC
{
   //audioproperties are no longer read on demand
   TagLib::FileRef f( url.path().local8Bit(), true /*readAudioProps*/ ); //this is the slow step

   return f.isNull()? 0 : new MetaBundle( url, f.tag(), f.audioProperties() );
}

void
TagReader::bindTags()
{
   //for GUI access only
   //we're a friend of PlaylistItem
   if( m_tags ) m_item->setText( *m_tags );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS SearchPath
//////////////////////////////////////////////////////////////////////////////////////////

SearchModule::SearchModule( QObject* parent, QString path, QString token, KListView* resultView, KListViewItem* historyItem )
   : Job( parent, Job::SearchModule )
   , m_parent( parent )
   , m_path( path )
   , m_token( token )
   , m_resultView( resultView )
   , m_historyItem( historyItem )
{}

SearchModule::~SearchModule()
{}

bool SearchModule::doJob()
{
    resultCount = 0;

    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Start, m_historyItem ) );
    searchDir( m_path );
    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Stop, m_historyItem ) );

    return TRUE;
}

void SearchModule::searchDir( QString path )
{
    DIR *d = opendir( QFile::encodeName( path ) );
    if ( d )
    {
        dirent *ent;
        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );

            if ( file != "." && file != ".." )
            {
                DIR *t = opendir( QFile::encodeName( path ) + QFile::encodeName( file ) + "/" );
                if ( t )
                {
                    closedir( t );
                    searchDir( path + file + "/" );
                }
                else
                    if ( file.contains( m_token, FALSE ) )
                    {
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

CollectionReader::CollectionReader( CollectionDB* parent, QObject* statusBar,
                                    const QStringList& folders, bool recursively, bool incremental )
   : Job( parent, Job::CollectionReader )
   , m_parent( parent )
   , m_statusBar( statusBar )
   , m_folders( folders )
   , m_recursively( recursively )
   , m_incremental( incremental )
{}

CollectionReader::~CollectionReader()
{}

bool
CollectionReader::doJob()
{
    QApplication::postEvent( m_statusBar, new ProgressEvent( ProgressEvent::Start ) );
    
    if ( !m_incremental )
        m_parent->purgeDirCache();

    QStringList entries;
    //iterate over all folders
    for ( uint i = 0; i < m_folders.count(); i++ )
        readDir( m_folders[i], entries );

    if ( !entries.empty() )
    {
        QApplication::postEvent( m_statusBar, new ProgressEvent( ProgressEvent::Total, entries.count() ) );
        readTags( entries );
    }
    QApplication::postEvent( m_statusBar, new ProgressEvent( ProgressEvent::Stop ) );

    return !entries.empty();
}

void
CollectionReader::readDir( const QString& dir, QStringList& entries )
{
    DIR* d = opendir( QFile::encodeName( dir ) );
    if ( !d ) return;
    dirent *ent;
    struct stat statBuf;
                
    //update dir statistics for rescanning purposes
    stat( dir.local8Bit(), &statBuf );
    m_parent->updateDirStats( dir, (long)statBuf.st_mtime );
    
    while ( (ent = readdir( d )) )
    {
        QCString entry = ent->d_name;

        if ( entry == "." || entry == ".." )
            continue;
        entry.prepend( QFile::encodeName( dir.endsWith( "/" ) ? dir : dir + "/" ) );

        stat( entry, &statBuf );

        if ( S_ISDIR( statBuf.st_mode ) )
        {
            if ( m_recursively )
                //call ourself recursively for each subdir
                if ( !m_incremental || !m_parent->isDirInCollection( entry ) )
                    readDir( QFile::decodeName( entry ), entries );
        }
        else if ( S_ISREG( statBuf.st_mode ) )
            entries << QString::fromLocal8Bit( entry );
    }
    closedir( d );
}

void
CollectionReader::readTags( const QStringList& entries )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    KURL url;
    m_parent->createTables( true );

    QStringList validExtensions;
    validExtensions << "jpg" << "png" << "gif" << "jpeg";

    for ( uint i = 0; i < entries.count(); i++ ) {
        if ( !( i % 20 ) ) //don't post events too often since this blocks amaroK
            QApplication::postEvent( m_statusBar, new ProgressEvent( ProgressEvent::Progress, i ) );

        url.setPath( entries[i] );
        TagLib::FileRef f( QFile::encodeName( url.path() ), false );  //false == don't read audioprops

        if ( !f.isNull() ) {
            MetaBundle* bundle = new MetaBundle( url, f.tag(), 0 );

            QString command = "INSERT INTO tags_temp "
                                "( url, dir, createdate, album, artist, genre, title, year, comment, track ) "
                                "VALUES('";

            command += m_parent->escapeString( bundle->url().path() ) + "','";
            command += m_parent->escapeString( bundle->url().directory() ) + "',";
            command += "strftime('%s', 'now'),";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "album", bundle->album(), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "artist", bundle->artist(), true, !m_incremental ) ) ) + ",";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "genre", bundle->genre(), true, !m_incremental ) ) ) + ",'";
            command += m_parent->escapeString( bundle->title() ) + "','";
            command += m_parent->escapeString( QString::number( m_parent->getValueID( "year", bundle->year(), true, !m_incremental ) ) ) + "','";
            command += m_parent->escapeString( bundle->comment() ) + "','";
            command += m_parent->escapeString( bundle->track() ) + "');";

            m_parent->execSql( command );
            delete bundle;
        } else
        {
            // Add images to the cover database
            if ( validExtensions.contains( url.filename().mid( url.filename().findRev('.')+1 ) ) )
                m_parent->addImageToPath( url.directory(), url.filename(), true );
        }
    }
    // let's lock the database (will block other threads)
    m_parent->execSql( "BEGIN TRANSACTION;" );

    // remove tables and recreate them (quicker than DELETE FROM)
    if ( !m_incremental )
    {
        m_parent->dropTables();
        m_parent->createTables();
    } else
    {
        // remove old entries from database, only
        for ( uint i = 0; i < m_folders.count(); i++ )
          m_parent->removeSongsInDir( m_folders[i] );
    }

    // rename tables
    m_parent->moveTempTables();

    // remove temp tables and unlock database
    m_parent->dropTables( true );
    m_parent->execSql( "END TRANSACTION;" );

    kdDebug() << "END " << k_funcinfo << endl;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS AudioPropertiesReader
//////////////////////////////////////////////////////////////////////////////////////////

AudioPropertiesReader::AudioPropertiesReader( QObject *o, PlaylistItem *pi )
   : Job( o )
   , m_item( pi )
   , m_listView( static_cast<QListViewItem *>(pi)->listView() )
   , m_url( pi->url() )
{
    //TODO derive this from TagReader?
}

bool
AudioPropertiesReader::doJob()
{
    //TODO it is probably safer to record the itemPos in the ctor and check here with listview->itemAt()
    //TODO it is probably more efficient to do this with less specifity, i.e. when view scrolls put a job
    //     in to read tags for visible items, and then get this function to get the visible items for that moment

    int y  = m_item->itemPos(); //const //FIXME slow function!
    int h  = m_item->height();  //const
    int y2 = m_listView->contentsY(); //TODO find out the performance of this function
    if( (y + h) >= y2 && y <= (y2 + m_listView->visibleHeight()) )
    {
        //This is a quick scan
        //A more accurate scan is done when the track is played, and those properties are recorded with the track
        TagLib::FileRef f( QFile::encodeName( m_url.path() ), true, TagLib::AudioProperties::Fast );
        int length  = MetaBundle::Unavailable;
        int bitrate = MetaBundle::Unavailable;

        if( !f.isNull() )
        {
            //FIXME do we need to check the ap pointer? Seems to have not crashed so far..
            TagLib::AudioProperties *ap = f.audioProperties();
            length  = ap->length();
            bitrate = ap->bitrate();
        }

        m_length  = MetaBundle::prettyLength( length );
        m_bitrate = MetaBundle::prettyBitrate( bitrate );

        return true;
    }

    return false;
}

void
AudioPropertiesReader::completeJob()
{
    m_item->setText( PlaylistItem::Length,  m_length  );
    m_item->setText( PlaylistItem::Bitrate, m_bitrate );
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

    if( !f.isNull() )
    {
        TagLib::Tag *t = f.tag();
        const TagLib::String s = LocaleAwareTString( m_tagString );

        switch( m_tagType ) {
        case PlaylistItem::Title:
            t->setTitle( s );
            break;
        case PlaylistItem::Artist:
            t->setArtist( s );
            break;
        case PlaylistItem::Album:
            t->setAlbum( s );
            break;
        case PlaylistItem::Year:
            t->setYear( m_tagString.toInt() );
            break;
        case PlaylistItem::Comment:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            t->setComment( s );
            break;
        case PlaylistItem::Genre:
            t->setGenre( s );
            break;
        case PlaylistItem::Track:
            t->setTrack( m_tagString.toInt() );
            break;
        default:
            return false;
        }

        f.save(); //FIXME this doesn't always work, but! it returns void. Great huh?
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


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PLStats
//////////////////////////////////////////////////////////////////////////////////////////

PLStats::PLStats( QObject *o, const KURL &u, const KURL::List &ul )
  : Job( o, Job::PLStats )
  , m_url( u )
  , m_contents( ul )
  , m_length( 0 )
{}

bool
PLStats::doJob()
{
    //TODO update playlistItems too
    //TODO find out if tags are read on creation of the file ref and if so, update them too
    //TODO currently this is horrendously inefficient, tags are read so many times!

    for( KURL::List::const_iterator it = m_contents.begin(); it != m_contents.end(); ++it )
    {
        TagLib::FileRef f( QFile::encodeName( (*it).path() ), true, TagLib::AudioProperties::Accurate );

        if( !f.isNull() ) m_length += f.audioProperties()->length();
    }

    return true;
}

//yes! no moc! There are no Q_OBJECTS
