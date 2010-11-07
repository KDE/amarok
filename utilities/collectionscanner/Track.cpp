/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
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

#include "Track.h"
#include "AFTUtility.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifdef UTILITIES_BUILD

#include "charset-detector/include/chardet.h"
#include "MetaReplayGain.h"

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

#endif // UTILITIES_BUILD

bool CollectionScanner::Track::s_useCharsetDetector = false;

#ifdef UTILITIES_BUILD

#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

CollectionScanner::Track::Track( const QString &path)
   : m_valid( false )
   , m_filetype( Amarok::Unknown )
   , m_compilation( false )
   , m_noCompilation( false )
   , m_hasCover( false )
   , m_year( -1 )
   , m_disc( -1 )
   , m_track( -1 )
   , m_bpm( -1 )
   , m_bitrate( -1 )
   , m_length( -1 )
   , m_samplerate( -1 )
   , m_filesize( -1 )

   , m_trackGain( 0 )
   , m_trackPeakGain( 0 )
   , m_albumGain( 0 )
   , m_albumPeakGain( 0 )

   , m_rating( -1 )
   , m_score( -1 )
   , m_playcount( -1 )
{
#define strip( x ) TStringToQString( x ).trimmed()

    m_path = path;
    m_rpath = QDir::current().relativeFilePath( path );

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

    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;
    // Tests reveal the following:
    //
    // TagLib::AudioProperties   Relative Time Taken
    //
    //  No AudioProp Reading        1
    //  Fast                        1.18
    //  Average                     Untested
    //  Accurate                    Untested
    TagLib::AudioProperties::ReadStyle readStyle = TagLib::AudioProperties::Fast;

    fileref = TagLib::FileRef( encodedName, true, readStyle );

    if( !fileref.isNull() )
    {
        tag = fileref.tag();
        if( tag )
        {
            m_title = strip( tag->title() );
            m_artist = strip( tag->artist() );
            m_album = strip( tag->album() );
            m_comment = strip( tag->comment() );
            m_genre = strip( tag->genre() );
            if( tag->year() && tag->year() < 2200 )
                m_year = tag->year();
            if( tag->track() )
                m_track = tag->track();

            m_valid = true;
        }

        Meta::ReplayGainTagMap replayGainTags = Meta::readReplayGainTags( fileref );
        if ( replayGainTags.contains( Meta::ReplayGain_Track_Gain ) )
        {
            m_trackGain = replayGainTags[Meta::ReplayGain_Track_Gain];
            if ( replayGainTags.contains( Meta::ReplayGain_Track_Peak ) )
                m_trackPeakGain = replayGainTags[Meta::ReplayGain_Track_Peak];
        }
        if ( replayGainTags.contains( Meta::ReplayGain_Album_Gain ) )
        {
            m_albumGain = replayGainTags[Meta::ReplayGain_Album_Gain];
            if ( replayGainTags.contains( Meta::ReplayGain_Album_Peak ) )
                m_albumPeakGain = replayGainTags[Meta::ReplayGain_Album_Peak];
        }

        /* As MPEG implementation on TagLib uses a Tag class that's not defined on the headers,
           we have to cast the files, not the tags! */
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            m_filetype = Amarok::Mp3;
            if ( file->ID3v2Tag() )
            {
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
                            decodeFMPS( TStringToQString( fields[0] ),
                                        TStringToQString( fields[1] ), true );
                        }
                    }

                    else if( name[0] == 'T' )
                    {
                        TagLib::ID3v2::TextIdentificationFrame* frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(*it);
                        if( !frame )
                            continue;

                        QString value = TStringToQString(frame->fieldList().toString("\n"));

                        if( name == "TPOS" )
                            m_disc = splitNumber(value);

                        else if( name == "TBPM" )
                            m_bpm = value.toFloat();

                        else if( name == "TCOM" )
                            m_composer = value;

                        else if( name == "TPE2" ) // non-standard: Apple, Microsoft
                            m_albumArtist = value;

                        // -- compilation
                        else if( name == "TCMP" )
                        {
                            if( value.toInt() )
                                m_compilation = true;
                            else
                                m_noCompilation = true;
                        }
                    }

                    // -- rating, playcount
                    else if( name == "POPM" )
                    {
                        TagLib::ID3v2::PopularimeterFrame *frame = dynamic_cast<TagLib::ID3v2::PopularimeterFrame *>(*it);
                        if( !frame )
                            continue;

                        if( TStringToQString(frame->email()) == "" ) // only read anonymous ratings
                        {
                            // FMPS tags have precedence
                            if( m_rating == -1 && frame->rating() != 0 )
                                m_rating = qreal(frame->rating()) / 256.0;
                            if( m_playcount == -1 && frame->counter() < 10000 )
                                m_playcount = frame->counter();
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
                            m_hasCover = true;
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
                        if( owner == "http://musicbrainz.org" )
                            m_uniqueid = "amarok-sqltrackuid://mb-" + identifier;

                        else if( owner == "Amarok 2 AFTv1 - amarok.kde.org" )
                            m_uniqueid = "amarok-sqltrackuid://" + identifier;

                        else
                            m_uniqueid = owner+"://"+identifier;
                    }
                }
            }

            // HACK: charset-detector disabled, so all tags assumed utf-8
            // TODO: fix charset-detector to detect encoding with higher accuracy

            if( s_useCharsetDetector && tag )
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
                            m_title = codec->toUnicode( strip( tag->title() ).toLatin1() );
                            m_artist = codec->toUnicode( strip( tag->artist() ).toLatin1() );
                            m_album = codec->toUnicode( strip( tag->album() ).toLatin1() );
                            m_comment = codec->toUnicode( strip( tag->comment() ).toLatin1() );
                        }
                    }
                }
            }
        }

        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
        {
            m_filetype = Amarok::Ogg;
            if ( file->tag() )
            {
                for( TagLib::Ogg::FieldListMap::ConstIterator it = file->tag()->fieldListMap().begin(); it != file->tag()->fieldListMap().end(); ++it )
                {
                    QString name = TStringToQString( it->first );
                    QString value = TStringToQString( it->second.toString("\n") );
                    decodeXiph( name, value );
                }
            }
        }

        else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            m_filetype = Amarok::Flac;
            if ( file->xiphComment() )
            {
                for( TagLib::Ogg::FieldListMap::ConstIterator it = file->xiphComment()->fieldListMap().begin(); it != file->xiphComment()->fieldListMap().end(); ++it )
                {
                    QString name = TStringToQString( it->first );
                    QString value = TStringToQString( it->second.toString("\n") );
                    decodeXiph( name, value );
                }
            }
        }

        else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
        {
            m_filetype = Amarok::Mp4;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            if( mp4tag )
            {
                for( TagLib::MP4::ItemListMap::Iterator it = mp4tag->itemListMap().begin(); it != mp4tag->itemListMap().end(); ++it )
                {
                    QString name = TStringToQString( it->first );
                    QString value = TStringToQString( it->second.toStringList().toString("\n") );

                    if( name == "\xA9wrt" )
                        m_composer = value;

                    else if( name == "aART" ) // iTunes 4.0
                        m_albumArtist = value;

                    else if( name == "tmpo" )
                        m_bpm = it->second.toInt();

                    else if( name == "disk" )
                        m_disc = it->second.toIntPair().first;

                    else if( name == "cpil" )
                    {
                        if( it->second.toBool() )
                            m_compilation = true;
                        else
                            m_noCompilation = true;
                    }

                    else if( name == "----:com.apple.iTunes:MusicBrainz Track Id" )
                        m_uniqueid = "mb-" + value;

                    else if( name == "----:com.apple.iTunes:Amarok 2 AFTv1 - amarok.kde.org" )
                        m_uniqueid = "amarok-sqltrackuid://" + value;

                    else
                    {
                        if( name.startsWith("----:com.apple.iTunes") )
                        {
                            decodeFMPS( name.remove(0, 22), value, true );
                        }
                    }

//                 if ( images && mp4tag->cover().size() )
//                     images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
                }
            }
        }
        //we didn't set a FileType till now, let's look it up via FileExtension
        if ( m_filetype == Amarok::Unknown )
        {
            QString ext = path.mid( path.lastIndexOf( '.' ) + 1 ); 
            m_filetype = Amarok::FileTypeSupport::fileType( ext );
        }
    }

    if( m_valid && fileref.audioProperties() )
    {
        m_bitrate = fileref.audioProperties()->bitrate();
        m_length = fileref.audioProperties()->length() * 1000;
        m_samplerate = fileref.audioProperties()->sampleRate();
    }

    m_filesize = QFile( path ).size();

    if( m_valid && m_uniqueid.isEmpty() )
    {
        AFTUtility aftutil;
        m_uniqueid = "amarok-sqltrackuid://" + aftutil.readUniqueId( path );
    }

    // TODO:
    // --- if no tags could be found. Invent something from the filename
    // replace underscores with spaces
    // capitalize each word if all characters are lower case
    // - split up the filename at dashes
    // if the filename starts with a number that is probably a track number
    // - two parts mean "artist - title"

