// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#include "collectiondb.h"
#include "collectionbrowser.h"
#include "collectionreader.h"
#include <dirent.h>    //stat
#include <errno.h>
#include <fstream>
#include <iostream>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <metabundle.h>
#include <sys/types.h> //stat
#include <sys/stat.h>  //stat
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <unistd.h>    //stat

bool CollectionReader::m_stop;

CollectionReader::CollectionReader( CollectionDB* parent, QObject *playlistBrowser,
                                    const QStringList& folders, bool recursively, bool importPlaylists, bool incremental )
        : DependentJob( parent, "CollectionReader" )
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

    if ( m_incremental )
    {
        QStringList values;
        struct stat statBuf;

        values = m_parent->query( "SELECT dir, changedate FROM directories;" );

        for ( uint i = 0; i < values.count(); i += 2 )
        {
            if ( stat( QFile::encodeName( values[i] ), &statBuf ) == 0 )
            {
                if ( QString::number( (long)statBuf.st_mtime ) != values[i + 1] )
                {
                    m_folders << values[i];
                    kdDebug() << "Collection dir changed: " << values[i] << endl;
                }
            }
            else
            {
                // this folder has been removed
                m_folders << values[i];
                kdDebug() << "Collection dir removed: " << values[i] << endl;
            }
        }
    }

    const QString logPath = KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "collection_scan.log";
    std::ofstream log(  QFile::encodeName( logPath ) );
    log << "Collection Scan logfile\n";
    log << "=======================\n";
    log << i18n( "Last processed file is at the bottom. Report this file in case of crashes while building the Collection.\n\n\n" ).local8Bit();


    if ( !m_folders.empty() )
    {
        // we need to create the temp tables before readDir gets called ( for the dir stats )
        m_parent->createTables( true );
        QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Start ) );
    }

    //iterate over all folders
    QStringList entries;
    for ( uint i = 0; i < m_folders.count(); i++ )
    {
        QString dir = m_folders[ i ];
        if ( !dir.endsWith( "/" ) ) dir += '/';

        readDir( dir, entries );
    }

    if ( !entries.empty() )
    {
        QApplication::postEvent( CollectionView::instance(), new ProgressEvent( ProgressEvent::Total, entries.count() ) );
        readTags( entries, log );
    }

    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Stop, entries.count() ) );
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
    if ( stat( QFile::encodeName( dir ), &statBuf ) == 0 )
        m_parent->updateDirStats( dir, ( long ) statBuf.st_mtime, !m_incremental );
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

    typedef QPair<QString, QString> CoverBundle;
    QValueList<CoverBundle> cbl;
    QStringList images;
    KURL url;

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
            m_parent->addSong( &bundle, m_incremental );

            if ( !cbl.contains( CoverBundle( bundle.artist(), bundle.album() ) ) )
                cbl.append( CoverBundle( bundle.artist(), bundle.album() ) );
        }
        // Add tag-less tracks to database
        else if ( validMusic.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ).lower() ) )
        {
            MetaBundle bundle;
            bundle.setUrl( url.path() );
            m_parent->addSong( &bundle, m_incremental );

            if ( !cbl.contains( CoverBundle( bundle.artist(), bundle.album() ) ) )
                cbl.append( CoverBundle( bundle.artist(), bundle.album() ) );
        }
        // Add images to the cover database
        else if ( validImages.contains( url.filename().mid( url.filename().findRev( '.' ) + 1 ).lower() ) )
            images << url.path();

        // Update Compilation-flag, when this is the last loop-run or we're going to switch to another dir in the next run
        if ( ( i + 1 ) == entries.count() || url.path().section( '/', 0, -2 ) != entries[ i + 1 ].section( '/', 0, -2 ) )
        {
            // we entered the next directory
            for ( uint j = 0; j < images.count(); j++ )
                m_parent->addImageToAlbum( images[ j ], cbl, true );

            cbl.clear();
            images.clear();
            m_parent->checkCompilations( url.path().section( '/', 0, -2 ) );
        }
    }

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
    m_parent->dropTables( true );

    kdDebug() << "END " << k_funcinfo << endl;
}
