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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "CollectionScanner"

#include "amarok.h"
#include "collectionscanner.h"
#include "debug.h"
#include "meta/file/File.h"

#include <KMD5>

#include <cerrno>
#include <iostream>

#include <limits.h>    //PATH_MAX
#include <stdlib.h>    //realpath

#include <audioproperties.h>
#include <fileref.h>
#include <tag.h>
#include <tstring.h>

#include <QByteArray>
#include <QDir>
#include <QDBusReply>
#include <QFile>
#include <QTimer>
#include <qdom.h>

#include <kglobal.h>
#include <klocale.h>

//Taglib:
#include <apetag.h>
#include <flacfile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <tlist.h>
#include <tstring.h>
#include <vorbisfile.h>

#ifdef HAVE_MP4V2
#include "metadata/mp4/mp4file.h"
#include "metadata/mp4/mp4tag.h"
#else
#include "metadata/m4a/mp4file.h"
#include "metadata/m4a/mp4itunestag.h"
#endif

#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <xiphcomment.h>

#include <amarok_collection_interface.h>

CollectionScanner::CollectionScanner( const QStringList& folders,
                                      bool recursive,
                                      bool incremental,
                                      bool importPlaylists,
                                      bool restart )
        : KApplication( /*GUIenabled*/ false )
        , m_importPlaylists( importPlaylists )
        , m_folders( folders )
        , m_recursively( recursive )
        , m_incremental( incremental )
        , m_restart( restart )
        , m_logfile( Amarok::saveLocation( QString() ) + "collection_scan.log"  )
{
    amarokCollectionInterface = new OrgKdeAmarokCollectionInterface("org.kde.amarok", "/Collection", QDBusConnection::sessionBus());
    kapp->setObjectName( QString( "amarokcollectionscanner" ).toAscii() );
    if( !restart )
        QFile::remove( m_logfile );

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{
    DEBUG_BLOCK
    delete amarokCollectionInterface;
}


void
CollectionScanner::doJob() //SLOT
{
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    std::cout << "<scanner>";


    QStringList entries;

    if( m_restart ) {
        QFile logFile( m_logfile );
        QString lastFile;
        if ( !logFile.open( QIODevice::ReadOnly ) )
            warning() << "Failed to open log file " << logFile.fileName() << " read-only"
           ;
        else {
            QTextStream logStream;
            logStream.setDevice(&logFile);
            logStream.setCodec(QTextCodec::codecForName( "UTF-8" ) );
            lastFile = logStream.readAll();
            logFile.close();
        }

        QFile folderFile( Amarok::saveLocation( QString() ) + "collection_scan.files"   );
        if ( !folderFile.open( QIODevice::ReadOnly ) )
            warning() << "Failed to open folder file " << folderFile.fileName()
            << " read-only";
        else {
            QTextStream folderStream;
            folderStream.setDevice(&folderFile);
            folderStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
            entries = folderStream.readAll().split( "\n" );
        }

        for( int count = entries.indexOf( lastFile ) + 1; count; --count )
            entries.pop_front();

    }
    else {
        foreach( QString dir, m_folders ) {
            if( dir.isEmpty() )
                //apparently somewhere empty strings get into the mix
                //which results in a full-system scan! Which we can't allow
                continue;

            if( !dir.endsWith( '/' ) )
                dir += '/';
            readDir( dir, entries );
        }

        QFile folderFile( Amarok::saveLocation( QString() ) + "collection_scan.files"   );
        folderFile.open( QIODevice::WriteOnly );
        QTextStream stream( &folderFile );
        stream.setCodec( QTextCodec::codecForName("UTF-8") );
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
    // linux specific, but this fits the 90% rule
    if( dir.startsWith( "/dev" ) || dir.startsWith( "/sys" ) || dir.startsWith( "/proc" ) )
        return;
    QDir d( dir );
    m_scannedFolders << d.canonicalPath();

    if( !d.exists() )
       return;

    AttributeMap attributes;
    attributes["path"] = dir;
    writeElement( "folder", attributes );
    d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Readable );
    foreach( QFileInfo f, d.entryInfoList() )
    {
        if( !f.exists() )
            break;

        if( f.isSymLink() )
            f = QFileInfo(f.symLinkTarget() );

        if( f.isDir() && m_recursively && !m_scannedFolders.contains( f.canonicalFilePath() ) )
        {
            readDir( f.absoluteFilePath() + '/', entries );
        }
        else if( f.isFile() )
            entries.append( f.absoluteFilePath() );
    }
}


void
CollectionScanner::scanFiles( const QStringList& entries )
{
    DEBUG_BLOCK

    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages;    validImages    << "jpg" << "png" << "gif" << "jpeg";
    QStringList validPlaylists; validPlaylists << "m3u" << "pls";

    QList<CoverBundle> covers;
    QStringList images;

    int itemCount = 0;

    oldForeachType( QStringList, entries ) {
        const QString path = *it;
        const QString ext  = extension( path );
        const QString dir  = directory( path );

        itemCount++;

        // Write path to logfile
        if( !m_logfile.isEmpty() ) {
            QFile log( m_logfile );
            if( log.open( QIODevice::WriteOnly ) ) {
                QByteArray cPath = path.toUtf8();
                log.write( cPath, cPath.length() );
                log.close();
            }
        }

        if( validImages.contains( ext ) )
            images += path;

        else if( m_importPlaylists && validPlaylists.contains( ext ) ) {
            AttributeMap attributes;
            attributes["path"] = path;
            writeElement( "playlist", attributes );
        }

        else {
            //FIXME: PORT 2.0
//             QList<EmbeddedImage> images;
            const AttributeMap attributes = readTags( path );

            if( !attributes.empty() ) {
                writeElement( "tags", attributes );

                CoverBundle cover( attributes["artist"], attributes["album"] );

                if( !covers.contains( cover ) )
                    covers += cover;

                //FIXME: PORT 2.0
//                 foreach( EmbeddedImage image, images )
//                 {
//                     AttributeMap attributes;
//                     attributes["path"] = path;
//                     attributes["hash"] = image.hash();
//                     attributes["description"] = image.description();
//                     writeElement( "embed", attributes );
//                 }
            }
        }

        // Update Compilation-flag, when this is the last loop-run
        // or we're going to switch to another dir in the next run
        QStringList::ConstIterator itTemp( it );
        ++itTemp;
        if( path == entries.last() || dir != directory( *itTemp ) )
        {
            // we entered the next directory
            oldForeachType( QStringList, images ) {
                // Serialize CoverBundle list with AMAROK_MAGIC as separator
                QString string;

                for( QList<CoverBundle>::ConstIterator it2 = covers.begin(); it2 != covers.end(); ++it2 )
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
CollectionScanner::readTags( const QString &path, TagLib::AudioProperties::ReadStyle readStyle )
{
    // Tests reveal the following:
    //
    // TagLib::AudioProperties   Relative Time Taken
    //
    //  No AudioProp Reading        1
    //  Fast                        1.18
    //  Average                     Untested
    //  Accurate                    Untested

    ScanMetaData smd;

    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;
    fileref = TagLib::FileRef( TagLibEncodeName( path ), true, readStyle );

    if( !fileref.isNull() )
    {
        smd.filesize = QFile( path ).size();
        smd.length = fileref.audioProperties()->length();
        smd.bitrate = fileref.audioProperties()->bitrate();
        smd.samplerate = fileref.audioProperties()->sampleRate();

        tag = fileref.tag();
        if ( tag )
        {
#define strip( x ) TStringToQString( x ).trimmed()
            smd.title = strip( tag->title() );
            smd.artist = strip( tag->artist() );
            smd.album = strip( tag->album() );
            smd.comment = strip( tag->comment() );
            smd.genre = strip( tag->genre() );
            smd.year = QString::number( tag->year() );
            smd.tracknr  = QString::number( tag->track() );
#undef strip
            smd.isValid = true;
        }


    /* As mpeg implementation on TagLib uses a Tag class that's not defined on the headers,
        we have to cast the files, not the tags! */

        QString disc;
        QString compilation;
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            smd.filetype = ScanMetaData::mp3;
            if ( file->ID3v2Tag() )
            {
                if ( !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() )
                    disc = TStringToQString( file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TBPM"].isEmpty() )
                    smd.bpm = TStringToQString( file->ID3v2Tag()->frameListMap()["TBPM"].front()->toString() ).trimmed().toFloat();

                if ( !file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() )
                    smd.composer = TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TPE2"].isEmpty() ) // non-standard: Apple, Microsoft
                    smd.artist = TStringToQString( file->ID3v2Tag()->frameListMap()["TPE2"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TCMP"].isEmpty() )
                    compilation = TStringToQString( file->ID3v2Tag()->frameListMap()["TCMP"].front()->toString() ).trimmed();

                //FIXME: Port 2.0
//                 if(images) {
//                     loadImagesFromTag( *file->ID3v2Tag(), *images );
//                 }
            }
        }
        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
        {
            smd.filetype = ScanMetaData::ogg;
            if ( file->tag() )
            {
                if ( !file->tag()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    smd.composer = TStringToQString( file->tag()->fieldListMap()["COMPOSER"].front() ).trimmed();

                if ( !file->tag()->fieldListMap()[ "BPM" ].isEmpty() )
                    smd.bpm = TStringToQString( file->tag()->fieldListMap()["BPM"].front() ).trimmed().toFloat();

                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).trimmed();

                if ( !file->tag()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->tag()->fieldListMap()["COMPILATION"].front() ).trimmed();
            }
        }
        else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            smd.filetype = ScanMetaData::flac;
            if ( file->xiphComment() )
            {
                if ( !file->xiphComment()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    smd.composer = TStringToQString( file->xiphComment()->fieldListMap()["COMPOSER"].front() ).trimmed();

                if ( !file->xiphComment()->fieldListMap()[ "BPM" ].isEmpty() )
                    smd.bpm = TStringToQString( file->xiphComment()->fieldListMap()["BPM"].front() ).trimmed().toFloat();

                if ( !file->xiphComment()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->xiphComment()->fieldListMap()["DISCNUMBER"].front() ).trimmed();

                if ( !file->xiphComment()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->xiphComment()->fieldListMap()["COMPILATION"].front() ).trimmed();
            }
//             if ( images && file->ID3v2Tag() ) {
//                 loadImagesFromTag( *file->ID3v2Tag(), *images );
//             }
        }
        else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
        {
            smd.filetype = ScanMetaData::mp4;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            if( mp4tag )
            {
                smd.composer = TStringToQString( mp4tag->composer() );
                smd.bpm = QString::number( mp4tag->bpm() ).toFloat();
                disc = QString::number( mp4tag->disk() );
                compilation = QString::number( mp4tag->compilation() );

//                 if ( images && mp4tag->cover().size() ) {
//                     images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
//                 }
            }
        }

        if ( !disc.isEmpty() )
        {
            int i = disc.indexOf('/');
            if ( i != -1 )
                // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
                smd.discnumber = disc.left( i ).toInt();
            else
                smd.discnumber = disc.toInt();
        }

        if ( compilation.isEmpty() ) {
            // well, it wasn't set, but if the artist is VA assume it's a compilation
            if ( smd.artist == i18n( "Various Artists" ) )
                smd.isCompilation = true;
        } else {
            int i = compilation.toInt();
            if ( i == 0 )
                smd.isCompilation = false;
            else if ( i == 1 )
                smd.isCompilation = true;
        }
    }

    AttributeMap attributes;

    if ( !smd.isValid ) {
        std::cout << "<dud/>";
        return attributes;
    }

    attributes["path"]    = path;
    attributes["title"]   = smd.title;
    attributes["artist"]  = smd.artist;
    attributes["composer"]= smd.composer;
    attributes["album"]   = smd.album;
    attributes["comment"] = smd.comment;
    attributes["genre"]   = smd.genre;
    attributes["year"]    = smd.year;
    attributes["track"]   = smd.tracknr;
    attributes["discnumber"]   = QString::number( smd.discnumber );
    attributes["bpm"]   = QString::number( smd.bpm );
    attributes["filetype"]  = QString::number( smd.filetype );
//     attributes["uniqueid"] = QString::number( -1 ); //FIXME: Port
    attributes["compilation"] = QString::number( smd.isCompilation );
    static const int Undetermined = -2;
    if ( smd.bitrate == Undetermined || smd.length == Undetermined || smd.samplerate == Undetermined )
        attributes["audioproperties"] = "false";
    else {
        attributes["audioproperties"] = "true";
        attributes["bitrate"]         = QString::number( smd.bitrate );
        attributes["length"]          = QString::number( smd.length );
        attributes["samplerate"]      = QString::number( smd.samplerate );
    }

    if ( smd.filesize >= 0 )
        attributes["filesize"] = QString::number( smd.filesize );

    return attributes;
}


void
CollectionScanner::writeElement( const QString& name, const AttributeMap& attributes )
{
    QDomDocument doc; // A dummy. We don't really use DOM, but SAX2
    QDomElement element = doc.createElement( name );

    oldForeachType( AttributeMap, attributes )
    {
        // There are at least some characters that Qt cannot categorize which make the resulting
        // xml document ill-formed and prevent the parser from processing the remaining document.
        // Because of this we skip attributes containing characters not belonging to any category.
        QString data = it.value();
        const unsigned len = data.length();
        bool nonPrint = false;
        for( unsigned i = 0; i < len; i++ )
        {
            if( data[i].category() == QChar::NoCategory )
            {
                nonPrint = true;
                break;
            }
        }

        if( nonPrint )
            continue;

        element.setAttribute( it.key(), it.value() );
    }

    QString text;
    QTextStream stream( &text, QIODevice::WriteOnly );
    element.save( stream, 0 );


    std::cout << text.toUtf8().data() << std::endl;
}

#include "collectionscanner.moc"