#undef strip
}

#endif // UTILITIES_BUILD

CollectionScanner::Track::Track( QXmlStreamReader *reader )
   : m_valid( true )
   , m_filetype( Amarok::Unknown )
   , m_compilation( false )
   , m_noCompilation( false )
   , m_hasCover( false )
   , m_year( -1 )
   , m_disc( -1 )
   , m_track( -1 )
   , m_bpm( -1 )
   , m_bitrate( -1 )
   , m_length( -1 )
   , m_samplerate( -1 )
   , m_filesize( -1 )

   , m_trackGain( -1 )
   , m_trackPeakGain( -1 )
   , m_albumGain( -1 )
   , m_albumPeakGain( -1 )

   , m_rating( -1 )
   , m_score( -1 )
   , m_playcount( -1 )
{
    // improve scanner with skipCurrentElement as soon as Amarok requires Qt 4.6
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "uniqueid" )
                m_uniqueid = reader->readElementText();
            else if( name == "path" )
                m_path = reader->readElementText();
            else if( name == "rpath" )
                m_rpath = reader->readElementText();

            else if( name == "filetype" )
                m_filetype = (Amarok::FileType)reader->readElementText().toInt();
            else if( name == "title" )
                m_title = reader->readElementText();
            else if( name == "artist" )
                m_artist = reader->readElementText();
            else if( name == "albumArtist" )
                m_albumArtist = reader->readElementText();
            else if( name == "album" )
                m_album = reader->readElementText();
            else if( name == "compilation" )
            {
                m_compilation = true;
                reader->readNext(); // this is an empty element and I read over the end element here
            }
            else if( name == "noCompilation" )
            {
                m_noCompilation = true;
                reader->readNext(); // this is an empty element and I read over the end element here
            }
            else if( name == "hasCover" )
            {
                m_hasCover = true;
                reader->readNext(); // this is an empty element and I read over the end element here
            }
            else if( name == "comment" )
                m_comment = reader->readElementText();
            else if( name == "genre" )
                m_genre = reader->readElementText();
            else if( name == "year" )
                m_year = reader->readElementText().toInt();
            else if( name == "disc" )
                m_disc = reader->readElementText().toInt();
            else if( name == "track" )
                m_track = reader->readElementText().toInt();
            else if( name == "bpm" )
                m_bpm = reader->readElementText().toFloat();
            else if( name == "bitrate" )
                m_bitrate = reader->readElementText().toInt();
            else if( name == "length" )
                m_length = reader->readElementText().toLong();
            else if( name == "samplerate" )
                m_samplerate = reader->readElementText().toInt();
            else if( name == "filesize" )
                m_filesize = reader->readElementText().toLong();

            else if( name == "trackGain" )
                m_trackGain = reader->readElementText().toFloat();
            else if( name == "trackPeakGain" )
                m_trackPeakGain = reader->readElementText().toFloat();
            else if( name == "albumGain" )
                m_albumGain = reader->readElementText().toFloat();
            else if( name == "albumPeakGain" )
                m_albumPeakGain = reader->readElementText().toFloat();

            else if( name == "composer" )
                m_composer = reader->readElementText();

            else if( name == "rating" )
                m_rating = reader->readElementText().toFloat();
            else if( name == "score" )
                m_score = reader->readElementText().toFloat();
            else if( name == "playcount" )
                m_playcount = reader->readElementText().toInt();

            else
            {
                qDebug() << "Unexpected xml start element"<<name<<"in input";
                reader->skipCurrentElement();
            }
        }

        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

