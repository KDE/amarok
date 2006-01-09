// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#define DEBUG_PREFIX "CollectionReader"

#include "amarok.h"
#include "amarokconfig.h"
#include <cerrno>
#include "debug.h"
#include "collectiondb.h"
#include "collectionreader.h"

#include <dirent.h>    //stat
#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <qdeepcopy.h>

#include <iostream>
#include "metabundle.h"
#include "playlistbrowser.h"
#include "statusbar.h"
#include <sys/stat.h>  //stat
#include <sys/types.h> //stat
#include <unistd.h>    //stat

CollectionReader::CollectionReader( CollectionDB* parent, const QStringList& folders )
        : DependentJob( parent, "CollectionReader" )
        , m_importPlaylists( AmarokConfig::importPlaylists() )
        , m_incremental( false )
        , m_folders( QDeepCopy<QStringList>(folders) )
        , m_db( CollectionDB::instance()->getStaticDbConnection() )
        , m_recursively( AmarokConfig::scanRecursively() )
        , log( QFile::encodeName( amaroK::saveLocation( QString::null ) + "collection_scan.log" ) )
{
    setDescription( i18n( "Building Collection" ) );

    // don't traverse /
    struct stat statBuf;
    if ( stat( "/", &statBuf ) == 0 ) {
        struct direntry de;
        memset(&de, 0, sizeof(struct direntry));
        de.dev = statBuf.st_dev;
        de.ino = statBuf.st_ino;

        m_processedDirs.resize(m_processedDirs.size()+1);
        m_processedDirs[m_processedDirs.size()-1] = de;
    }
}


CollectionReader::~CollectionReader()
{
    CollectionDB::instance()->returnStaticDbConnection( m_db );
}



IncrementalCollectionReader::IncrementalCollectionReader( CollectionDB *parent )
        : CollectionReader( parent, QStringList() )
        , m_hasChanged( false )
{
    m_importPlaylists = false;
    m_incremental = true;

    setDescription( i18n( "Updating Collection" ) );
}

bool
IncrementalCollectionReader::doJob()
{
    /**
     * The Incremental Reader works as follows: Here we check the mtime of every directory in the "directories"
     * table and store all changed directories in m_folders.
     *
     * These directories are then scanned in CollectionReader::doJob(), with m_recursively set according to the
     * user's preference, so the user can add directories or whole directory trees, too. Since we don't want to
     * rescan unchanged subdirectories, CollectionReader::readDir() checks if we are scanning recursively and
     * prevents that.
     */

    struct stat statBuf;
    const QStringList values = CollectionDB::instance()->query( "SELECT dir, changedate FROM directories;" );

    foreach( values ) {
        const QString folder = *it;
        const QString mtime  = *++it;

        if ( stat( QFile::encodeName( folder ), &statBuf ) == 0 ) {
            if ( QString::number( (long)statBuf.st_mtime ) != mtime ) {
                m_folders << folder;
                debug() << "Collection dir changed: " << folder << endl;
            }
        }
        else {
            // this folder has been removed
            m_folders << folder;
            debug() << "Collection dir removed: " << folder << endl;
        }
    }

    if ( !m_folders.isEmpty() ) {
        m_hasChanged = true;
        amaroK::StatusBar::instance()->shortMessage( i18n( "Updating Collection..." ) );
    }

    return CollectionReader::doJob();
}

bool
CollectionReader::doJob()
{
    if ( !m_db->isConnected() )
        return false;

    log << "Collection Scan Log\n";
    log << "===================\n";
    log << i18n( "Report this file if amaroK crashes when building the Collection." ).local8Bit();
    log << "\n\n\n";

    // we need to create the temp tables before readDir gets called ( for the dir stats )
    CollectionDB::instance()->createTables( m_db );
    setProgressTotalSteps( 100 );


    QStrList entries;
    foreach( m_folders ) {
        if( (*it).isEmpty() )
            //apparently somewhere empty strings get into the mix
            //which results in a full-system scan! Which we can't allow
            continue;

        QString dir = *it;
        if ( !dir.endsWith( "/" ) )
            dir += '/';

        setStatus( i18n("Reading directory structure") );
        readDir( dir, entries );
    }

    if ( !entries.isEmpty() ) {
        setStatus( i18n("Reading metadata") );
        setProgressTotalSteps( entries.count() );
        readTags( entries );
    }

    if ( !isAborted() ) {
        if ( !m_incremental )
            CollectionDB::instance()->clearTables();
        else
            foreach( m_folders )
                CollectionDB::instance()->removeSongsInDir( *it );

        // rename tables
        CollectionDB::instance()->moveTempTables( m_db );
    }

    CollectionDB::instance()->dropTables( m_db );

    log.close();

    return !isAborted();
}


