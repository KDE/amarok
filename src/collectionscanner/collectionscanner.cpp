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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "CollectionScanner"

#include "amarok.h"
#include "collectionscanner.h"
#include "debug.h"

#include <cerrno>
#include <iostream>

#include <dirent.h>    //stat
#include <limits.h>    //PATH_MAX
#include <stdlib.h>    //realpath

#include <audioproperties.h>
#include <fileref.h>
#include <tag.h>
#include <tstring.h>

#include <QByteArray>
#include <QDBusReply>
#include <QFile>
#include <QTimer>
#include <qdom.h>

#include <kglobal.h>
#include <klocale.h>

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
            << endl;
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
            << " read-only" << endl;
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

            if( !dir.endsWith( "/" ) )
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

    const QByteArray dir8Bit = QFile::encodeName( dir );
    DIR *d = opendir( dir8Bit );
    if( d == NULL ) {
        warning() << "Skipping, " << strerror(errno) << ": " << dir << endl;
        return;
    }
    int dfd = -1;
#ifdef USE_SOLARIS
    dfd = d->d_fd;
#else //USE_SOLARIS
#ifndef Q_WS_WIN
    dfd = dirfd(d);
#else //Q_WS_WIN
    dfd = -1;
#endif //Q_WS_WIN
#endif //USE_SOLARIS
    if (dfd == -1) {
	warning() << "Skipping, unable to obtain file descriptor: " << dir << endl;
	closedir(d);
	return;
    }

    struct stat statBuf;
    struct stat statBuf_symlink;
    fstat( dfd, &statBuf );

    struct direntry de;
    memset(&de, 0, sizeof(struct direntry));
    de.dev = statBuf.st_dev;
    de.ino = statBuf.st_ino;

    int f = -1;
    for( int i = 0; i < m_processedDirs.size(); ++i )
        if( memcmp( &m_processedDirs[i], &de, sizeof( direntry ) ) == 0 ) {
            f = i; break;
        }

    if ( ! S_ISDIR( statBuf.st_mode ) || f != -1 ) {
        debug() << "Skipping, already scanned: " << dir << endl;
        closedir(d);
        return;
    }

    AttributeMap attributes;
    attributes["path"] = dir;
    writeElement( "folder", attributes );

    m_processedDirs.resize( m_processedDirs.size() + 1 );
    m_processedDirs[m_processedDirs.size() - 1] = de;

    for( dirent *ent; ( ent = readdir( d ) ); ) {
        QByteArray entry (ent->d_name);
        QByteArray entryname (ent->d_name);

        if ( entry == "." || entry == ".." )
            continue;

        entry.prepend( dir8Bit );

        if ( stat( entry, &statBuf ) != 0 )
            continue;
        if ( lstat( entry, &statBuf_symlink ) != 0 )
            continue;

        // loop protection
        if ( ! ( S_ISDIR( statBuf.st_mode ) || S_ISREG( statBuf.st_mode ) ) )
            continue;

        if ( S_ISDIR( statBuf.st_mode ) && m_recursively && entry.length() && entryname[0] != '.' )
        {
            if ( S_ISLNK( statBuf_symlink.st_mode ) ) {
                char nosymlink[PATH_MAX];
                if ( realpath( entry, nosymlink ) ) {
                    debug() << entry << " is a symlink. Using: " << nosymlink << endl;
                    entry = nosymlink;
                }
            }
            const QString file = QFile::decodeName( entry );

            bool isInCollection = false;
            if( m_incremental )
	    {
            QDBusReply<bool> reply = amarokCollectionInterface->isDirInCollection(file); 
	    if(reply.isValid())
		 isInCollection=reply;
	    }
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
            MetaBundle::EmbeddedImageList images;
            MetaBundle mb( KUrl( path ), true, TagLib::AudioProperties::Fast, &images );
            const AttributeMap attributes = readTags( mb );

            if( !attributes.empty() ) {
                writeElement( "tags", attributes );

                CoverBundle cover( attributes["artist"], attributes["album"] );

                if( !covers.contains( cover ) )
                    covers += cover;

                oldForeachType( MetaBundle::EmbeddedImageList, images ) {
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
    attributes["bpm"]   = mb.bpm() ? QString::number( mb.bpm() ) : QString();
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

