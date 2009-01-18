/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <hydrogen@notyetimplemented.com        *
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

#include "Amarok.h"
#include "charset-detector/include/chardet.h"

#include <cerrno>
#include <iostream>
#include <limits.h>    //PATH_MAX
#include <cmath>

#include <QTextDocument> //Qt::escape
#include <QByteArray>
#include <QDBusReply>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>

#include <KLocale>
#include <KMD5>

//Taglib:
#include <apetag.h>
#include <fileref.h>
#include <flacfile.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
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


CollectionScanner::CollectionScanner( const QStringList& folders,
                                      const QString& amarokPid,
                                      const QString& collectionId,
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
        , m_logfile( saveLocation() + "collection_scan.log"  )
{
    kapp->setObjectName( "amarokcollectionscanner" );
    if( !restart )
        QFile::remove( m_logfile );

    if( !collectionId.isEmpty() )
    {
        if( amarokPid.isEmpty() )
            m_amarokCollectionInterface = new QDBusInterface( "org.kde.amarok", "/SqlCollection/" + collectionId );
        else
            m_amarokCollectionInterface = new QDBusInterface( "org.kde.amarok-" + amarokPid, "/SqlCollection/" + collectionId );
    }
    else
        m_amarokCollectionInterface = 0;

    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}


CollectionScanner::~CollectionScanner()
{
    delete m_amarokCollectionInterface;
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

        QFile folderFile( saveLocation()  + "collection_scan.files"   );
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

        QFile folderFile( saveLocation() + "collection_scan.files" );
        folderFile.open( QIODevice::WriteOnly );
        QTextStream stream( &folderFile );
        stream.setCodec( QTextCodec::codecForName("UTF-8") );
        stream << entries.join( "\n" );
        folderFile.close();
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
    AttributeHash attributes;
    attributes["path"] = dir;
    writeElement( "folder", attributes );
    d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Readable );
    QFileInfoList list = d.entryInfoList();
    foreach( QFileInfo f, list )
    {
        if( !f.exists() )
            break;

        if( f.isSymLink() )
            f = QFileInfo( f.symLinkTarget() );

        if( f.isDir() && m_recursively && !m_scannedFolders.contains( f.canonicalFilePath() ) )
        {
            bool isInCollection = false;
            if( m_incremental && m_amarokCollectionInterface )
            {
                QDBusReply<bool> reply = m_amarokCollectionInterface->call( "isDirInCollection", f.canonicalFilePath() );
                if( reply.isValid() )
                    isInCollection = reply.value();
            }

            if( !m_incremental || !isInCollection )
                readDir( f.absoluteFilePath() + '/', entries );
        }
        else if( f.isFile() )
            entries.append( f.absoluteFilePath() );
    }
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
//                     attributes["path"] = path;
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

                for( QList<CoverBundle>::ConstIterator it2 = covers.begin(); it2 != covers.end(); ++it2 )
                {
                    string += ( string.isEmpty() ? "" : "AMAROK_MAGIC" ) + (*it2).first + "AMAROK_MAGIC" + (*it2).second;
                }

                AttributeHash attributes;
                attributes["path"] = imagePath;
                attributes["list"] = string;
                writeElement( "image", attributes );
            }

            AttributeHash attributes;
            attributes["path"] = dir;
            writeElement( "compilation", attributes );

            // clear now because we've processed them
            covers.clear();
            images.clear();
        }
    }
}

const QString
CollectionScanner::readEmbeddedUniqueId( const TagLib::FileRef &fileref )
{
    int currentVersion = 1; //TODO: Make this more global?
    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if( !file->ID3v2Tag( false ) )
            return QString();
        QString ourId = QString( "Amarok 2 AFTv" + QString::number( currentVersion ) + " - amarok.kde.org" );
        if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
            return QString();
        TagLib::ID3v2::FrameList frameList = file->ID3v2Tag()->frameListMap()["UFID"];
        TagLib::ID3v2::FrameList::Iterator iter;
        for( iter = frameList.begin(); iter != frameList.end(); ++iter )
        {
            TagLib::ID3v2::UniqueFileIdentifierFrame* currFrame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*iter);
            if( currFrame )
            {
                QString owner = TStringToQString( currFrame->owner() );
                if( owner.startsWith( "Amarok 2 AFT" ) )
                {
                    int version = owner.at( 13 ).digitValue();
                    if( version == currentVersion )
                        return TStringToQString( TagLib::String( currFrame->identifier() ) );
                }
            }
        }
    }
    return QString();
}

