/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
 *             (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
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

#include "MetaTagLib.h"
#include "FileType.h"
#include "MetaReplayGain.h"

#ifndef UTILITIES_BUILD
#include "amarokconfig.h"
#endif

#include <QImage>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>
#include <QDebug>

#include <KEncodingProber>

//Taglib:
#include <audioproperties.h>
#include <fileref.h>

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

#include <popularimeterframe.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <attachedpictureframe.h>
#include <commentsframe.h>
#include <xiphcomment.h>

namespace Meta
{
    namespace Tag
    {
        enum FileTypes
        {
            FLAC,
            MPEG,
            MP4,
            MPC,
            OGG,
            SPEEX
        };

        QMutex s_mutex;

        static TagLib::FileRef getFileRef( const QString &path );

        static void addRandomness( QCryptographicHash *md5 );

        /** Returns a byte vector that can be used to generate the unique id based on the tags. */
        static TagLib::ByteVector generatedUniqueIdHelper( const TagLib::FileRef &fileref );
        static QString generateUniqueId( const QString &path, const TagLib::FileRef &fileref );

        bool isValidMusicBrainzId( const QString &id );

        /** Splits a string with two numers and returns the first.
            e.g. "1/11" is split and 1 returned.
            returns -1 if the number could not be parsed
        */
        static int splitNumber( const QString str );

        static Meta::FieldHash decodeMpeg( TagLib::MPEG::File *file );
        static Meta::FieldHash decodeXiph( const QString &name, const QString &value );

        /** Decodes FMPS tags.
            http://gitorious.org/~jefferai/xdg-specs/jefferais-xdg-specs/blobs/d3fef64aa1e2e0528b991b0313fd5b68d78761bd/specifications/FMPSpecs/specification.txt
        */
        Meta::FieldHash decodeFMPS( const QString &identifier, const QString &value, bool camelCase );

        const char* fieldName( qint64 field, FileTypes type );
        qint64 field( const QString &str, FileTypes type );

    }
}

#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

TagLib::FileRef
Meta::Tag::getFileRef( const QString &path )
{
#ifdef Q_OS_WIN32
    const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(path.utf16());
#else
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(path.utf16());
#else
    QByteArray fileName = QFile::encodeName( path );
    const char *encodedName = fileName.constData(); // valid as long as fileName exists
#endif
#endif

    // Tests reveal the following:
    //
    // TagLib::AudioProperties   Relative Time Taken
    //
    //  No AudioProp Reading        1
    //  Fast                        1.18
    //  Average                     Untested
    //  Accurate                    Untested

    return TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );
}


// ----------------------- unique id ------------------------


void
Meta::Tag::addRandomness( QCryptographicHash *md5 )
{
    //md5 has size of file already added for some little extra randomness for the hash
    qsrand( QTime::currentTime().msec() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
    md5->addData( QString::number( qrand() ).toAscii() );
}

TagLib::ByteVector
Meta::Tag::generatedUniqueIdHelper( const TagLib::FileRef &fileref )
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
    else if ( TagLib::Ogg::Speex::File *file = dynamic_cast<TagLib::Ogg::Speex::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        if( file->xiphComment() )
            return file->xiphComment()->render();
        else if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->ID3v2Tag() )
            return file->ID3v2Tag()->render();
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

QString
Meta::Tag::generateUniqueId( const QString &path, const TagLib::FileRef &fileref )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QFile qfile( path );
    QByteArray size;
    md5.addData( size.setNum( qfile.size() ) );

    TagLib::ByteVector bv = generatedUniqueIdHelper( fileref );
    md5.addData( bv.data(), bv.size() );

    char databuf[16384];
    int readlen = 0;
    QString returnval;

    if( qfile.open( QIODevice::ReadOnly ) )
    {
        if( ( readlen = qfile.read( databuf, 16384 ) ) > 0 )
        {
            md5.addData( databuf, readlen );
            qfile.close();
        }
        else
        {
            qfile.close();
            addRandomness( &md5 );
        }
    }
    else
        addRandomness( &md5 );

    return QString( md5.result().toHex() );
}

bool
Meta::Tag::isValidMusicBrainzId( const QString &id )
{
    if( id.isEmpty() || id == QLatin1String( "[mb track uuid]" ) )
        return false;

    QRegExp mbIdPattern( "[-a-f0-9]+", Qt::CaseInsensitive );
    return mbIdPattern.exactMatch( id );
}

// ----------------------- reading ------------------------