#ifdef UTILITIES_BUILD

void
CollectionScanner::Track::toXml( QXmlStreamWriter *writer ) const
{
    if( !m_valid )
        return;

    if( !m_uniqueid.isEmpty() )
        writer->writeTextElement( "uniqueid", m_uniqueid );
    if( !m_path.isEmpty() )
        writer->writeTextElement( "path", m_path );
    if( !m_rpath.isEmpty() )
        writer->writeTextElement( "rpath", m_rpath );

    writer->writeTextElement( "filetype", QString::number( (int)m_filetype ) );

    if( !m_title.isEmpty() )
        writer->writeTextElement( "title", m_title);
    if( !m_artist.isEmpty() )
        writer->writeTextElement( "artist", m_artist);
    if( !m_albumArtist.isEmpty() )
        writer->writeTextElement( "albumArtist", m_albumArtist);
    if( !m_album.isEmpty() )
        writer->writeTextElement( "album", m_album);
    if( m_compilation )
        writer->writeEmptyElement( "compilation" );
    if( m_noCompilation )
        writer->writeEmptyElement( "noCompilation" );
    if( m_hasCover )
        writer->writeEmptyElement( "hasCover" );
    if( !m_comment.isEmpty() )
        writer->writeTextElement( "comment", m_comment);
    if( !m_genre.isEmpty() )
        writer->writeTextElement( "genre", m_genre);
    if( m_year != -1 )
        writer->writeTextElement( "year", QString::number( m_year ) );
    if( m_disc != -1 )
        writer->writeTextElement( "disc", QString::number( m_disc ) );
    if( m_track != -1 )
        writer->writeTextElement( "track", QString::number( m_track ) );
    if( m_bpm != -1 )
        writer->writeTextElement( "bpm", QString::number( m_bpm ) );
    if( m_bitrate != -1 )
        writer->writeTextElement( "bitrate", QString::number( m_bitrate ) );
    if( m_length != -1 )
        writer->writeTextElement( "length", QString::number( m_length ) );
    if( m_samplerate != -1 )
        writer->writeTextElement( "samplerate", QString::number( m_samplerate ) );
    if( m_filesize != -1 )
        writer->writeTextElement( "filesize", QString::number( m_filesize ) );

    if( m_trackGain != 0 )
        writer->writeTextElement( "trackGain", QString::number( m_trackGain ) );
    if( m_trackPeakGain != 0 )
        writer->writeTextElement( "trackPeakGain", QString::number( m_trackPeakGain ) );
    if( m_albumGain != 0 )
        writer->writeTextElement( "albumGain", QString::number( m_albumGain ) );
    if( m_albumPeakGain != 0 )
        writer->writeTextElement( "albumPeakGain", QString::number( m_albumPeakGain ) );

    if( !m_composer.isEmpty() )
        writer->writeTextElement( "composer", m_composer);

    if( m_rating != -1 )
        writer->writeTextElement( "rating", QString::number( m_rating ) );
    if( m_score != -1 )
        writer->writeTextElement( "score", QString::number( m_score ) );
    if( m_playcount != -1 )
        writer->writeTextElement( "playcount", QString::number( m_playcount ) );

}

