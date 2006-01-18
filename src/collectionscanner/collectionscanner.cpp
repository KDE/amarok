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
#include "metabundle.h"
#include "debug.h"

#include <iostream>

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qtimer.h>

#include <dcopref.h>
#include <kcrash.h>
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
                                      const QString& logfile )
        : KApplication()
        , m_importPlaylists( importPlaylists )
        , m_folders( folders )
        , m_recursively( recursive )
        , m_incremental( incremental )
        , m_logfile( logfile )
{
    QFile::remove( m_logfile );

     // Disable the KDE crash handler, so that amaroK can catch SIGSEGV
    KCrash::setCrashHandler( NULL );

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{
    DEBUG_BLOCK
}


void
CollectionScanner::doJob() //SLOT
{
    // we need to create the temp tables before readDir gets called ( for the dir stats )

    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    std::cout << "<scanner>";

    QStringList entries;

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

    if( !entries.isEmpty() ) {
        AttributeMap attributes;
        attributes["count"] = QString::number( entries.count() );
        writeElement( "itemcount", attributes );

        scanFiles( entries );
    }

    std::cout << "</scanner>" << std::endl;

    quit();
}


void
CollectionScanner::readDir( const QString& path, QStringList& entries )
{
    static DCOPRef dcopRef( "amarok", "collection" );

    // linux specific, but this fits the 90% rule
    if( path.startsWith( "/dev" ) || path.startsWith( "/sys" ) || path.startsWith( "/proc" ) )
        return;
    // Protect against loops
    // FIXME This doesn't help with symlink loops. The symlinks need to be resolved
    if( m_processedFolders.contains( path ) )
        return;

    AttributeMap attributes;
    attributes["path"] = path;
    writeElement( "folder", attributes );

    m_processedFolders += path;
    QDir dir( path );


    // FILES:
    const QStringList files = dir.entryList( QDir::Files | QDir::Readable | QDir::Hidden );

    // Append file paths to list
    foreachType( QStringList, files )
        entries += dir.absFilePath( *it );


    if( !m_recursively ) return;


    // FOLDERS:
    const QStringList folders = dir.entryList( QDir::Dirs | QDir::Readable );

    // Recurse folders
    foreachType( QStringList, folders ) {
        if( (*it).startsWith( "." ) ) continue;

        const QString dirPath = dir.absFilePath( *it );

        bool isInCollection;
        if( m_incremental )
            dcopRef.call( "isDirInCollection", dirPath ).get( isInCollection );

        if( !m_incremental || !isInCollection )
            // we must add a '/' after the dirname, to avoid dupes
            readDir( dirPath + "/", entries );
    }
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
            const AttributeMap attributes = readTags( path );

            if( !attributes.empty() ) {
                writeElement( "tags", attributes );

                CoverBundle cover( attributes["artist"], attributes["album"] );

                if( !covers.contains( cover ) )
                    covers += cover;
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
CollectionScanner::readTags( const QString& path )
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

    KURL escapedPath;
    escapedPath.setPath( path );

    MetaBundle mb ( escapedPath, true, TagLib::AudioProperties::Fast );

    if ( !mb.isValidMedia() ) {
        std::cout << "<dud/>";
        return attributes;
    }

    attributes["path"]    = path;
    attributes["title"]   = mb.title();
    attributes["artist"]  = mb.artist();
    attributes["composer"]= mb.composer();
    attributes["album"]   = mb.album();
    attributes["comment"] = mb.comment();
    attributes["genre"]   = mb.genre();
    attributes["year"]    = mb.year() ? QString::number( mb.year() ) : QString();
    attributes["track"]   = mb.track() ? QString::number( mb.track() ) : QString();
    attributes["discnumber"]   = mb.discNumber() ? QString::number( mb.discNumber() ) : QString();

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