Meta::FieldHash
Meta::Tag::readTags( const QString &path, bool useCharsetDetector )
{
    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    Meta::FieldHash result;

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return result;

    TagLib::Tag *tag = fileref.tag();
    if( !tag )
        return result;

#define strip( x ) TStringToQString( x ).trimmed()
    result.insert( Meta::valTitle,   strip( tag->title() ) );
    result.insert( Meta::valArtist,  strip( tag->artist() ) );
    result.insert( Meta::valAlbum,   strip( tag->album() ) );
    result.insert( Meta::valComment, strip( tag->comment() ) );
    result.insert( Meta::valGenre,   strip( tag->genre() ) );
    if( tag->year() && tag->year() < 2200 )
        result.insert( Meta::valYear, tag->year() );
    if( tag->track() )
        result.insert( Meta::valTrackNr, tag->track() );

    Meta::ReplayGainTagMap replayGainTags = Meta::readReplayGainTags( fileref );
    if( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        result.insert( Meta::valTrackGain, replayGainTags[Meta::ReplayGain_Track_Gain] );
    if( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
        result.insert( Meta::valTrackGainPeak, replayGainTags[Meta::ReplayGain_Track_Peak] );

    // strangely: the album gain defaults to the track gain
    if( replayGainTags.contains( Meta::ReplayGain_Album_Gain ) )
        result.insert( Meta::valAlbumGain, replayGainTags[Meta::ReplayGain_Album_Gain] );
    else if( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        result.insert( Meta::valAlbumGain, replayGainTags[Meta::ReplayGain_Track_Gain] );
    if( replayGainTags.contains( Meta::ReplayGain_Album_Peak ) )
        result.insert( Meta::valAlbumGainPeak, replayGainTags[Meta::ReplayGain_Album_Peak] );
    else if( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
        result.insert( Meta::valAlbumGainPeak, replayGainTags[Meta::ReplayGain_Track_Peak] );



    /* As MPEG implementation on TagLib uses a Tag class that's not defined on the headers,
       we have to cast the files, not the tags! */
    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        result.insert( Meta::valFiletype, Amarok::Mp3 );
        result.unite( decodeMpeg( file ) );

        // HACK: charset-detector disabled, so all tags assumed utf-8
        // TODO: fix charset-detector to detect encoding with higher accuracy
        if( useCharsetDetector && tag )
        {
            TagLib::String metaData = tag->title() + tag->artist() + tag->album() + tag->comment();
            const char* buf = metaData.toCString();
            size_t len = strlen( buf );
            KEncodingProber prober;
            KEncodingProber::ProberState proberResult = prober.feed( buf, len );
            QString track_encoding( prober.encoding() );
            if( proberResult != KEncodingProber::NotMe )
            {
                /*  for further information please refer to:
                    http://doc.trolltech.com/4.4/qtextcodec.html
                    http://www.mozilla.org/projects/intl/chardet.html
                */
                if ( ( track_encoding.toUtf8() == "gb18030" )
                     || ( track_encoding.toUtf8() == "big5" )
                     || ( track_encoding.toUtf8() == "euc-kr" )
                     || ( track_encoding.toUtf8() == "euc-jp" )
                     || ( track_encoding.toUtf8() == "koi8-r" ) )
                {
                    QTextCodec *codec = QTextCodec::codecForName( track_encoding.toUtf8() );
                    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
                    QTextCodec::setCodecForCStrings( utf8codec );
                    if ( codec != 0 )
                    {
                        result.insert( Meta::valTitle, codec->toUnicode( strip( tag->title() ).toLatin1() ) );
                        result.insert( Meta::valArtist, codec->toUnicode( strip( tag->artist() ).toLatin1() ) );
                        result.insert( Meta::valAlbum, codec->toUnicode( strip( tag->album() ).toLatin1() ) );
                        result.insert( Meta::valComment, codec->toUnicode( strip( tag->comment() ).toLatin1() ) );
                    }
                }
            }
        }
    }

    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        result.insert( Meta::valFiletype, Amarok::Ogg );
        if ( file->tag() )
        {
            for( TagLib::Ogg::FieldListMap::ConstIterator it = file->tag()->fieldListMap().begin(); it != file->tag()->fieldListMap().end(); ++it )
            {
                QString name = TStringToQString( it->first );
                QString value = TStringToQString( it->second.toString("\n") );
                result.unite( decodeXiph( name, value ) );
            }
        }
    }

    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        result.insert( Meta::valFiletype, Amarok::Flac );
        if ( file->xiphComment() )
        {
            for( TagLib::Ogg::FieldListMap::ConstIterator it = file->xiphComment()->fieldListMap().begin(); it != file->xiphComment()->fieldListMap().end(); ++it )
            {
                QString name = TStringToQString( it->first );
                QString value = TStringToQString( it->second.toString("\n") );
                result.unite( decodeXiph( name, value ) );
            }
        }
    }

    else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        result.insert( Meta::valFiletype, Amarok::Mp4 );
        TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
        if( mp4tag )
        {
            for( TagLib::MP4::ItemListMap::Iterator it = mp4tag->itemListMap().begin(); it != mp4tag->itemListMap().end(); ++it )
            {
                QString name = TStringToQString( it->first );
                QString value = TStringToQString( it->second.toStringList().toString("\n") );

                if( name == QLatin1String( "\xA9wrt" ) )
                    result.insert( Meta::valComposer, value );

                else if( name == QLatin1String( "aART" ) ) // iTunes 4.0
                    result.insert( Meta::valAlbumArtist, value );

                else if( name == QLatin1String( "tmpo" ) )
                    result.insert( Meta::valBpm, it->second.toInt() );

                else if( name == QLatin1String( "disk" ) )
                    result.insert( Meta::valDiscNr, it->second.toIntPair().first );

                else if( name == QLatin1String( "cpil" ) )
                {
                    if( it->second.toBool() )
                        result.insert( Meta::valCompilation, true );
                    else
                        result.insert( Meta::valCompilation, false );
                }

                else
                {
                    if( name.startsWith("----:com.apple.iTunes") )
                        result.unite( decodeFMPS( name.remove(0, 22), value, true ) );
                }

                //                 if ( images && mp4tag->cover().size() )
                //                     images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
            }
        }
    }
    //we didn't set a FileType till now, let's look it up via FileExtension
    if( !result.contains( Meta::valFiletype ) )
    {
        QString ext = path.mid( path.lastIndexOf( '.' ) + 1 );
        result.insert( Meta::valFiletype, Amarok::FileTypeSupport::fileType( ext ) );
    }

    if( fileref.audioProperties() )
    {
        result.insert( Meta::valBitrate, fileref.audioProperties()->bitrate() );
        result.insert( Meta::valLength, fileref.audioProperties()->length() * 1000 );
        result.insert( Meta::valSamplerate, fileref.audioProperties()->sampleRate() );
    }

    QFileInfo fileInfo( path );
    result.insert( Meta::valFilesize, fileInfo.size() );
    result.insert( Meta::valModified, fileInfo.lastModified() );

    if( !result.contains( Meta::valUniqueId ) )
        result.insert( Meta::valUniqueId, generateUniqueId( path, fileref ) );

    // TODO:
    // --- if no tags could be found. Invent something from the filename
    // replace underscores with spaces
    // capitalize each word if all characters are lower case
    // - split up the filename at dashes
    // if the filename starts with a number that is probably a track number
    // - two parts mean "artist - title"

#undef strip

    return result;
}

