/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
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

#define DEBUG_PREFIX "CollectionScanner"

#include "collectionscanner.h"
#include "debug.h"

#include <cerrno>
#include <dirent.h>    //stat
#include <iostream>
#include <sys/stat.h>  //stat
#include <sys/types.h> //stat
#include <unistd.h>    //stat

#include <taglib/fileref.h>
#include <taglib/id3v1genres.h> //used to load genre list
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <qdom.h>
#include <qfile.h>
#include <qtimer.h>

#include <kglobal.h>
#include <klocale.h>

/**
 * Use this to const-iterate over QStringLists, if you like.
 * Watch out for the definition of last in the scope of your for.
 *
 *     QStringList strings;
 *     foreach( strings )
 *         debug() << *it << endl;
 */
#define foreach( x ) \
    for( QStringList::ConstIterator it = x.begin(), end = x.end(); it != end; ++it )



CollectionScanner::CollectionScanner( const QStringList& folders, bool recursive, bool importPlaylists )
        : KApplication()
        , m_importPlaylists( importPlaylists )
        , m_folders( folders )
        , m_recursively( recursive )
        , log( "~/collection_scanner.log" )
{
    // don't traverse /
    struct stat statBuf;
    if( stat( "/", &statBuf ) == 0 ) {
        struct direntry de;
        memset(&de, 0, sizeof(struct direntry));
        de.dev = statBuf.st_dev;
        de.ino = statBuf.st_ino;

        m_processedDirs.resize(m_processedDirs.size()+1);
        m_processedDirs[m_processedDirs.size()-1] = de;
    }

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{}


void
CollectionScanner::doJob() //SLOT
{
    log << "Collection Scan Log\n";
    log << "===================\n";
    log << i18n( "Report this file if amaroK crashes when building the Collection." ).local8Bit();
    log << "\n\n\n";

    // we need to create the temp tables before readDir gets called ( for the dir stats )

    std::cout << "<scanner>";

    QStrList entries;
    foreach( m_folders ) {
        if( (*it).isEmpty() )
            //apparently somewhere empty strings get into the mix
            //which results in a full-system scan! Which we can't allow
            continue;

        QString dir = *it;
        if( !dir.endsWith( "/" ) )
            dir += '/';

        readDir( dir, entries );
    }

    if( !entries.isEmpty() ) {
        const QString str = QString( "<itemcount count='%1'/>" ).arg( entries.count() );
        std::cout << str.utf8();
        scanFiles( entries );
    }

    std::cout << "</scanner>";
    log.close();

    quit();
}


void
CollectionScanner::readDir( const QString& dir, QStrList& entries )
{
    // FIXME Replace all Unix calls here with portable Qt code (QDir)


    // linux specific, but this fits the 90% rule
    if( dir.startsWith("/dev") || dir.startsWith("/sys") || dir.startsWith("/proc") )
        return;

    QCString dir8Bit = QFile::encodeName( dir );

    struct stat statBuf;

    // FIXME
    if( stat( dir8Bit, &statBuf ) != 0 )
        return;

//     //update dir statistics for rescanning purposes
//     if( stat( dir8Bit, &statBuf ) == 0 )
//         CollectionDB::instance()->updateDirStats( dir, (long)statBuf.st_mtime, !m_incremental ? m_db : 0 );
//     else {
//         if( m_incremental ) {
//             CollectionDB::instance()->removeSongsInDir( dir );
//             CollectionDB::instance()->removeDirFromCollection( dir );
//         }
//         return;
//     }

    struct direntry de;
    memset(&de, 0, sizeof(struct direntry));
    de.dev = statBuf.st_dev;
    de.ino = statBuf.st_ino;

    int f = -1;

#if __GNUC__ < 4
    for(unsigned int i = 0; i < m_processedDirs.size(); ++i)
        if(memcmp(&m_processedDirs[i], &de, sizeof(direntry)) == 0) {
            f = i; break;
        }
#else
    f = m_processedDirs.find(de);
#endif

    if( ! S_ISDIR ( statBuf.st_mode) || f != -1 ) {
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

        if( entry == "." || entry == ".." )
            continue;

        entry.prepend( dir8Bit );

        if( stat( entry, &statBuf ) != 0 )
            continue;

        // loop protection
        if( ! ( S_ISDIR( statBuf.st_mode ) || S_ISREG( statBuf.st_mode ) ) )
            continue;

        if( S_ISDIR( statBuf.st_mode ) && m_recursively && entry.length() && entry[0] != '.' )
        {
            const QString file = QFile::decodeName( entry );

//             if( !m_incremental || !CollectionDB::instance()->isDirInCollection( file ) )
                // we MUST add a '/' after the dirname
                readDir( file + '/', entries );
        }

        else if( S_ISREG( statBuf.st_mode ) )
        {
            QString file = QFile::decodeName( entry );

//             if( m_importPlaylists ) {
//                 if( file.endsWith(".m3u") || file.endsWith(".pls") )
//                     QApplication::postEvent( PlaylistBrowser::instance(), new PlaylistFoundEvent( file ) );
//             }

            entries.append( entry );
        }
    }

    closedir( d );
}


void
CollectionScanner::scanFiles( const QStrList& entries )
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

        const QString path = QFile::decodeName ( it.current() );
        const QString ext = extension( path );
        const QString dir = directory( path );

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

        readTags( path, TagLib::AudioProperties::Fast );

        if( validImages.contains( ext ) )
           images += path;

//         else if( bundle.isValidMedia() )
//         {
//             CoverBundle cover( bundle.artist(), bundle.album() );
//
//             if( !covers.contains( cover ) )
//                 covers += cover;
//
//            CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );
//         }

//         // Update Compilation-flag, when this is the last loop-run
//         // or we're going to switch to another dir in the next run
//         if( path == entries.getLast() || dir != amaroK::directory( QFile::decodeName(++QStrListIterator( it )) ) )
//         {
//             // we entered the next directory
//             foreach( images )
//                 CollectionDB::instance()->addImageToAlbum( *it, covers, m_db );
//
//             CollectionDB::instance()->checkCompilations( dir, !m_incremental, m_db );
//
//             // clear now because we've processed them
//             covers.clear();
//             images.clear();
//         }
    }
}


void
CollectionScanner::readTags( const QString& path, TagLib::AudioProperties::ReadStyle readStyle )
{
    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;

    fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

    if( !fileref.isNull() )
        tag = fileref.tag();

    if( fileref.isNull() || !tag ) {
        std::cout << "<dud/>";
        return;
    }

    QDomDocument doc; // A dummy. We don't really use DOM, but SAX2
    QDomElement tags = doc.createElement( "tags" );
    tags.setAttribute( "path", path );

    #define strip( x ) TStringToQString( x ).stripWhiteSpace()
    tags.setAttribute( "title", strip( tag->title() ) );
    tags.setAttribute( "artist", strip( tag->artist() ) );
    tags.setAttribute( "album", strip( tag->album() ) );
    tags.setAttribute( "comment", strip( tag->comment() ) );
    tags.setAttribute( "genre", strip( tag->genre() ) );
    tags.setAttribute( "year", tag->year() ? QString::number( tag->year() ) : QString() );
    tags.setAttribute( "track", tag->track() ? QString::number( tag->track() ) : QString() );
    #undef strip

    QString text;
    QTextStream stream( &text, IO_WriteOnly );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    tags.save( stream, 0 );


    std::cout << text.utf8() << std::endl;
}


#include "collectionscanner.moc"

