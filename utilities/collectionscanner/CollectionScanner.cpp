/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CollectionScanner.h"
#include "AFTUtility.h"

#include "charset-detector/include/chardet.h"
#include "MetaReplayGain.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits.h>    //PATH_MAX

#include <QByteArray>
#include <QDBusReply>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QtDebug>
#include <QTextCodec>
#include <QTextStream>
#include <QTime>
#include <QTimer>

//Taglib:
#include <apetag.h>
#include <fileref.h>
#include <flacfile.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4item.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <tlist.h>
#include <tstring.h>
#include <vorbisfile.h>

#include <audiblefiletyperesolver.h>
#include <realmediafiletyperesolver.h>
#include "shared/taglib_filetype_resolvers/asffiletyperesolver.h"
#include "shared/taglib_filetype_resolvers/mp4filetyperesolver.h"
#include "shared/taglib_filetype_resolvers/wavfiletyperesolver.h"

#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <xiphcomment.h>

#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

int
main( int argc, char *argv[] )
{
    CollectionScanner scanner( argc, argv );
    return scanner.exec();
}

static QTextStream s_textStream( stderr );

CollectionScanner::CollectionScanner( int &argc, char **argv )
        : QCoreApplication( argc, argv )
        , m_collectionId() //UNUSED, problems with DBus
        , m_amarokPid() //UNUSED, problems with DBus
        , m_batch( false )
        , m_charset( true )
        , m_importPlaylists( false )
        , m_batchFolderTime()
        , m_recursively( false )
        , m_incremental( false )
        , m_restart( false )
        , m_amarokCollectionInterface( 0 )
{
    setObjectName( "amarokcollectionscanner" );

    readArgs();

    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new ASFFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WAVFileTypeResolver);

    m_logfile = ( m_batch ? ( m_incremental ? "amarokcollectionscanner_batchincrementalscan.log" : "amarokcollectionscanner_batchfullscan.log" )
                       : m_saveLocation + "collection_scan.log" );

    if( !m_restart )
        QFile::remove( m_logfile );

    m_amarokCollectionInterface = new QDBusInterface( "org.kde.amarok", "/SqlCollection" );

    if( m_batch && m_incremental )
    {
        bool success = readBatchIncrementalFile();
        if( !success )
            return;
    }

    if( !m_mtimeFile.isEmpty() && m_incremental )
    {
        bool success = readMtimeFile();
        if( !success )
            return;
    }

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{
    delete m_amarokCollectionInterface;
}

//Populates m_folders with folders from that pointed to file, but only if the mtime of the folder
//is greater than the mtime of the file itself
bool
CollectionScanner::readBatchIncrementalFile()
{
    QString filePath = m_folders.first();
    if( !QFile::exists( filePath ) )
        return false;

    m_batchFolderTime = QFileInfo( filePath ).lastModified();

    QFile folderFile( filePath );
    if( !folderFile.open( QIODevice::ReadOnly ) )
        return false;

    m_folders.clear();

    QTextStream folderStream;
    folderStream.setDevice( &folderFile );

    QString temp = folderStream.readLine();
    while( !temp.isEmpty() )
    {
        QStringList parts  = temp.split( "_AMAROKMTIME_" );
        QString dir = parts[0];
        QFileInfo info( dir );
        if( info.exists() && info.isDir() )
        {
            QDateTime lastMod = info.lastModified();
            if( lastMod > m_batchFolderTime )
                m_folders << dir;
        }
        //TODO: rpath substitution?
        temp = folderStream.readLine();
    }

    folderFile.close();
    return true;
}

//Populate m_folders with folders, but also m_mtimes with mtimes
bool
CollectionScanner::readMtimeFile()
{
    QString filePath = m_mtimeFile;
    if( !QFile::exists( filePath ) )
        return false;

    QFile folderFile( filePath );
    if( !folderFile.open( QIODevice::ReadOnly ) )
        return false;

    m_folders.clear();

    QTextStream folderStream;
    folderStream.setDevice( &folderFile );

    QString temp = folderStream.readLine();
    while( !temp.isEmpty() )
    {
        QStringList parts  = temp.split( "_AMAROKMTIME_" );
        m_folders << parts[0];
        m_mTimeMap[parts[0]] = parts[1].toUInt();
        temp = folderStream.readLine();
    }

    //qDebug() << "contents of folders: " << m_folders << endl;
    //qDebug() << "contents of mtimemap: " << m_mTimeMap << endl;
    folderFile.close();
    return true;
}