void
CollectionScanner::Track::decodeXiph( const QString &name, const QString &value )
{
    if( name == "COMPOSER" )
        m_composer = value;

    else if( name == "ALBUMARTIST" )
        m_albumArtist = value;

    else if( name == "BPM" )
        m_bpm = value.toFloat();

    else if( name == "DISCNUMBER" )
        m_disc = splitNumber(value.trimmed());

    else if( name == "COMPILATION" )
    {
        if( value.toInt() )
            m_compilation = true;
        else
            m_noCompilation = true;
    }
    else if( name == "MUSICBRAINZ_TRACKID" )
    {
        m_uniqueid = "mb-" + value;
    }
    else if( name == "AMAROK 2 AFTV1 - AMAROK.KDE.ORG" )
    {
        m_uniqueid = "amarok-sqltrackuid://" + value;
    }
    else
        decodeFMPS( name, value, false );
}

void
CollectionScanner::Track::decodeFMPS( const QString &identifier, const QString &value, bool camelCase )
{
    bool ok = false;
    qreal f = value.toFloat( &ok );

    if( (camelCase  && identifier == "FMPS_Rating") ||
        (!camelCase && identifier == "FMPS_RATING") )
    {
        if( ok )
            m_rating = f;
    }

    else if( (camelCase  && identifier == "FMPS_Rating_Amarok_Score") ||
             (!camelCase && identifier == "FMPS_RATING_AMAROK_SCORE") )
    {
        if( ok )
            m_score = f;
    }

    else if( (camelCase  && identifier == "FMPS_Playcount") ||
             (!camelCase && identifier == "FMPS_PLAYCOUNT") )
    {
        if( ok )
            m_playcount = f;
    }
}