Meta::FieldHash
Meta::Tag::decodeMpeg( TagLib::MPEG::File *file )
{
    Meta::FieldHash result;

    if( !file->ID3v2Tag() )
        return result;

    for( TagLib::ID3v2::FrameList::ConstIterator it = file->ID3v2Tag()->frameList().begin(); it != file->ID3v2Tag()->frameList().end(); ++it )
    {
        TagLib::String name  = TagLib::String((*it)->frameID());

        // -- comments
        if( name == "TXXX" )
        {
            TagLib::ID3v2::UserTextIdentificationFrame* frame = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);
            if( !frame )
                continue;

            // the value of the user text frame is stored in the
            // second and following fields.
            TagLib::StringList fields = frame->fieldList();
            if( fields.size() >= 2 )
            {
                result.unite( decodeFMPS( TStringToQString( fields[0] ),
                                          TStringToQString( fields[1] ), true ) );
            }
        }

        else if( name[0] == 'T' )
        {
            TagLib::ID3v2::TextIdentificationFrame* frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(*it);
            if( !frame )
                continue;

            QString value = TStringToQString(frame->fieldList().toString("\n"));

            if( name == "TPOS" )
                result.insert( Meta::valDiscNr, splitNumber(value) );

            else if( name == "TBPM" )
                result.insert( Meta::valBpm, value.toFloat() );

            else if( name == "TCOM" )
                result.insert( Meta::valComposer, value );

            else if( name == "TPE2" )
                result.insert( Meta::valAlbumArtist, value );

            // -- compilation
            else if( name == "TCMP" )
            {
                if( value.toLower() == QLatin1String( "true" ) || value.toInt() )
                    result.insert( Meta::valCompilation, true );
                else
                    result.insert( Meta::valCompilation, false );
            }
        }

        // -- rating, playcount
        else if( name == "POPM" )
        {
            TagLib::ID3v2::PopularimeterFrame *frame = dynamic_cast<TagLib::ID3v2::PopularimeterFrame *>(*it);
            if( !frame )
                continue;

            if( TStringToQString(frame->email()).isEmpty() ) // only read anonymous ratings
            {
                // FMPS tags have precedence
                if( !result.contains( Meta::valRating ) && frame->rating() != 0 )
                    result.insert( Meta::valRating, qreal(frame->rating()) / 256.0 );
                if( !result.contains( Meta::valPlaycount ) && frame->counter() < 10000 )
                    result.insert( Meta::valPlaycount, frame->counter() );
            }
        }

        // -- covers
        else if( name == "APIC" )
        {
            TagLib::ID3v2::AttachedPictureFrame* frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
            if( !frame )
                continue;

            if( (frame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover ||
                 frame->type() == TagLib::ID3v2::AttachedPictureFrame::Other) &&
                frame->picture().size() > 1024 ) // must be at least 1kb
            {
                result.insert( Meta::valHasCover, true );
            }
        }

        // -- uid
        else if( name == "UFID" )
        {
            TagLib::ID3v2::UniqueFileIdentifierFrame* frame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*it);
            if( !frame )
                continue;

            QString owner = TStringToQString( frame->owner() );
            QString identifier = TStringToQString( TagLib::String( frame->identifier() ) );
            if( identifier.isEmpty() )
                continue;

            if( owner == QLatin1String( "http://musicbrainz.org" ) )
            {
                if( isValidMusicBrainzId( identifier ) &&
                    !result.contains( Meta::valUniqueId ) ) // AFT id is prefered
                    result.insert( Meta::valUniqueId, identifier.prepend( "mb-" ) );
            }
            else if( owner == QLatin1String( "Amarok 2 AFTv1 - amarok.kde.org" ) )
            {
                if( !identifier.isEmpty() )
                    result.insert( Meta::valUniqueId, identifier );
            }
            // don't import any other uids. we can't rely on them.
        }
    }

    return result;
}