const TagLib::ByteVector
CollectionScanner::generatedUniqueIdHelper( const TagLib::FileRef &fileref )
{
    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if( file->ID3v2Tag() )
            return file->ID3v2Tag()->render();
        else if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->APETag() )
            return file->APETag()->render();
    }
    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        if( file->ID3v2Tag() )
            return file->ID3v2Tag()->render();
        else if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->xiphComment() )
            return file->xiphComment()->render();
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( fileref.file() ) )
    {
        if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->APETag() )
            return file->APETag()->render();
    }
    TagLib::ByteVector bv;
    return bv;
}

const QString
CollectionScanner::readUniqueId( const QString &path )
{
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(path.utf16());
#else
    QByteArray fileName = QFile::encodeName( path );
    const char * encodedName = fileName.constData(); // valid as long as fileName exists
#endif

    TagLib::FileRef fileref = TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );

    if( fileref.isNull() )
        return QString();

    const QString embeddedString = readEmbeddedUniqueId( fileref );
    if( !embeddedString.isEmpty() )
        return embeddedString;

    TagLib::ByteVector bv = CollectionScanner::generatedUniqueIdHelper( fileref );

    KMD5 md5( bv.data(), bv.size() );

    QFile qfile( path );

    char databuf[16384];
    int readlen = 0;
    QByteArray size;
    QString returnval;

    if( qfile.open( QIODevice::ReadOnly ) )
    {
        if( ( readlen = qfile.read( databuf, 16384 ) ) > 0 )
        {
            md5.update( databuf, readlen );
            md5.update( size.setNum( qfile.size() ) );
            qfile.close();
            return QString( md5.hexDigest().data() );
        }
        else
        {
            qfile.close();
            return QString();
        }
    }

    return QString();
}

qreal
CollectionScanner::peakToDecibels( qreal scaleVal )
{
    if ( scaleVal > 0 )
        return 20 * log10( scaleVal );
    else
        return 0;
}

// NOTE: representation is taken to be a binary value with units in the first column,
//       1/2 in the second and so on.
qreal
CollectionScanner::readRVA2PeakValue( const TagLib::ByteVector &data, int bits, bool *ok )
{
    qreal peak = 0.0;
    // discarding digits at the end reduces precision, but doesn't otherwise change the value
    if ( bits > 32 )
        bits = 32;
    // the +7 makes sure we round up when we divide by 8
    unsigned int bytes = (bits + 7) / 8;
    // fewer than 4 bits would just be daft
    if ( bits >= 4 && data.size() >= bytes )
    {
        // excessBits is the number of bits we have to discard at the end
        unsigned int excessBits = (8 * bytes) - bits;
        // mask has 1s everywhere but the last /excessBits/ bits
        quint32 mask = 0xffffffff << excessBits;
        quint32 rawValue = 0;
        for ( unsigned int i = 0; i < bytes; ++i )
        {
            rawValue <<= 8;
            rawValue += (unsigned char)data[i];
        }
        rawValue &= mask;
        peak = rawValue;
        // amount we need to "shift" the value right to make the first digit the unit column
        unsigned int rightShift = (8 * bytes) - 1;
        peak /= (qreal)(1 << rightShift);
        if ( ok )
            *ok = true;
    }
    else
        if ( ok )
            *ok = false;
    return peak;
}

void
CollectionScanner::addPeakFromScale( const TagLib::String &scaleVal, const QString &key, AttributeHash *attributes )
{
    // scale value is >= 0, and typically not much bigger than 1
    QString value = TStringToQString( scaleVal );
    bool ok = false;
    qreal peak = value.toFloat( &ok );
    if ( ok && peak >= 0 )
        (*attributes)[key] = QString::number( peakToDecibels( peak ) );
}

void
CollectionScanner::addGain( const TagLib::String &input, const QString &key, AttributeHash *attributes )
{
    QString value = TStringToQString( input ).remove( " dB" );
    bool ok = false;
    qreal gain = value.toFloat( &ok );
    if (ok)
        (*attributes)[key] = QString::number( gain );
}