void
CollectionScanner::doJob() //SLOT
{
    if( !m_restart )
    {
        std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
        std::cout << "<scanner>";
    }

    QStringList entries;

    if( m_restart )
    {
        QFile logFile( m_logfile );
        QString lastFile;
        if( logFile.open( QIODevice::ReadOnly ) )
        {
            QTextStream logStream;
            logStream.setDevice(&logFile);
            logStream.setCodec(QTextCodec::codecForName( "UTF-8" ) );
            lastFile = logStream.readAll();
            logFile.close();
        }

        QFile folderFile;
        if( !m_batch )
            folderFile.setFileName( m_saveLocation  + "collection_scan.files"   );
        else if( m_incremental )
            folderFile.setFileName( "amarokcollectionscanner_batchincrementalscan.files" );
        else
            folderFile.setFileName( "amarokcollectionscanner_batchfullscan.files" );
        if( folderFile.open( QIODevice::ReadOnly ) )
        {
            QTextStream folderStream;
            folderStream.setDevice(&folderFile);
            folderStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
            entries = folderStream.readAll().split( '\n' );
        }

        for( int count = entries.indexOf( lastFile ) + 1; count; --count )
            entries.pop_front();
    }
    else
    {
        foreach( QString dir, m_folders ) // krazy:exclude=foreach
        {
            if( dir.isEmpty() )
                //apparently somewhere empty strings get into the mix
                //which results in a full-system scan! Which we can't allow
                continue;

            // Make sure that all paths are absolute, not relative
            if( QDir::isRelativePath( dir ) )
                dir = QDir::cleanPath( QDir::currentPath() + '/' + dir );

            if( !dir.endsWith( '/' ) )
                dir += '/';

            readDir( dir, entries );
        }

        QFile folderFile;
        if( !m_batch )
            folderFile.setFileName( m_saveLocation + "collection_scan.files" );
        else if( m_incremental )
            folderFile.setFileName( "amarokcollectionscanner_batchincrementalscan.files" );
        else
            folderFile.setFileName( "amarokcollectionscanner_batchfullscan.files" );
        if ( folderFile.open( QIODevice::WriteOnly ) )
        {
            QTextStream stream( &folderFile );
            stream.setCodec( QTextCodec::codecForName("UTF-8") );
            stream << entries.join( "\n" );
            folderFile.close();
        }
    }

    if( !entries.isEmpty() )
    {
        if( !m_restart )
        {
            AttributeHash attributes;
            attributes["count"] = QString::number( entries.count() );
            writeElement( "itemcount", attributes );
        }

        scanFiles( entries );
    }

    std::cout << "</scanner>" << std::endl;

    if( m_batch )
    {
        if( m_incremental )
        {
            QFile::remove( "amarokcollectionscanner_batchincrementalscan.files" );
            QFile::remove( "amarokcollectionscanner_batchincrementalscan.log" );
        }
        else
        {
            QFile::remove( "amarokcollectionscanner_batchfullscan.files" );
            QFile::remove( "amarokcollectionscanner_batchfullscan.log" );
        }
    }

    quit();
}

