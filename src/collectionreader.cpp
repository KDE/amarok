// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#include "amarokconfig.h"
#include "collectiondb.h"
#include "collectionreader.h"
#include <dirent.h>    //stat
#include <errno.h>
#include <iostream>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <metabundle.h>
#include "playlistbrowser.h"
#include <sys/types.h> //stat
#include <sys/stat.h>  //stat
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <unistd.h>    //stat

namespace amaroK
{
    QString
    saveLocation( const QString &directory = QString::null )
    {
        return KGlobal::dirs()->saveLocation( "data", QString("amarok/") + directory, true );
    }
}

CollectionReader::CollectionReader( CollectionDB* parent, const QStringList& folders )
    : DependentJob( parent, "CollectionReader" )
    , m_db( CollectionDB::instance()->getStaticDbConnection() )
    , m_folders( folders )
    , m_recursively( AmarokConfig::scanRecursively() )
    , m_importPlaylists( AmarokConfig::importPlaylists() )
    , m_incremental( false )
    , log( QFile::encodeName( amaroK::saveLocation() + "collection_scan.log" ) )
{
    setDescription( i18n( "Building collection" ) );
}


CollectionReader::~CollectionReader()
{
    CollectionDB::instance()->returnStaticDbConnection( m_db );
}



IncrementalCollectionReader::IncrementalCollectionReader( CollectionDB *parent )
    : CollectionReader( parent, QStringList() )
{
    m_recursively     = false;
    m_importPlaylists = false;
    m_incremental     = true;

    setDescription( i18n( "Updating collection" ) );
}


#define foreach( x ) \
    for( QStringList::ConstIterator it = x.begin(), end = x.end(); it != end; ++it )

bool
IncrementalCollectionReader::doJob()
{
    /**
     * Incremental here means only scan directories that has been modified since the last scan
     */

    struct stat statBuf;
    const QStringList values = CollectionDB::instance()->query( "SELECT dir, changedate FROM directories;" );

    foreach( values ) {
        const QString folder = *it;
        const QString mtime  = *++it;

        if ( stat( QFile::encodeName( folder ), &statBuf ) == 0 ) {
            if ( QString::number( (long)statBuf.st_mtime ) != mtime ) {
                m_folders << folder;
                kdDebug() << "Collection dir changed: " << folder << endl;
            }
        }
        else {
            // this folder has been removed
            m_folders << folder;
            kdDebug() << "Collection dir removed: " << folder << endl;
        }
    }

    return CollectionReader::doJob();
}

bool
CollectionReader::doJob()
{
    log << "Collection Scan Log\n";
    log << "===================\n";
    log << i18n( "Report this file if amaroK crashes when building the Collection." ).local8Bit();
    log << "\n\n\n";

    if ( !m_folders.empty() ) {
        // we need to create the temp tables before readDir gets called ( for the dir stats )
        CollectionDB::instance()->createTables( m_db );
        setProgressTotalSteps( 100 );
    }


    QStringList entries;
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

    if ( !entries.empty() ) {
        setStatus( i18n("Reading metadata") );
        setProgressTotalSteps( entries.count() );
        readTags( entries );
    }

    log.close();

    return !entries.empty();
}