void
CollectionScanner::readMP3ReplayGainTags( TagLib::MPEG::File *file, AttributeHash *attributes )
{
    if ( file->ID3v2Tag() )
    {
        {   // ID3v2.4.0 native replay gain tag support (as written by Quod Libet, for example).
            TagLib::ID3v2::FrameList frames = file->ID3v2Tag()->frameListMap()["RVA2"];
            frames.append(file->ID3v2Tag()->frameListMap()["XRVA"]);
            if ( !frames.isEmpty() )
            {
                for ( unsigned int i = 0; i < frames.size(); ++i )
                {
                    // we have to parse this frame ourselves
                    // ID3v2 frame header is 10 bytes, so skip that
                    TagLib::ByteVector data = frames[i]->render().mid( 10 );
                    unsigned int offset = 0;
                    QString desc( data.data() );
                    offset += desc.count() + 1;
                    unsigned int channel = data.mid( offset, 1 ).toUInt( true );
                    // channel 1 is the main volume - the only one we care about
                    if ( channel == 1 )
                    {
                        ++offset;
                        qint16 adjustment512 = data.mid( offset, 2 ).toShort( true );
                        qreal adjustment = ( (qreal)adjustment512 ) / 512.0;
                        offset += 2;
                        unsigned int peakBits = data.mid( offset, 1 ).toUInt( true );
                        ++offset;
                        bool ok = false;
                        qreal peak = readRVA2PeakValue( data.mid( offset ), peakBits, &ok );
                        if ( ok )
                        {
                            if ( desc.toLower() == "album" )
                            {
                                (*attributes)["albumgain"] = QString::number( adjustment );
                                (*attributes)["albumpeakgain"] = QString::number( peakToDecibels( peak ) );
                            }
                            else if ( desc.toLower() == "track" || !attributes->contains( "trackgain" ) )
                            {
                                (*attributes)["trackgain"] = QString::number( adjustment );
                                (*attributes)["trackpeakgain"] = QString::number( peakToDecibels( peak ) );
                            }
                        }
                    }
                }
                if ( attributes->contains( "trackgain" ) || attributes->contains( "albumgain" ) )
                    return;
            }
        }

        {   // Foobar2000-style ID3v2.3.0 tags
            TagLib::ID3v2::FrameList frames = file->ID3v2Tag()->frameListMap()["TXXX"];
            for ( TagLib::ID3v2::FrameList::Iterator it = frames.begin(); it != frames.end(); ++it ) {
                TagLib::ID3v2::UserTextIdentificationFrame* frame =
                    dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>( *it );
                if ( frame && frame->fieldList().size() >= 2 )
                {
                    QString desc = TStringToQString( frame->description() ).toLower();
                    if ( desc == "replaygain_track_gain" )
                        addGain( frame->fieldList()[1], "trackgain", attributes );
                    if ( desc == "replaygain_track_peak" )
                        addPeakFromScale( frame->fieldList()[1], "trackpeakgain", attributes );
                    if ( desc == "replaygain_album_gain" )
                        addGain( frame->fieldList()[1], "albumgain", attributes );
                    if ( desc == "replaygain_album_peak" )
                        addPeakFromScale( frame->fieldList()[1], "albumpeakgain", attributes );
                }
            }
            if ( attributes->contains( "trackgain" ) || attributes->contains( "albumgain" ) )
                return;
        }
    }
    if ( file->APETag() )
    {
        const TagLib::APE::ItemListMap &items = file->APETag()->itemListMap();
        if ( items.contains("REPLAYGAIN_TRACK_GAIN") )
        {
            addGain( items["REPLAYGAIN_TRACK_GAIN"].values()[0], "trackgain", attributes );
            if ( items.contains("REPLAYGAIN_TRACK_PEAK") )
                addPeakFromScale( items["REPLAYGAIN_TRACK_PEAK"].values()[0], "trackpeakgain", attributes );
        }
        if ( items.contains("REPLAYGAIN_ALBUM_GAIN") )
        {
            addGain( items["REPLAYGAIN_ALBUM_GAIN"].values()[0], "albumgain", attributes );
            if ( items.contains("REPLAYGAIN_ALBUM_PEAK") )
                addPeakFromScale( items["REPLAYGAIN_ALBUM_PEAK"].values()[0], "albumpeakgain", attributes );
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

        QString disc;
        QString compilation;

        //FIXME: get replaygain info for other formats we can read

        /* As mpeg implementation on TagLib uses a Tag class that's not defined on the headers,
           we have to cast the files, not the tags! */
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            fileType = mp3;

            readMP3ReplayGainTags( file, &attributes );

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
            else if ( file->ID3v1Tag() )
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
                //http://doc.trolltech.com/4.4/qtextcodec.html
                //http://www.mozilla.org/projects/intl/chardet.html
                    if ( track_encoding == "x-euc-tw" ) track_encoding = ""; //no match
                    if ( track_encoding == "HZ-GB2312" ) track_encoding = ""; //no match
                    if ( track_encoding == "ISO-2022-CN" ) track_encoding = ""; //no match
                    if ( track_encoding == "ISO-2022-KR" ) track_encoding = ""; //no match
                    if ( track_encoding == "ISO-2022-JP" ) track_encoding = ""; //no match
                    if ( track_encoding == "x-mac-cyrillic" ) track_encoding = ""; //no match
                    if ( track_encoding == "IBM855" ) track_encoding =""; //no match
                    if ( track_encoding == "IBM866" ) track_encoding = "IBM 866";
                    if ( track_encoding == "TIS-620" ) track_encoding = ""; //ISO-8859-11, no match
                    if ( !track_encoding.isEmpty() )
                    {
                    //FIXME:about 10% tracks cannot be decoded well. It shows blank for now.
                        //debug () << "Final Codec Name:" << track_encoding.toUtf8() <<endl;
                        QTextCodec *codec = QTextCodec::codecForName( track_encoding.toUtf8() );
                        QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
                        QTextCodec::setCodecForCStrings( utf8codec );
                        attributes["title"] = codec->toUnicode( strip( tag->title() ).toLatin1() );
                        attributes["artist"] = codec->toUnicode( strip( tag->artist() ).toLatin1() );
                        attributes["album"] = codec->toUnicode( strip( tag->album() ).toLatin1() );
                        attributes["comment"] = codec->toUnicode( strip( tag->comment() ).toLatin1() );
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

                if ( !file->tag()->fieldListMap()[ "REPLAYGAIN_TRACK_GAIN" ].isEmpty() )
                {
                    addGain( file->tag()->fieldListMap()["REPLAYGAIN_TRACK_GAIN"].front(), "trackgain", &attributes );
                    if ( !file->tag()->fieldListMap()[ "REPLAYGAIN_TRACK_PEAK" ].isEmpty() )
                        addPeakFromScale( file->tag()->fieldListMap()["REPLAYGAIN_TRACK_PEAK"].front(), "trackpeakgain", &attributes );
                }

                if ( !file->tag()->fieldListMap()[ "REPLAYGAIN_ALBUM_GAIN" ].isEmpty() )
                {
                    addGain( file->tag()->fieldListMap()["REPLAYGAIN_ALBUM_GAIN"].front(), "albumgain", &attributes );
                    if ( !file->tag()->fieldListMap()[ "REPLAYGAIN_ALBUM_PEAK" ].isEmpty() )
                        addPeakFromScale( file->tag()->fieldListMap()["REPLAYGAIN_ALBUM_PEAK"].front(), "albumpeakgain", &attributes );
                }
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

                if ( !file->xiphComment()->fieldListMap()[ "REPLAYGAIN_TRACK_GAIN" ].isEmpty() )
                {
                    addGain( file->xiphComment()->fieldListMap()["REPLAYGAIN_TRACK_GAIN"].front(), "trackgain", &attributes );
                    if ( !file->xiphComment()->fieldListMap()[ "REPLAYGAIN_TRACK_PEAK" ].isEmpty() )
                        addPeakFromScale( file->xiphComment()->fieldListMap()["REPLAYGAIN_TRACK_PEAK"].front(), "trackpeakgain", &attributes );
                }

                if ( !file->xiphComment()->fieldListMap()[ "REPLAYGAIN_ALBUM_GAIN" ].isEmpty() )
                {
                    addGain( file->xiphComment()->fieldListMap()["REPLAYGAIN_ALBUM_GAIN"].front(), "albumgain", &attributes );
                    if ( !file->xiphComment()->fieldListMap()[ "REPLAYGAIN_ALBUM_PEAK" ].isEmpty() )
                        addPeakFromScale( file->xiphComment()->fieldListMap()["REPLAYGAIN_ALBUM_PEAK"].front(), "albumpeakgain", &attributes );
                }
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
                attributes["composer"] = TStringToQString( mp4tag->composer() );
                attributes["bpm"] = QString::number( mp4tag->bpm() ).toFloat();
                disc = QString::number( mp4tag->disk() );
                compilation = QString::number( mp4tag->compilation() );
                // FIXME: get replaygain info for MP4 files

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

        if ( compilation.isEmpty() )
        {
            // well, it wasn't set, but if the artist is VA assume it's a compilation
            if ( attributes["artist"] == i18n( "Various Artists" ) )
                attributes["compilation"] = QString::number( 1 );
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

    attributes["path"]      = path;
    attributes["filetype"]  = QString::number( fileType );
    const int bitrate = fileref.audioProperties()->bitrate();
    const int length = fileref.audioProperties()->length();
    const int samplerate = fileref.audioProperties()->sampleRate();
    static const int Undetermined = -2;
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

    attributes["uniqueid"] = QString( "amarok-sqltrackuid://" + readUniqueId( path ) );

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
        const QString data = Qt::escape( it.value() );
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

#include "CollectionScanner.moc"