void
CollectionScanner::readDir( const QString& dir, QStringList& entries )
{
    // Linux specific, but this fits the 90% rule
    if( dir.startsWith( "/dev" ) || dir.startsWith( "/sys" ) || dir.startsWith( "/proc" ) )
        return;

    //qDebug() << "Checking dir " << dir;
    if( m_scannedDirs.contains( dir ) )
    {
        //qDebug() << "Not scanning dir because already scanned";
        return;
    }

    m_scannedDirs << dir;

    if( m_incremental && m_mTimeMap.contains( dir ) )
    {
        //qDebug() << "Found in mTimeMap!";
        uint mtime = m_mTimeMap[dir];
        QFileInfo info( dir );
        //qDebug() << "info lastmodified: " << info.lastModified().toTime_t();
        //qDebug() << "mtime = " << mtime;
        if( !info.exists() || info.lastModified().toTime_t() == mtime )
        {
        //    qDebug() << "mtimes mean no scanning";
            return;
        }
        //qDebug() << "Going ahead with the scan";
    }

    QDir d( dir );
    m_scannedFolders << d.canonicalPath();

    if( !d.exists() )
        return;
    AttributeHash attributes;
    if( m_batch && !m_rpath.isEmpty() )
    {
        QString rdir = dir;
        rdir.remove( QDir::cleanPath( QDir::currentPath() ) );
        rdir.prepend( QDir::cleanPath( m_rpath + '/' ) );
        attributes["path"] = rdir;
    }
    else
        attributes["path"] = dir;
    writeElement( "folder", attributes );
    d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files );
    QFileInfoList list = d.entryInfoList();

    QStringList recurseDirs;
    foreach( QFileInfo f, list )
    {
        if( !f.exists() )
            break;

        if( f.isSymLink() )
            f = QFileInfo( f.symLinkTarget() );

        if( f.isDir() && m_recursively && !m_scannedFolders.contains( f.canonicalFilePath() ) )
        {
            //The following D-Bus call is used to see if a found folder is new or not
            //During an incremental scan the scanning isn't really recursive, as all folders
            //are stored in the database (even folders implicitly selected by top-level directories)
            //if recursive scanning is selected.  So we don't scan recursively because if any of those
            //folders have updates ScanManager has already figured it out.  Hence why we only scan
            //if isDirInCollection is false: it means the directory is new and we don't know about it
            bool isInCollection = false;
            if( m_incremental && m_amarokCollectionInterface )
            {
                QDBusReply<bool> reply = m_amarokCollectionInterface->call( "isDirInCollection", f.canonicalFilePath() );
                if( reply.isValid() )
                    isInCollection = reply.value();
            }

            if( !m_incremental || !isInCollection )
                recurseDirs << QString( f.absoluteFilePath() + '/' );
        }
        else if( f.isFile() )
            entries.append( f.absoluteFilePath() );
    }
    foreach( QString dir, recurseDirs )
        readDir( dir, entries );
}


void
CollectionScanner::scanFiles( const QStringList& entries )
{
    typedef QPair<QString, QString> CoverBundle;

    QStringList validImages;    validImages    << "jpg" << "png" << "gif" << "jpeg" << "bmp";
    QStringList validPlaylists; validPlaylists << "m3u" << "pls";

    QList<CoverBundle> covers;
    QStringList images;

    int itemCount = 0;

    for( QStringList::ConstIterator it = entries.constBegin(), end = entries.constEnd(); it != end; ++it )
    {
        const QString path = *it;
        const QString ext  = extension( path );
        const QString dir  = directory( path );

        itemCount++;

        // Write path to logfile
        if( !m_logfile.isEmpty() )
        {
            QFile log( m_logfile );
            if( log.open( QIODevice::WriteOnly ) )
            {
                QByteArray cPath = path.toUtf8();
                log.write( cPath, cPath.length() );
                log.close();
            }
        }

        if( validImages.contains( ext ) )
            images += path;

        else if( m_importPlaylists && validPlaylists.contains( ext ) )
        {
            AttributeHash attributes;
            if( m_batch && !m_rpath.isEmpty() )
            {
                QString rpath = path;
                rpath.remove( QDir::cleanPath( QDir::currentPath() ) );
                rpath.prepend( QDir::cleanPath( m_rpath + '/' ) );
                attributes["path"] = rpath;
            }
            else
                attributes["path"] = path;
            writeElement( "playlist", attributes );
        }

        else
        {
            //FIXME: PORT 2.0
//             QList<EmbeddedImage> images;
            const AttributeHash attributes = readTags( path );

            if( !attributes.empty() )
            {
                writeElement( "tags", attributes );

                CoverBundle cover( attributes["artist"], attributes["album"] );

                if( !covers.contains( cover ) )
                    covers += cover;

                //FIXME: PORT 2.0
//                 foreach( EmbeddedImage image, images )
//                 {
//                     AttributeHash attributes;
//                     if( m_batch && !m_rpath.isEmpty() )
//                     {
//                         QString rpath = path;
//                         rpath.remove( QDir::cleanPath( QDir::currentPath() ) );
//                         rpath.prepend( QDir::cleanPath( m_rpath + '/' ) );
//                         attributes["path"] = rpath;
//                     }
//                     else
//                         attributes["path"] = path;
//                     attributes["hash"] = image.hash();
//                     attributes["description"] = image.description();
//                     writeElement( "embed", attributes );
//                 }
            }
        }

        // Update Compilation-flag, when this is the last loop-run
        // or we're going to switch to another dir in the next run
        QStringList::const_iterator itTemp( it );
        ++itTemp;
        if( path == entries.last() || dir != directory( *itTemp ) )
        {
            // we entered the next directory
            foreach( const QString &imagePath, images )
            {
                // Serialize CoverBundle list with AMAROK_MAGIC as separator
                QString string;

                for( QList<CoverBundle>::ConstIterator it2 = covers.constBegin(); it2 != covers.constEnd(); ++it2 )
                {
                    string += ( string.isEmpty() ? "" : "AMAROK_MAGIC" ) + (*it2).first + "AMAROK_MAGIC" + (*it2).second;
                }

                AttributeHash attributes;
                if( m_batch && !m_rpath.isEmpty() )
                {
                    QString rpath = imagePath;
                    rpath.remove( QDir::cleanPath( QDir::currentPath() ) );
                    rpath.prepend( QDir::cleanPath( m_rpath + '/' ) );
                    attributes["path"] = rpath;
                }
                else
                    attributes["path"] = imagePath;
                attributes["list"] = string;
                writeElement( "image", attributes );
            }

            AttributeHash attributes;
            if( m_batch && !m_rpath.isEmpty() )
            {
                QString rdir = dir;
                rdir.remove( QDir::cleanPath( QDir::currentPath() ) );
                rdir.prepend( QDir::cleanPath( m_rpath + '/' ) );
                attributes["path"] = rdir;
            }
            else
                attributes["path"] = dir;
            writeElement( "compilation", attributes );

            // clear now because we've processed them
            covers.clear();
            images.clear();
        }
    }
}