Meta::FieldHash
Meta::Tag::decodeXiph( const QString &name, const QString &value )
{
    Meta::FieldHash result;
    if( name.compare( QLatin1String( "COMPOSER" ), Qt::CaseInsensitive ) == 0 )
        result.insert( Meta::valComposer, value );

    else if( name.compare( QLatin1String( "ALBUMARTIST" ), Qt::CaseInsensitive ) == 0 )
        result.insert( Meta::valAlbumArtist, value );

    else if( name.compare( QLatin1String( "BPM" ), Qt::CaseInsensitive ) == 0 )
        result.insert( Meta::valBpm, value.toFloat() );

    else if( name.compare( QLatin1String( "DISCNUMBER" ), Qt::CaseInsensitive ) == 0 )
        result.insert( Meta::valDiscNr, splitNumber(value.trimmed()) );

    else if( name.compare( QLatin1String( "COMPILATION" ), Qt::CaseInsensitive ) == 0 )
    {
        if( value.toInt() )
            result.insert( Meta::valCompilation, true );
        else
            result.insert( Meta::valCompilation, false );
    }
    else
        result.unite( decodeFMPS( name, value, false ) );

    return result;
}

Meta::FieldHash
Meta::Tag::decodeFMPS( const QString &identifier, const QString &value, bool camelCase )
{
    Meta::FieldHash result;
    if( value.isEmpty() )
        return result;

    bool ok = false;
    qreal f = value.toFloat( &ok );

    if( (camelCase  && identifier == QLatin1String( "FMPS_Rating" )) ||
        (!camelCase && identifier == QLatin1String( "FMPS_RATING" )) )
    {
        if( ok )
            result.insert( Meta::valRating, f );
    }

    else if( (camelCase  && identifier == QLatin1String( "FMPS_Rating_Amarok_Score" )) ||
             (!camelCase && identifier == QLatin1String( "FMPS_RATING_AMAROK_SCORE" )) )
    {
        if( ok )
            result.insert( Meta::valScore, f );
    }

    else if( (camelCase  && identifier == QLatin1String( "FMPS_Playcount" )) ||
             (!camelCase && identifier == QLatin1String( "FMPS_PLAYCOUNT" )) )
    {
        if( ok )
            result.insert( Meta::valPlaycount, f );
    }

    // -- not actually FMPS but following the same logic
    else if( (camelCase  && identifier == QLatin1String( "MusicBrainz Track Id" )) ||
             (!camelCase && identifier == QLatin1String( "MUSICBRAINZ_TRACKID" )) )
    {
        if( isValidMusicBrainzId( value ) &&
            !result.contains( Meta::valUniqueId ) ) // AFT id is prefered
            result.insert( Meta::valUniqueId,
                           QVariant("mb-" + value) );
    }

    else if( (camelCase  && identifier == QLatin1String( "Amarok 2 AFTv1 - amarok.kde.org" ) ) ||
             (!camelCase && identifier == QLatin1String( "AMAROK 2 AFTV1 - AMAROK.KDE.ORG" )) )
    {
        if( !value.isEmpty() )
            result.insert( Meta::valUniqueId, value );
    }

    return result;
}