void
CollectionReader::readDir( const QString& dir, QStrList& entries )
{
    // linux specific, but this fits the 90% rule
    if ( dir.startsWith("/dev") || dir.startsWith("/sys") || dir.startsWith("/proc") )
        return;

    QCString dir8Bit = QFile::encodeName( dir );

    struct stat statBuf;
    //update dir statistics for rescanning purposes
    if ( stat( dir8Bit, &statBuf ) == 0 )
        CollectionDB::instance()->updateDirStats( dir, (long)statBuf.st_mtime, !m_incremental ? m_db : 0 );
    else {
        if ( m_incremental ) {
            CollectionDB::instance()->removeSongsInDir( dir );
            CollectionDB::instance()->removeDirFromCollection( dir );
        }
        return;
    }

    struct direntry de;
    memset(&de, 0, sizeof(struct direntry));
    de.dev = statBuf.st_dev;
    de.ino = statBuf.st_ino;

    int f = -1;

#if __GNUC__ < 4
    for (unsigned int i = 0; i < m_processedDirs.size(); ++i)
        if (memcmp(&m_processedDirs[i], &de, sizeof(direntry)) == 0) {
            f = i; break;
        }
#else
    f = m_processedDirs.find(de);
#endif

    if ( ! S_ISDIR ( statBuf.st_mode) || f != -1 ) {
        debug() << "Skipping, already scanned: " << dir << endl;
        return;
    }

    m_processedDirs.resize(m_processedDirs.size()+1);
    m_processedDirs[m_processedDirs.size()-1] = de;

    DIR *d = opendir( dir8Bit );
    if( d == NULL ) {
        if( errno == EACCES )
            warning() << "Skipping, no access permissions: " << dir << endl;
        return;
    }

    for( dirent *ent; (ent = readdir( d )) && !isAborted(); ) {
        QCString entry (ent->d_name);

        if ( entry == "." || entry == ".." )
            continue;

        entry.prepend( dir8Bit );

        if ( stat( entry, &statBuf ) != 0 )
            continue;

        // loop protection
        if ( ! ( S_ISDIR( statBuf.st_mode ) || S_ISREG( statBuf.st_mode ) ) )
            continue;

        if ( S_ISDIR( statBuf.st_mode ) && m_recursively && entry.length() && entry[0] != '.' )
        {
            const QString file = QFile::decodeName( entry );

            if( !m_incremental || !CollectionDB::instance()->isDirInCollection( file ) )
                // we MUST add a '/' after the dirname
                readDir( file + '/', entries );
        }

        else if( S_ISREG( statBuf.st_mode ) )
        {
            QString file = QFile::decodeName( entry );

            if ( m_importPlaylists ) {
                if ( file.endsWith(".m3u") || file.endsWith(".pls") )
                    QApplication::postEvent( PlaylistBrowser::instance(), new PlaylistFoundEvent( file ) );
            }

            entries.append( entry );
        }
    }

    closedir( d );
}

void
CollectionReader::readTags( const QStrList& entries )
{
    DEBUG_BLOCK

    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages; validImages << "jpg" << "png" << "gif" << "jpeg";
//    QStringList validMusic; validMusic << "mp3" << "ogg" << "wav" << "flac";

    QValueList<CoverBundle> covers;
    QStringList images;

    for(QStrListIterator it(entries); it.current(); ++it) {

        // Check if we shall abort the scan
        if( isAborted() )
           return;

        incrementProgress();

        const QString path = QFile::decodeName ( it.current() );
        KURL url; url.setPath( path );
        const QString ext = amaroK::extension( path );
        const QString dir = amaroK::directory( path );

        // Append path to logfile
        log << path.local8Bit() << std::endl;
        log.flush();

        // Tests reveal the following:
        //
        // TagLib::AudioProperties   Relative Time Taken
        //
        //  No AudioProp Reading        1
        //  Fast                        1.18
        //  Average                     Untested
        //  Accurate                    Untested

        // don't use the KURL ctor as it checks the db first
        MetaBundle bundle;
        bundle.setPath( path );
        bundle.readTags( TagLib::AudioProperties::Fast );

        if( validImages.contains( ext ) )
           images += path;

        else if( bundle.isValidMedia() )
        {
            CoverBundle cover( bundle.artist(), bundle.album() );

            if( !covers.contains( cover ) )
                covers += cover;

           CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );
        }

        // Update Compilation-flag, when this is the last loop-run
        // or we're going to switch to another dir in the next run
        if( path == entries.getLast() || dir != amaroK::directory( QFile::decodeName(++QStrListIterator( it )) ) )
        {
            // we entered the next directory
            foreach( images )
                CollectionDB::instance()->addImageToAlbum( *it, covers, m_db );

            CollectionDB::instance()->checkCompilations( dir, !m_incremental, m_db );

            // clear now because we've processed them
            covers.clear();
            images.clear();
        }
    }
}