int
CollectionScanner::Track::splitNumber( const QString str ) const
{
    int i;
    int res;
    bool ok = false;

    i = str.indexOf('/');
    if( i != -1 )
        i = str.indexOf(':');

    // guard against b0rked tags
    if ( i != -1 )
        // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
        res = str.left( i ).toInt( &ok );
    else
        res = str.toInt( &ok );

    if( !ok )
        return -1;

    return res;
}

#endif // UTILITIES_BUILD


bool
CollectionScanner::Track::isValid() const
{
    return m_valid;
}

QString
CollectionScanner::Track::uniqueid() const
{
    return m_uniqueid;
}

QString
CollectionScanner::Track::path() const
{
    return m_path;
}

QString
CollectionScanner::Track::rpath() const
{
    return m_rpath;
}

Amarok::FileType
CollectionScanner::Track::filetype() const
{
    return m_filetype;
}

QString
CollectionScanner::Track::title() const
{
    return m_title;
}

QString
CollectionScanner::Track::artist() const
{
    return m_artist;
}

QString
CollectionScanner::Track::albumArtist() const
{
    return m_albumArtist;
}

QString
CollectionScanner::Track::album() const
{
    return m_album;
}

bool
CollectionScanner::Track::isCompilation() const
{
    return m_compilation;
}

bool
CollectionScanner::Track::isNoCompilation() const
{
    return m_noCompilation;
}


bool
CollectionScanner::Track::hasCover() const
{
    return m_hasCover;
}

QString
CollectionScanner::Track::comment() const
{
    return m_comment;
}

QString
CollectionScanner::Track::genre() const
{
    return m_genre;
}

int
CollectionScanner::Track::year() const
{
    return m_year;
}

int
CollectionScanner::Track::disc() const
{
    return m_disc;
}

int
CollectionScanner::Track::track() const
{
    return m_track;
}

int
CollectionScanner::Track::bpm() const
{
    return m_bpm;
}

int
CollectionScanner::Track::bitrate() const
{
    return m_bitrate;
}

qint64
CollectionScanner::Track::length() const
{
    return m_length;
}

int
CollectionScanner::Track::samplerate() const
{
    return m_samplerate;
}

qint64
CollectionScanner::Track::filesize() const
{
    return m_filesize;
}


QString
CollectionScanner::Track::composer() const
{
    return m_composer;
}


qreal
CollectionScanner::Track::replayGain( Meta::ReplayGainTag mode ) const
{
    switch( mode )
    {
    case Meta::ReplayGain_Track_Gain:
        return m_trackGain;
    case Meta::ReplayGain_Track_Peak:
        return m_trackPeakGain;
    case Meta::ReplayGain_Album_Gain:
        return m_albumGain;
    case Meta::ReplayGain_Album_Peak:
        return m_albumPeakGain;
    }
    return 0.0;
}

qreal
CollectionScanner::Track::rating() const
{
    return m_rating;
}

qreal
CollectionScanner::Track::score() const
{
    return m_score;
}

int
CollectionScanner::Track::playcount() const
{
    return m_playcount;
}

void
CollectionScanner::Track::setUseCharsetDetector( bool value )
{
    s_useCharsetDetector = value;
}