int
Meta::Tag::splitNumber( const QString str )
{
    int i;
    int res;
    bool ok = false;

    i = str.indexOf('/');
    if( i == -1 )
        i = str.indexOf(':');

    // guard against b0rked tags
    if( i == 0 )
        return -1;
    else if( i != -1 )
        res = str.left( i ).toInt( &ok );
        // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
    else
        res = str.toInt( &ok );

    if( !ok )
        return -1;

    return res;
}

#ifndef UTILITIES_BUILD
// the utilities don't need to handle images

QImage
Meta::Tag::embeddedCover( const QString &path )
{
    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return QImage();

    TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() );
    if( !file || !file->ID3v2Tag() || file->ID3v2Tag()->frameListMap()["APIC"].isEmpty() )
        return QImage();

    TagLib::ID3v2::FrameList apicList = file->ID3v2Tag()->frameListMap()["APIC"];
    TagLib::ID3v2::FrameList::ConstIterator iter;
    TagLib::ID3v2::AttachedPictureFrame* frameToUse = 0;

    for( iter = apicList.begin(); iter != apicList.end(); ++iter )
    {
        TagLib::ID3v2::AttachedPictureFrame* currFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*iter);

        // ignore images that are too small
        if( currFrame->picture().size() <= 1024 )
            continue;

        // TODO: we need a better algorithm to find the best image
        if( currFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover )
            frameToUse = currFrame;
        else if( !frameToUse && currFrame->type() == TagLib::ID3v2::AttachedPictureFrame::Other )
            frameToUse = currFrame;
    }
    if( !frameToUse )
        return QImage();

    return QImage::fromData((uchar*)(frameToUse->picture().data()), frameToUse->picture().size());
}

#endif

// ----------------------- writing ------------------------

struct FieldRef {
    const qint64 field;
    const char* name;
};

const FieldRef flacFields[] = {
    { Meta::valAlbumArtist, "ALBUMARTIST" },
    { Meta::valBpm, "BPM" },
    { Meta::valCompilation, "COMPILATION" },
    { Meta::valComposer, "COMPOSER" },
    { Meta::valDiscNr, "DISCNUMBER" },
    { Meta::valPlaycount, "FMPS_PLAYCOUNT" },
    { Meta::valRating, "FMPS_RATING" },
    { Meta::valScore, "FMPS_RATING_AMAROK_SCORE" },
    { 0, 0 }
};

const FieldRef mpegFields[] = {
    { Meta::valAlbumArtist, "TPE2" }, // non-standard: Apple, Microsoft
    { Meta::valBpm, "TBPM" },
    { Meta::valCompilation, "TCMP" },
    { Meta::valComposer, "TCOM" },
    { Meta::valDiscNr, "TPOS" },
    { Meta::valPlaycount, "POPM" },
    { Meta::valRating, "POPM" },
    { Meta::valScore, "TXX" },
    { Meta::valUniqueId, "UFID" },
    { 0, 0 }
};

const FieldRef mp4Fields[] = {
    { Meta::valAlbumArtist, "aART" },
    { Meta::valBpm, "tmpo" },
    { Meta::valCompilation, "cpil" },
    { Meta::valComposer, "\xA9wrt" },
    { Meta::valDiscNr, "disk" },
    { Meta::valRating, "----:com.apple.iTunes:FMPS_Rating" },
    { Meta::valScore, "----:com.apple.iTunes:FMPS_Rating_Amarok_Score" },
    { Meta::valPlaycount, "----:com.apple.iTunes:FMPS_Playcount" },
    { 0, 0 }
};

