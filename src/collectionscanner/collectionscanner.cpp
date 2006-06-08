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

#define DEBUG_PREFIX "CollectionScanner"

#include "amarok.h"
#include "collectionscanner.h"
#include "debug.h"

#include <cerrno>
#include <iostream>

#include <dirent.h>    //stat

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <qdom.h>
#include <qfile.h>
#include <qtimer.h>

#include <dcopref.h>
#include <kglobal.h>
#include <klocale.h>


/**
 * Convenience macro for const-iterating. Use like this:
 *
 *     BundleList bundles;
 *     foreachType( BundleList, bundles )
 *         debug() << *it.url() << endl;
 */
#define foreachType( Type, x ) \
    for( Type::ConstIterator it = x.begin(), end = x.end(); it != end; ++it )


CollectionScanner::CollectionScanner( const QStringList& folders,
                                      bool recursive,
                                      bool incremental,
                                      bool importPlaylists,
                                      bool restart )
        : KApplication( /*allowStyles*/ false, /*GUIenabled*/ false )
        , m_importPlaylists( importPlaylists )
        , m_folders( folders )
        , m_recursively( recursive )
        , m_incremental( incremental )
        , m_restart( restart )
        , m_logfile( amaroK::saveLocation( QString::null ) + "collection_scan.log"  )
{
    if( !restart )
        QFile::remove( m_logfile );

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{
    DEBUG_BLOCK
}


void
CollectionScanner::doJob() //SLOT
{
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    std::cout << "<scanner>";


    QStringList entries;

    if( m_restart ) {
        QFile logFile( m_logfile );
        logFile.open( IO_ReadOnly );
        QString lastFile = logFile.readAll();

        QFile folderFile( amaroK::saveLocation( QString::null ) + "collection_scan.files"   );
        folderFile.open( IO_ReadOnly );
        entries = QStringList::split( "\n", folderFile.readAll() );

        for( int count = entries.findIndex( lastFile ) + 1; count; --count )
            entries.pop_front();

//         debug() << "Restarting at: " << entries.front() << endl;
    }
    else {
        foreachType( QStringList, m_folders ) {
            if( (*it).isEmpty() )
                //apparently somewhere empty strings get into the mix
                //which results in a full-system scan! Which we can't allow
                continue;

            QString dir = *it;
            if( !dir.endsWith( "/" ) )
                dir += '/';

            readDir( dir, entries );
        }

        QFile folderFile( amaroK::saveLocation( QString::null ) + "collection_scan.files"   );
        folderFile.open( IO_WriteOnly );
        QTextStream stream( &folderFile );
        stream << entries.join( "\n" );
        folderFile.close();
    }

    if( !entries.isEmpty() ) {
        if( !m_restart ) {
            AttributeMap attributes;
            attributes["count"] = QString::number( entries.count() );
            writeElement( "itemcount", attributes );
        }

        scanFiles( entries );
    }

    std::cout << "</scanner>" << std::endl;

    quit();
}


void
CollectionScanner::readDir( const QString& dir, QStringList& entries )
{
    static DCOPRef dcopRef( "amarok", "collection" );

    // linux specific, but this fits the 90% rule
    if( dir.startsWith( "/dev" ) || dir.startsWith( "/sys" ) || dir.startsWith( "/proc" ) )
        return;

    const QCString dir8Bit = QFile::encodeName( dir );
    struct stat statBuf;
    stat( dir8Bit, &statBuf );

    struct direntry de;
    memset(&de, 0, sizeof(struct direntry));
    de.dev = statBuf.st_dev;
    de.ino = statBuf.st_ino;

    int f = -1;
#if __GNUC__ < 4
    for( unsigned int i = 0; i < m_processedDirs.size(); ++i )
        if( memcmp( &m_processedDirs[i], &de, sizeof( direntry ) ) == 0 ) {
            f = i; break;
        }
#else
    f = m_processedDirs.find( de );
#endif

    if ( ! S_ISDIR( statBuf.st_mode ) || f != -1 ) {
        debug() << "Skipping, already scanned: " << dir << endl;
        return;
    }

    AttributeMap attributes;
    attributes["path"] = dir;
    writeElement( "folder", attributes );

    m_processedDirs.resize( m_processedDirs.size() + 1 );
    m_processedDirs[m_processedDirs.size() - 1] = de;

    DIR *d = opendir( dir8Bit );
    if( d == NULL ) {
        if( errno == EACCES )
            warning() << "Skipping, no access permissions: " << dir << endl;
        return;
    }

    for( dirent *ent; ( ent = readdir( d ) ); ) {
        QCString entry (ent->d_name);
        QCString entryname (ent->d_name);

        if ( entry == "." || entry == ".." )
            continue;

        entry.prepend( dir8Bit );

        if ( stat( entry, &statBuf ) != 0 )
            continue;

        // loop protection
        if ( ! ( S_ISDIR( statBuf.st_mode ) || S_ISREG( statBuf.st_mode ) ) )
            continue;

        if ( S_ISDIR( statBuf.st_mode ) && m_recursively && entry.length() && entryname[0] != '.' )
        {
            const QString file = QFile::decodeName( entry );

            bool isInCollection = false;
            if( m_incremental )
                dcopRef.call( "isDirInCollection", file ).get( isInCollection );

            if( !m_incremental || !isInCollection )
                // we MUST add a '/' after the dirname
                readDir( file + '/', entries );
        }

        else if( S_ISREG( statBuf.st_mode ) )
            entries.append( QFile::decodeName( entry ) );
    }

    closedir( d );
}


void
CollectionScanner::scanFiles( const QStringList& entries )
{
    DEBUG_BLOCK

    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages;    validImages    << "jpg" << "png" << "gif" << "jpeg";
    QStringList validPlaylists; validPlaylists << "m3u" << "pls";

    QValueList<CoverBundle> covers;
    QStringList images;

    foreachType( QStringList, entries ) {
        const QString path = *it;
        const QString ext  = extension( path );
        const QString dir  = directory( path );

        // Write path to logfile
        if( !m_logfile.isEmpty() ) {
            QFile log( m_logfile );
            if( log.open( IO_WriteOnly ) )
                log.writeBlock( path.local8Bit(), path.length() );
        }

        if( validImages.contains( ext ) )
            images += path;

        else if( m_importPlaylists && validPlaylists.contains( ext ) ) {
            AttributeMap attributes;
            attributes["path"] = path;
            writeElement( "playlist", attributes );
        }

        else {
            MetaBundle::EmbeddedImageList images;
            MetaBundle mb( KURL::fromPathOrURL( path ), true, TagLib::AudioProperties::Fast, &images );
            const AttributeMap attributes = readTags( mb );

            if( !attributes.empty() ) {
                writeElement( "tags", attributes );

                CoverBundle cover( attributes["artist"], attributes["album"] );

                if( !covers.contains( cover ) )
                    covers += cover;

                foreachType( MetaBundle::EmbeddedImageList, images ) {
                    AttributeMap attributes;
                    attributes["path"] = path;
                    attributes["hash"] = (*it).hash();
                    attributes["description"] = (*it).description();
                    writeElement( "embed", attributes );
                }
            }
        }

        // Update Compilation-flag, when this is the last loop-run
        // or we're going to switch to another dir in the next run
        QStringList::ConstIterator itTemp( it );
        ++itTemp;
        if( path == entries.last() || dir != directory( *itTemp ) )
        {
            // we entered the next directory
            foreachType( QStringList, images ) {
                // Serialize CoverBundle list with AMAROK_MAGIC as separator
                QString string;

                for( QValueList<CoverBundle>::ConstIterator it2 = covers.begin(); it2 != covers.end(); ++it2 )
                    string += (*it2).first + "AMAROK_MAGIC" + (*it2).second + "AMAROK_MAGIC";

                AttributeMap attributes;
                attributes["path"] = *it;
                attributes["list"] = string;
                writeElement( "image", attributes );
            }

            AttributeMap attributes;
            attributes["path"] = dir;
            writeElement( "compilation", attributes );

            // clear now because we've processed them
            covers.clear();
            images.clear();
        }
    }
}


AttributeMap
CollectionScanner::readTags( const MetaBundle& mb )
{
    // Tests reveal the following:
    //
    // TagLib::AudioProperties   Relative Time Taken
    //
    //  No AudioProp Reading        1
    //  Fast                        1.18
    //  Average                     Untested
    //  Accurate                    Untested

    AttributeMap attributes;

    if ( !mb.isValidMedia() ) {
        std::cout << "<dud/>";
        return attributes;
    }

    attributes["path"]    = mb.url().path();
    attributes["title"]   = mb.title();
    attributes["artist"]  = mb.artist();
    attributes["composer"]= mb.composer();
    attributes["album"]   = mb.album();
    attributes["comment"] = mb.comment();
    attributes["genre"]   = mb.genre();
    attributes["year"]    = mb.year() ? QString::number( mb.year() ) : QString();
    attributes["track"]   = mb.track() ? QString::number( mb.track() ) : QString();
    attributes["discnumber"]   = mb.discNumber() ? QString::number( mb.discNumber() ) : QString();
    attributes["filetype"]  = QString::number( mb.fileType() );
    attributes["uniqueid"] = mb.uniqueId();
    attributes["compilation"] = QString::number( mb.compilation() );

    if ( mb.audioPropertiesUndetermined() )
        attributes["audioproperties"] = "false";
    else {
        attributes["audioproperties"] = "true";
        attributes["bitrate"]         = QString::number( mb.bitrate() );
        attributes["length"]          = QString::number( mb.length() );
        attributes["samplerate"]      = QString::number( mb.sampleRate() );
    }

    if ( mb.filesize() >= 0 )
        attributes["filesize"] = QString::number( mb.filesize() );

    return attributes;
}


void
CollectionScanner::writeElement( const QString& name, const AttributeMap& attributes )
{
    QDomDocument doc; // A dummy. We don't really use DOM, but SAX2
    QDomElement element = doc.createElement( name );

    foreachType( AttributeMap, attributes )
    {
        // There are at least some characters that Qt cannot categorize which make the resulting
        // xml document ill-formed and prevent the parser from processing the remaining document.
        // Because of this we skip attributes containing characters not belonging to any category.
        QString data = it.data();
        const unsigned len = data.length();
        bool nonPrint = false;
        for( unsigned i = 0; i < len; i++ )
        {
            if( data.ref( i ).category() == QChar::NoCategory )
            {
                nonPrint = true;
                break;
            }
        }

        if( nonPrint )
            continue;

        element.setAttribute( it.key(), it.data() );
    }

    QString text;
    QTextStream stream( &text, IO_WriteOnly );
    element.save( stream, 0 );

    std::cout << text.utf8() << std::endl;
}


#include "collectionscanner.moc"

