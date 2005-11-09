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

#include <iostream>

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/id3v1genres.h> //used to load genre list
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qtimer.h>

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
                                      bool importPlaylists,
                                      const QString& logfile )
        : KApplication()
        , m_importPlaylists( importPlaylists )
        , m_folders( folders )
        , m_recursively( recursive )
        , m_logfile( logfile )
{
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
    // linux specific, but this fits the 90% rule
    if( path.startsWith( "/dev" ) || path.startsWith( "/sys" ) || path.startsWith( "/proc" ) )
        return;
    // Protect against dupes and symlink loops
    if( m_processedFolders.contains( path ) )
        return;

    AttributeMap attributes;
    attributes["path"] = path;
    writeElement( "folder", attributes );

    m_processedFolders += path;
    QDir dir( path );


    // FILES:
    const QStringList files = dir.entryList( QDir::Files | QDir::Readable );

    // Append file paths to list
    foreachType( QStringList, files )
        entries += dir.absFilePath( *it );


    if( !m_recursively ) return;


    // FOLDERS:
    const QStringList folders = dir.entryList( QDir::Dirs | QDir::Readable );

    // Recurse folders
    foreachType( QStringList, folders ) {
        if( (*it).startsWith( "." ) ) continue;
        // we must add a '/' after the dirname, to avoid dupes
        readDir( dir.absFilePath( *it ) + "/", entries );
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

#if 0
        // Update Compilation-flag, when this is the last loop-run
        // or we're going to switch to another dir in the next run
        if( path == entries.getLast() || dir != ++QStringListIterator( it ) )
        {
            // we entered the next directory
            foreach( images )
                AttributeMap attributes;
                attributes["path"] = *it;
                writeElement( "image", attributes );

//             CollectionDB::instance()->checkCompilations( dir, !m_incremental, m_db );

            // clear now because we've processed them
            covers.clear();
            images.clear();
        }
#endif
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

    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;

    fileref = TagLib::FileRef( QFile::encodeName( path ), true, TagLib::AudioProperties::Fast );

    if( !fileref.isNull() )
        tag = fileref.tag();

    if( fileref.isNull() || !tag ) {
        std::cout << "<dud/>";
        return attributes;
    }

    #define strip( x ) TStringToQString( x ).stripWhiteSpace()
    attributes["path"]    = path;
    attributes["title"]   = strip( tag->title() );
    attributes["artist"]  = strip( tag->artist() );
    attributes["album"]   = strip( tag->album() );
    attributes["comment"] = strip( tag->comment() );
    attributes["genre"]   = strip( tag->genre() );
    attributes["year"]    = tag->year() ? QString::number( tag->year() ) : QString();
    attributes["track"]   = tag->track() ? QString::number( tag->track() ) : QString();
    #undef strip

    TagLib::AudioProperties* ap = fileref.audioProperties();
    if( ap ) {
        attributes["audioproperties"] = "true";
        attributes["bitrate"]         = QString::number( ap->bitrate() );
        attributes["length"]          = QString::number( ap->length() );
        attributes["samplerate"]      = QString::number( ap->sampleRate() );
    }
    else
        attributes["audioproperties"] = "false";


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
            if( !data.ref(i).isPrint() || data.ref( i ).category() == QChar::NoCategory )
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