const FieldRef mpcFields[] = {
    { Meta::valAlbumArtist, "Album Artist" },
    { Meta::valBpm, "BPM" },
    { Meta::valCompilation, "Compilation" },
    { Meta::valComposer, "Composer" },
    { Meta::valDiscNr, "Disc" },
    { Meta::valPlaycount, "FMPS_PLAYCOUNT" },
    { Meta::valRating, "FMPS_RATING" },
    { Meta::valScore, "FMPS_RATING_AMAROK_SCORE" },
    { 0, 0 }
};

const char*
Meta::Tag::fieldName( qint64 field, Meta::Tag::FileTypes type )
{
    FieldRef const *fields;

    switch( type )
    {
    case FLAC:
    case OGG:
    case SPEEX:
        fields = flacFields;
        break;
    case MPEG:
        fields = mpegFields;
        break;
    case MP4:
        fields = mp4Fields;
        break;
    case MPC:
        fields = mpcFields;
        break;
    default:
        return "";
    }

    // note: this loop is not much slower than a sparse case.
    // also this function is not called very frequently
    for( int i = 0; fields[i].name; i++ )
        if( fields[i].field == field )
            return fields[i].name;

    return "";
}

qint64
Meta::Tag::field( const QString &str, Meta::Tag::FileTypes type )
{
    FieldRef const *fields;

    switch( type )
    {
    case FLAC:
    case OGG:
    case SPEEX:
        fields = flacFields;
        break;
    case MPEG:
        fields = mpegFields;
        break;
    case MP4:
        fields = mp4Fields;
        break;
    case MPC:
        fields = mpcFields;
        break;
    default:
        return 0;
    }

    // note: this loop is not much slower than a sparse case.
    // also this function is not called very frequently
    for( int i = 0; fields[i].name; i++ )
        if( fields[i].name == str )
            return fields[i].field;

    return 0;

}

/** When we say uid we really mean url. It should be something like "some-service://123459" */
QPair< QString, QString >
explodeUidUrl( const QString &uidUrl, const Meta::Tag::FileTypes type )
{
    QString owner = QString( "Amarok 2 AFTv1 - amarok.kde.org" );   //TODO: Make version more global?
    QString uid = uidUrl;

    if( uid.startsWith( "amarok-" ) )
        uid = uid.remove( QRegExp( "^(amarok-\\w+://).+$" ) );

    if( uid.startsWith( "mb-" ) )
    {
        switch( type )
        {
        case Meta::Tag::FLAC:
        case Meta::Tag::MPC:
        case Meta::Tag::OGG:
        case Meta::Tag::SPEEX:
            owner = "MUSICBRAINZ_TRACKID";
            break;
        case Meta::Tag::MPEG:
            owner = "http://musicbrainz.org";
            break;
        case Meta::Tag::MP4:
            owner = "----:com.apple.iTunes:MusicBrainz Track Id";
            break;
        }

        uid = uid.mid( 3 );
    }
    else if( type == Meta::Tag::FLAC || type == Meta::Tag::MPC || type == Meta::Tag::OGG || type == Meta::Tag::SPEEX )
        owner = owner.toUpper();
    else if( type == Meta::Tag::MP4 )
        owner = owner.prepend( "----:com.apple.iTunes:" );

    return qMakePair( owner, uid );
}