void
CollectionReader::readDir( const QString& dir, QStringList& entries )
{
    QCString dir8Bit = QFile::encodeName( dir );

    if ( m_processedDirs.contains( dir ) ) {
        kdDebug() << "[CollectionReader] Skipping, already scanned: " << dir << endl;
        return;
    }

    m_processedDirs << dir;

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


    DIR *d = opendir( dir8Bit );
    if( d == NULL ) {
        if( errno == EACCES )
            kdWarning() << "[CollectionReader] Skipping, no access permissions: " << dir << endl;
        return;
    }


    for( dirent *ent; (ent = readdir( d )); )
    {
        if( isAborted() )
            return;

        QCString entry = ent->d_name;

        if ( entry == "." || entry == ".." )
            continue;

        entry.prepend( dir8Bit );

        if ( stat( entry, &statBuf ) != 0 )
            continue;

        if ( S_ISDIR( statBuf.st_mode ) ) {
            if ( m_recursively ) {
                // Check for symlink recursion
                QFileInfo info( entry );
                if ( info.isSymLink() && m_processedDirs.contains( info.readLink() ) ) {
                    kdWarning() << "[CollectionReader] Skipping, recursive symlink: " << dir << endl;
                    continue;
                }
                if ( !m_incremental || !CollectionDB::instance()->isDirInCollection( entry ) )
                    // we MUST add a '/' after the dirname
                    readDir( QFile::decodeName( entry ) + '/', entries );
            }
        }
        else if ( S_ISREG( statBuf.st_mode ) ) {
            //if a playlist is found it will send a PlaylistFoundEvent to PlaylistBrowser
            QString file = QFile::decodeName( entry );

            if( m_importPlaylists ) {
                QString ext = file.right( 4 ).lower();
                if ( ext == ".m3u" || ext == ".pls" )
                    QApplication::postEvent( PlaylistBrowser::instance(), new PlaylistFoundEvent( file ) );
            }

            entries << file;
        }
    }

    closedir( d );
}


static inline QString extension( const QString &filename ) { return filename.mid( filename.findRev( '.' ) + 1 ).lower(); }
static inline QString directory( const QString &filename ) { return filename.section( '/', 0, -2 ); }

void
CollectionReader::readTags( const QStringList& entries )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages, validMusic;
    validImages << "jpg" << "png" << "gif" << "jpeg";
    validMusic  << "mp3" << "ogg" << "wav" << "flac";

    foreach( entries )
    {
        // Check if we shall abort the scan
        if( isAborted() )
           return;

        incrementProgress();

        KURL url; url.setPath( *it );
        QValueList<CoverBundle> covers;
        QStringList images;
        const QString ext = extension( *it );
        const QString dir = directory( *it );

        // Append path to logfile
        log << url.path().local8Bit() << "\n";
        log.flush();

        TagLib::FileRef f( QFile::encodeName( url.path() ), false );  //false == don't read audioprops
        if ( !f.isNull() ) {
            MetaBundle bundle( url, f.tag(), 0 );
            CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );

            if ( !covers.contains( CoverBundle( bundle.artist(), bundle.album() ) ) )
                covers.append( CoverBundle( bundle.artist(), bundle.album() ) );
        }
        // Add tag-less tracks to database

        else if ( validMusic.contains( ext ) ) {
            MetaBundle bundle; bundle.setUrl( url );
            CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );

            CoverBundle cover( bundle.artist(), bundle.album() );
            if ( !covers.contains( cover ) )
                covers += cover;
        }

        else if ( validMusic.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ).lower() ) )
        {
            MetaBundle bundle; bundle.setUrl( url.path() );
            CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );

            CoverBundle cover( bundle.artist(), bundle.album() );
            if ( !covers.contains( cover ) )
                covers += cover;
        }
        // Add images to the cover database
        else if ( validImages.contains( ext ) )
            images << url.path();

        // Update Compilation-flag, when this is the last loop-run or we're going to switch to another dir in the next run
        if ( it == entries.fromLast() || dir != directory( *++QStringList::ConstIterator( it ) ) )
        {
            // we entered the next directory
            foreach( images )
                CollectionDB::instance()->addImageToAlbum( *it, covers, m_db );

            CollectionDB::instance()->checkCompilations( dir,  !m_incremental, m_db );
        }
    }

    if ( !m_incremental )
        CollectionDB::instance()->clearTables();
    else
        foreach( m_folders )
            CollectionDB::instance()->removeSongsInDir( *it );

    // rename tables
    CollectionDB::instance()->moveTempTables( m_db );
    CollectionDB::instance()->dropTables( m_db );

    kdDebug() << "END " << k_funcinfo << endl;
}