AttributeHash
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


#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(path.utf16());
#else
    QByteArray fileName = QFile::encodeName( path );
    const char * encodedName = fileName.constData(); // valid as long as fileName exists
#endif

    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;
    fileref = TagLib::FileRef( encodedName, true, readStyle );

    AttributeHash attributes;
    bool isValid = false;
    FileType fileType = ogg;
    if( !fileref.isNull() )
    {
        tag = fileref.tag();
        if ( tag )
        {
            #define strip( x ) TStringToQString( x ).trimmed()

            attributes["title"] = strip( tag->title() );
            attributes["artist"] = strip( tag->artist() );
            attributes["album"] = strip( tag->album() );
            attributes["comment"] = strip( tag->comment() );
            attributes["genre"] = strip( tag->genre() );
            attributes["year"] = QString::number( tag->year() );
            attributes["track"]  = QString::number( tag->track() );
            isValid = true;
        }

        Meta::ReplayGainTagMap replayGainTags = Meta::readReplayGainTags( fileref );
        if ( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        {
            attributes["trackgain"] = QString::number( replayGainTags[Meta::ReplayGain_Track_Gain] );
            if ( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
                attributes["trackpeakgain"] = QString::number( replayGainTags[Meta::ReplayGain_Track_Peak] );
        }
        if ( replayGainTags.contains( Meta::ReplayGain_Album_Gain ) )
        {
            attributes["albumgain"] = QString::number( replayGainTags[Meta::ReplayGain_Album_Gain] );
            if ( replayGainTags.contains( Meta::ReplayGain_Album_Peak ) )
                attributes["albumpeakgain"] = QString::number( replayGainTags[Meta::ReplayGain_Album_Peak] );
        }

        QString disc;
        QString compilation;

        /* As MPEG implementation on TagLib uses a Tag class that's not defined on the headers,
           we have to cast the files, not the tags! */
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            fileType = mp3;
            if ( file->ID3v2Tag() )
            {
                if ( !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() )
                    disc = TStringToQString( file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TBPM"].isEmpty() )
                    attributes["bpm"] = TStringToQString( file->ID3v2Tag()->frameListMap()["TBPM"].front()->toString() ).trimmed().toFloat();

                if ( !file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() )
                    attributes["composer"] = TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TPE2"].isEmpty() ) // non-standard: Apple, Microsoft
                    attributes["albumArtist"] = TStringToQString( file->ID3v2Tag()->frameListMap()["TPE2"].front()->toString() ).trimmed();

                if ( !file->ID3v2Tag()->frameListMap()["TCMP"].isEmpty() )
                    compilation = TStringToQString( file->ID3v2Tag()->frameListMap()["TCMP"].front()->toString() ).trimmed();

                //FIXME: Port 2.0
//                 if( images )
//                     loadImagesFromTag( *file->ID3v2Tag(), *images );
            }
// HACK: charset-detector disabled, so all tags assumed utf-8
// TODO: fix charset-detector to detect encoding with higher accuracy

            if( m_charset && tag )
            {
                TagLib::String metaData = tag->title() + tag->artist() + tag->album() + tag->comment();
                const char* buf = metaData.toCString();
                size_t len = strlen( buf );
                int res = 0;
                chardet_t det = NULL;
                char encoding[CHARDET_MAX_ENCODING_NAME];
                chardet_create( &det );
                res = chardet_handle_data( det, buf, len );
                chardet_data_end( det );
                res = chardet_get_charset( det, encoding, CHARDET_MAX_ENCODING_NAME );
                chardet_destroy( det );

                QString track_encoding = encoding;

                if ( res == CHARDET_RESULT_OK )
                {
                    /*  for further information please refer to:
                     http://doc.trolltech.com/4.4/qtextcodec.html
                     http://www.mozilla.org/projects/intl/chardet.html
                     */
                    if ( ( track_encoding.toUtf8() == "gb18030" ) || ( track_encoding.toUtf8() == "big5" )
                        || ( track_encoding.toUtf8() == "euc-kr" ) || ( track_encoding.toUtf8() == "euc-jp" )
                        || ( track_encoding.toUtf8() == "koi8-r" ) )
                    {
                        QTextCodec *codec = QTextCodec::codecForName( track_encoding.toUtf8() );
                        QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
                        QTextCodec::setCodecForCStrings( utf8codec );
                        if ( codec != 0 )
                        {
                            attributes["title"] = codec->toUnicode( strip( tag->title() ).toLatin1() );
                            attributes["artist"] = codec->toUnicode( strip( tag->artist() ).toLatin1() );
                            attributes["album"] = codec->toUnicode( strip( tag->album() ).toLatin1() );
                            attributes["comment"] = codec->toUnicode( strip( tag->comment() ).toLatin1() );
                        }
                    }
                }
            }
            #undef strip
        }
        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
        {
            fileType = ogg;
            if ( file->tag() )
            {
                if ( !file->tag()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    attributes["composer"] = TStringToQString( file->tag()->fieldListMap()["COMPOSER"].front() ).trimmed();

                if ( !file->tag()->fieldListMap()[ "BPM" ].isEmpty() )
                    attributes["bpm"] = TStringToQString( file->tag()->fieldListMap()["BPM"].front() ).trimmed().toFloat();

                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).trimmed();

                if ( !file->tag()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->tag()->fieldListMap()["COMPILATION"].front() ).trimmed();
            }
        }
        else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            fileType = flac;
            if ( file->xiphComment() )
            {
                if ( !file->xiphComment()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    attributes["composer"] = TStringToQString( file->xiphComment()->fieldListMap()["COMPOSER"].front() ).trimmed();

                if ( !file->xiphComment()->fieldListMap()[ "BPM" ].isEmpty() )
                    attributes["bpm"] = TStringToQString( file->xiphComment()->fieldListMap()["BPM"].front() ).trimmed().toFloat();

                if ( !file->xiphComment()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->xiphComment()->fieldListMap()["DISCNUMBER"].front() ).trimmed();

                if ( !file->xiphComment()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->xiphComment()->fieldListMap()["COMPILATION"].front() ).trimmed();
            }
//             if ( images && file->ID3v2Tag() )
//                 loadImagesFromTag( *file->ID3v2Tag(), *images );
        }
        else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
        {
            fileType = mp4;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            if( mp4tag )
            {
                if ( mp4tag->itemListMap().contains( "\xA9wrt" ) )
                    attributes["composer"] = TStringToQString( mp4tag->itemListMap()["\xa9wrt"].toStringList().front() );

                if ( mp4tag->itemListMap().contains( "tmpo" ) )
                    attributes["bpm"] = QString::number( mp4tag->itemListMap()["tmpo"].toInt() );

                if ( mp4tag->itemListMap().contains( "disk" ) )
                    disc = QString::number( mp4tag->itemListMap()["disk"].toIntPair().first );

                if ( mp4tag->itemListMap().contains( "cpil" ) )
                    compilation = QString::number( mp4tag->itemListMap()["cpil"].toBool() ? '1' : '0' );

//                 if ( images && mp4tag->cover().size() )
//                     images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
            }
        }

        if ( !disc.isEmpty() )
        {
            int i = disc.indexOf('/');
            // guard against b0rked tags
            int discnumber;
            if ( i != -1 )
                // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
                discnumber = disc.left( i ).toInt();
            else
                discnumber = disc.toInt();
            attributes["discnumber"] = QString::number( discnumber );
        }

        // Sometimes the file is explicitly tagged with compilation details. When it is not,
        // then we need to delegate checking whether this files is in a compilation to Amarok.
        if ( compilation.isEmpty() )
        {
            // well, it wasn't set, but if the artist is VA assume it's a compilation
            //TODO: If we get pure-Qt translation support, put this back in; else functionality moved to the processor
            //if ( attributes["artist"] == QObject::tr( "Various Artists" ) )
            //    attributes["compilation"] = QString::number( 1 );
            attributes["compilation"] = "checkforvarious";
        }
        else
        {
            int i = compilation.toInt();
            attributes["compilation"] = QString::number( i );
        }
    }

    if ( !isValid )
    {
        std::cout << "<dud/>";
        return attributes;
    }

    if( m_batch && !m_rpath.isEmpty() )
    {
        QString rpath = path;
        rpath.remove( QDir::cleanPath( QDir::currentPath() ) );
        rpath.prepend( QDir::cleanPath( m_rpath + '/' ) );
        attributes["path"] = rpath;
    }
    else
        attributes["path"] = path;

    attributes["filetype"]  = QString::number( fileType );

    static const int Undetermined = -2;

    int bitrate = Undetermined;
    qint64 length = Undetermined;
    int samplerate = Undetermined;
    if( fileref.audioProperties() )
    {
        bitrate = fileref.audioProperties()->bitrate();
        length = fileref.audioProperties()->length() * 1000;
        samplerate = fileref.audioProperties()->sampleRate();
    }
    if ( bitrate == Undetermined || length == Undetermined || samplerate == Undetermined )
        attributes["audioproperties"] = "false";
    else
    {
        attributes["audioproperties"] = "true";
        attributes["bitrate"]         = QString::number( bitrate );
        attributes["length"]          = QString::number( length );
        attributes["samplerate"]      = QString::number( samplerate );
    }

    const int size = QFile( path ).size();
    if( size >= 0 )
        attributes["filesize"] =  QString::number( size );

    AFTUtility aftutil;
    attributes["uniqueid"] = QString( "amarok-sqltrackuid://" + aftutil.readUniqueId( path ) );

    return attributes;
}


void
CollectionScanner::writeElement( const QString &name, const AttributeHash &attributes )
{
    QDomDocument doc; // A dummy. We don't really use DOM, but SAX2
    QDomElement element = doc.createElement( name );

    QHashIterator<QString, QString> it( attributes );
    while( it.hasNext() )
    {
        it.next();
        // There are at least some characters that Qt cannot categorize which make the resulting
        // xml document ill-formed and prevent the parser from processing the remaining document.
        // Because of this we skip attributes containing characters not belonging to any category.
        const QString data = escape( it.value() );
        const unsigned len = data.length();
        bool nonPrint = false;
        for( unsigned i = 0; i < len; i++ )
        {
            if( !data[i].isPrint() || ( data[i].category() == QChar::NoCategory ) )
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

// taken verbatim from Qt's sources, since it's stupidly in the QtGui module
// the following function is subject to the following constraints:

// Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
// Contact: Qt Software Information (qt-info@nokia.com)

// Code is being used under terms of the LGPL version 2.1

QString
CollectionScanner::escape( const QString& plain )
{
    QString rich;
    rich.reserve(int(plain.length() * 1.1));
    for (int i = 0; i < plain.length(); ++i) {
        if (plain.at(i) == QLatin1Char('<'))
            rich += QLatin1String("&lt;");
        else if (plain.at(i) == QLatin1Char('>'))
            rich += QLatin1String("&gt;");
        else if (plain.at(i) == QLatin1Char('&'))
            rich += QLatin1String("&amp;");
        else
            rich += plain.at(i);
    }
    return rich;
}

void
CollectionScanner::readArgs()
{
    QStringList argslist = arguments();
    if( argslist.size() < 2 )
        displayHelp();
    bool longopt = false;
    bool nomore = false;
    int argnum = 0;
    bool rpatharg = false;
    bool pidarg = false;
    bool mtimearg = false;
    bool savelocationarg = false;
    bool collectionidarg = false;
    foreach( QString arg, argslist )
    {
        ++argnum;
        if( arg.isEmpty() || argnum == 1 )
            continue;
        if( nomore )
        {
            m_folders.append( arg );
        }
        else if( longopt || arg.startsWith( "--" ) )
        {
            if( longopt )
            {
                longopt = false;

                if( rpatharg )
                    m_rpath = arg;
                else if( pidarg )
                    m_amarokPid = arg;
                else if( savelocationarg )
                    m_saveLocation = arg;
                else if( collectionidarg )
                    m_collectionId = arg;
                else if( mtimearg )
                    m_mtimeFile = arg;
                else
                    displayHelp();

                rpatharg = false;
                pidarg = false;
                savelocationarg = false;
                collectionidarg = false;
                mtimearg = false;
            }
            else
            {
                QString myarg = arg.remove( 0, 2 );
                if( myarg == "rpath" )
                {
                    rpatharg = true;
                    longopt = true;
                }
                else if( myarg == "pid" )
                {
                    pidarg = true;
                    longopt = true;
                }
                else if( myarg == "savelocation" )
                {
                    savelocationarg = true;
                    longopt = true;
                }
                else if( myarg == "collectionid" )
                {
                    collectionidarg = true;
                    longopt = true;
                }
                else if( myarg == "mtime-file" )
                {
                    mtimearg = true;
                    longopt = true;
                }
                else if( myarg == "recursive" )
                    m_recursively = true;
                else if( myarg == "incremental" )
                    m_incremental = true;
                else if( myarg == "importplaylists" )
                    m_importPlaylists = true;
                else if( myarg == "restart" )
                    m_restart = true;
                else if( myarg == "batch" )
                    m_batch = true;
                else if( myarg == "charset" )
                    m_charset = false;
                else
                    displayHelp();
            }

        }
        else if( arg.startsWith( '-' ) )
        {
            QString myarg = arg.remove( 0, 1 );
            int pos = 0;
            while( pos < myarg.length() )
            {
                if( myarg[pos] == 'r' )
                    m_recursively = true;
                else if( myarg[pos] == 'i' )
                    m_incremental = true;
                else if( myarg[pos] == 'p' )
                    m_importPlaylists = true;
                else if( myarg[pos] == 's' )
                    m_restart = true;
                else if( myarg[pos] == 'b' )
                    m_batch = true;
                else if( myarg[pos] == 'c' )
                    m_charset = false;
                else
                    displayHelp();

                ++pos;
            }
        }
        else
        {
            nomore = true;
            m_folders.append( arg );
        }
    }
}

void
CollectionScanner::displayHelp()
{
    s_textStream << qPrintable( tr( "Amarok Collection Scanner" ) ) << endl << endl;
    s_textStream << qPrintable( tr( "Note: For debugging purposes this application can be invoked from the command line,\nbut it will not actually build a collection this way without the Amarok player." ) ) << endl << endl;
    s_textStream << qPrintable( tr( "IRC:\nserver: irc.freenode.net / channels: #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org" ) ) << endl << endl;
    s_textStream << qPrintable( tr( "Usage: amarokcollectionscanner [options] <Folder(s)>" ) ) << endl << endl;
    s_textStream << qPrintable( tr( "User-modifiable Options:" ) ) << endl;
    s_textStream << qPrintable( tr( "Folder(s)             : Space-separated list of folders to scan; when using -b and -i, the path to the file generated by Amarok containing the list of folders" ) ) << endl;
    s_textStream << qPrintable( tr( "-h, --help            : This help text" ) ) << endl;
    s_textStream << qPrintable( tr( "-r, --recursive       : Scan folders recursively" ) ) << endl;
    s_textStream << qPrintable( tr( "-i, --incremental     : Incremental scan (modified folders only)" ) ) << endl;
    s_textStream << qPrintable( tr( "-p, --importplaylists : Import playlists" ) ) << endl;
    s_textStream << qPrintable( tr( "-s, --restart         : After a crash, restart the scanner in its last position" ) ) << endl;
    s_textStream << qPrintable( tr( "-b, --batch           : Run in batch mode" ) ) << endl;
    s_textStream << qPrintable( tr( "-c, --nocharset       : Do not run the charset detector" ) ) << endl;
    s_textStream << qPrintable( tr( "--rpath=\"<path>\"      : In full-scan batch mode, specifies a path to prepend to entries (default is the current directory)" ) ) << endl;
    s_textStream.flush();
    ::exit(0);
}

#include "CollectionScanner.moc"