/** Creates or replaces an exiting tag field with the new information
    @returns true if successfull and saving is needed.
*/
static bool
replaceField( TagLib::FileRef fileref, const qint64 &field, const QVariant &value )
{
    TagLib::String tValue = Qt4QStringToTString( value.toString() );

    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        TagLib::ByteVector tName( fieldName( field, Meta::Tag::MPEG ) );
        if( tName.isEmpty() )
            return false;

        TagLib::ID3v2::Tag *tag = file->ID3v2Tag( true ); // true ^= create

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::MPEG );
            TagLib::String tOwner  = Qt4QStringToTString( uidPair.first );
            TagLib::ByteVector tUid( uidPair.second.toAscii().data() );

            for( TagLib::ID3v2::FrameList::ConstIterator it = tag->frameList().begin(); it != tag->frameList().end(); ++it )
            {
                if( (*it)->frameID() == tName )
                {
                    TagLib::ID3v2::UniqueFileIdentifierFrame* frame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*it);
                    if( !frame )
                        continue;

                    if( frame->owner() == tOwner )
                    {
                        if ( !uidPair.second.isEmpty() )
                            frame->setIdentifier( tUid );
                        else
                            tag->removeFrame( frame );
                       return true; // finished
                    }
                }
            }

            // else, create a new uid frame
            if ( !uidPair.second.isEmpty() )
            {
                TagLib::ID3v2::UniqueFileIdentifierFrame* frame = new TagLib::ID3v2::UniqueFileIdentifierFrame( tOwner, tUid );
                tag->addFrame( frame );
            }
        }
        else if( field == Meta::valScore || field == Meta::valRating || field == Meta::valPlaycount )
        {
            TagLib::String description;
            if( field == Meta::valScore )
            {
                description = "FMPS_Rating_Amarok_Score";
                tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );
            }
            else if( field == Meta::valRating )
            {
                description = "FMPS_Rating";
                tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
            }
            else if( field == Meta::valPlaycount )
            {
                description = "FMPS_Playcount";
                tValue = Qt4QStringToTString( QString::number( value.toInt() ) );
            }

            // POPM gets its information from either Rating (if available) or playcount (if not)
            if( field == Meta::valRating || field == Meta::valPlaycount )
            {
                TagLib::ID3v2::PopularimeterFrame* popFrame = 0;
                if( !tag->frameListMap()[tName].isEmpty() )
                    popFrame = dynamic_cast<TagLib::ID3v2::PopularimeterFrame*>( tag->frameListMap()[tName].front() );

                if( !popFrame )
                {
                    popFrame = new TagLib::ID3v2::PopularimeterFrame( tName );
                    tag->addFrame( popFrame );
                }

                if( field == Meta::valRating )
                    popFrame->setRating( value.toInt() / 10.0 * 255 );
                else
                    popFrame->setCounter( value.toInt() );

                tName = "TXXX";
            }

            //Now only FMPS frames left. tName == "TXXX"
            for( TagLib::ID3v2::FrameList::ConstIterator it = tag->frameList().begin(); it != tag->frameList().end(); ++it )
            {
                if( (*it)->frameID() == tName )
                {
                    TagLib::ID3v2::UserTextIdentificationFrame* frame = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);
                    if( !frame )
                        continue;

                    if( frame->description() == description )
                    {
                        if( value.toBool() )
                            frame->setText( tValue );
                        else
                            tag->removeFrame( frame );
                       return true; // finished
                    }
                }
            }

            // else, create a new comment frame
            if ( value.toBool() )
            {
                TagLib::ID3v2::UserTextIdentificationFrame* frame = new TagLib::ID3v2::UserTextIdentificationFrame( tName );

                frame->setDescription( description );
                frame->setText( tValue );
                tag->addFrame( frame );
                return true;
            }
        }
        else
        {
            if( field == Meta::valCompilation )
                tValue = Qt4QStringToTString( QString::number( value.toInt() ) ); // use 0 and 1 instead of true and false

            if( tValue.isEmpty() )
                tag->removeFrames( tName );
            else
            {
                TagLib::ID3v2::TextIdentificationFrame *frame = 0;
                if( !tag->frameListMap()[tName].isEmpty() )
                    frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>( tag->frameListMap()[tName].front() );

                if( !frame )
                {
                    frame = new TagLib::ID3v2::TextIdentificationFrame( tName );
                    tag->addFrame( frame );
                }

                // note: TagLib is smart enough to automatically set UTF8 encoding if needed.
                frame->setText(tValue);
            }
            return true;
        }
    }
    // ogg
    else if( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        TagLib::String tName = fieldName( field, Meta::Tag::OGG );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::OGG );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if( tName.isEmpty() )
            return false;

        file->tag()->addField( tName,  tValue );
        return true;
    }
    // flac
    else if( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        TagLib::String tName = fieldName( field, Meta::Tag::FLAC );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::FLAC );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return false;

        file->xiphComment()->addField( tName, tValue );
        return true;
    }
    // speex
    else if( TagLib::Ogg::Speex::File *file = dynamic_cast<TagLib::Ogg::Speex::File *>( fileref.file() ) )
    {
        TagLib::String tName = fieldName( field, Meta::Tag::SPEEX );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::SPEEX );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return false;

        file->tag()->addField( tName, tValue );
        return true;
    }
    //MP4
    else if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        TagLib::String tName = fieldName( field, Meta::Tag::MP4 );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::MP4 );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }

        if ( tName.isEmpty() )
            return false;

        TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
        if( field == Meta::valBpm || field == Meta::valDiscNr ||
            field == Meta::valRating || field == Meta::valScore ||
            field == Meta::valPlaycount )
            mp4tag->itemListMap()[tName] = TagLib::MP4::Item( value.toInt(), 0 );
        else if( field == Meta::valCompilation )
            mp4tag->itemListMap()[tName] = TagLib::MP4::Item( value.toBool() );
        else
            mp4tag->itemListMap()[tName] = TagLib::StringList( tValue );
        return true;
    }
    //MPC
    else if( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( fileref.file() ) )
    {
        TagLib::String tName = Meta::Tag::fieldName( field, Meta::Tag::MPC );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), Meta::Tag::MPC );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return false;

        file->APETag( true )->addValue( tName, tValue );
        return true;
    }
    return false;
}

