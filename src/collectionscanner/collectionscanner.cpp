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

    QStringList entries;
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
CollectionScanner::readDir( const QString& path, QStringList& entries )
{
    //TODO Add recursive symlink loop protection

    // linux specific, but this fits the 90% rule
    if( path.startsWith( "/dev" ) || path.startsWith( "/sys" ) || path.startsWith( "/proc" ) )
        return;

    QDir dir( path );


    // FILES:
    const QStringList files = dir.entryList( QDir::Files | QDir::Readable );

    // Append file paths to list
    for( QStringList::ConstIterator it = files.begin(); it != files.end(); ++it )
        entries += dir.absFilePath( *it );


    if( !m_recursively ) return;


    // FOLDERS:
    const QStringList dirs = dir.entryList( QDir::Dirs | QDir::Readable );

    // Recurse folders
    for( QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it ) {
        if( (*it).startsWith( "." ) ) continue;
        readDir( dir.absFilePath( *it ), entries );
    }
}


void
CollectionScanner::scanFiles( const QStringList& entries )
{
    DEBUG_BLOCK

    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages; validImages << "jpg" << "png" << "gif" << "jpeg";
//    QStringList validMusic; validMusic << "mp3" << "ogg" << "wav" << "flac";

    QValueList<CoverBundle> covers;
    QStringList images;

    for( QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it ) {

        const QString path = *it;
        const QString ext  = extension( path );
        const QString dir  = directory( path );

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

        readTags( path );

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
CollectionScanner::readTags( const QString& path )
{
    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;

    fileref = TagLib::FileRef( QFile::encodeName( path ), true, TagLib::AudioProperties::Fast );

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

    TagLib::AudioProperties* ap = fileref.audioProperties();
    if( ap ) {
        tags.setAttribute( "audioproperties", "true" );
        tags.setAttribute( "bitrate",    QString::number( ap->bitrate() ) );
        tags.setAttribute( "length",     QString::number( ap->length() ) );
        tags.setAttribute( "samplerate", QString::number( ap->sampleRate() ) );
    }
    else
        tags.setAttribute( "audioproperties", "false" );


    QString text;
    QTextStream stream( &text, IO_WriteOnly );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    tags.save( stream, 0 );


    std::cout << text.utf8() << std::endl;
}


#include "collectionscanner.moc"