void
Meta::Tag::writeTags( const QString &path, const FieldHash &changes )
{
#ifndef UTILITIES_BUILD
    // depending on the configuration we might not want to write back anything
    if( !AmarokConfig::writeBack() )
        return;
#endif

    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() || changes.isEmpty() )
        return;

    TagLib::Tag *tag = fileref.tag();
    if( !tag )
        return;

    bool needsSaving = false;
    TagLib::String str;
    TagLib::uint val;

    foreach( const qint64 field, changes.keys() )
    {
#ifndef UTILITIES_BUILD

        // Statistics and scores are updated whenever you play a track, so we have a separate option for that.
        if( !AmarokConfig::writeBackStatistics() &&
            (field == Meta::valScore ||
             field == Meta::valFirstPlayed ||
             field == Meta::valLastPlayed ||
             field == Meta::valPlaycount) )
            continue;
#endif
        switch( field )
        {
        case Meta::valTitle:
            str = Qt4QStringToTString( changes.value( Meta::valTitle ).toString() );
            tag->setTitle( str );
            needsSaving = true;
            break;
        case Meta::valAlbum:
            str = Qt4QStringToTString( changes.value( Meta::valAlbum ).toString() );
            tag->setAlbum( str );
            needsSaving = true;
            break;
        case Meta::valArtist:
            str = Qt4QStringToTString( changes.value( Meta::valArtist ).toString() );
            tag->setArtist( str );
            needsSaving = true;
            break;
        case Meta::valComment:
            str = Qt4QStringToTString( changes.value( Meta::valComment ).toString() );
            tag->setComment( str );
            needsSaving = true;
            break;
        case Meta::valGenre:
            str = Qt4QStringToTString( changes.value( Meta::valGenre ).toString() );
            tag->setGenre( str );
            needsSaving = true;
            break;
        case Meta::valYear:
            val = changes.value( Meta::valYear ).toUInt();
            tag->setYear( val );
            needsSaving = true;
            break;
        case Meta::valTrackNr:
            val = changes.value( Meta::valTrackNr ).toUInt();
            tag->setTrack( val );
            needsSaving = true;
            break;
        default:
            needsSaving |= replaceField( fileref, field, changes.value( field ) );
        }
    }

    if( needsSaving )
        fileref.save();
}

#ifndef UTILITIES_BUILD

void
Meta::Tag::setEmbeddedCover( const QString &path, const QImage &cover )
{
    QMutexLocker locker( &s_mutex ); // we do not rely on taglib being thread safe especially when writing the same file from different threads.

    TagLib::FileRef fileref = getFileRef( path );

    if( fileref.isNull() )
        return;

    TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() );
    if( !file || !file->ID3v2Tag() )
        return;

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    qDebug() << "Syncing file image is" << cover.width() << "x" << cover.width();

    if( !cover.save(&buffer, "JPEG") )
        return;

    // -- search for a cover picture we can overwrite
    TagLib::ID3v2::AttachedPictureFrame *apic = 0;
    TagLib::ID3v2::FrameList frames = file->ID3v2Tag()->frameListMap()["APIC"];

    for( unsigned int i=0; i<frames.size() && !apic; i++ )
    {
        apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames[i]);
        if( apic->type() != TagLib::ID3v2::AttachedPictureFrame::FrontCover )
            apic = 0; // that's not it.
    }

    // -- create a new picture frame
    if( !apic )
    {
        apic = new TagLib::ID3v2::AttachedPictureFrame("APIC");
        file->ID3v2Tag()->addFrame(apic);
    }

    qDebug() << "write image with size:" <<bytes.count();

    apic->setMimeType("image/jpeg");
    apic->setType( TagLib::ID3v2::AttachedPictureFrame::FrontCover );
    apic->setPicture( TagLib::ByteVector(bytes.data(), bytes.count()));
    fileref.save();
}

#endif

#undef Qt4QStringToTString

