/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TagLibUtils.h"

#include "meta/support/MetaConstants.h"

#include <QFile>

// Taglib
#include <fileref.h>
#include <tag.h>
#include <tlist.h>
#include <tmap.h>
#include <tstring.h>
#include <tstringlist.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <vorbisfile.h>
#include <textidentificationframe.h>
#include <xiphcomment.h>
#include <mp4file.h>
#include <mp4item.h>
#include <mp4tag.h>
#include <asftag.h>

// Local version of taglib's QStringToTString macro. It is here, because taglib's one is
// not Qt3Support clean (uses QString::utf8()). Once taglib will be clean of qt3support
// it is safe to use QStringToTString again
#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

void
Meta::Field::writeFields( const QString &filename, const QVariantMap &changes )
{
    #ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t* encodedName = reinterpret_cast<const wchar_t *>(filename.utf16());
    #else
    QByteArray fileName = QFile::encodeName( filename );
    const char * encodedName = fileName.constData(); // valid as long as fileName exists
    #endif

    TagLib::FileRef f = TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );
    return writeFields( f, changes );
}

void
Meta::Field::writeFields( TagLib::FileRef fileref, const QVariantMap &changes )
{
    if( fileref.isNull() || changes.isEmpty() )
        return;

    TagLib::Tag *tag = fileref.tag();
    if( !tag )
        return;

    // We should avoid rewriting files to disk if there haven't been any changes to the actual data tags
    // This method could be called when there are only non-tag attributes to change, like score and rating
    bool shouldSave = false;

    if( changes.contains( Meta::Field::TITLE ) )
    {
        shouldSave = true;
        const TagLib::String title = Qt4QStringToTString( changes.value( Meta::Field::TITLE ).toString() );
        tag->setTitle( title );
    }

    if( changes.contains( Meta::Field::ALBUM ) )
    {
        shouldSave = true;
        const TagLib::String album = Qt4QStringToTString( changes.value( Meta::Field::ALBUM ).toString() );
        tag->setAlbum( album );
    }

    if( changes.contains( Meta::Field::ARTIST ) )
    {
        shouldSave = true;
        const TagLib::String artist = Qt4QStringToTString( changes.value( Meta::Field::ARTIST ).toString() );
        tag->setArtist( artist );
    }

    if( changes.contains( Meta::Field::COMMENT ) )
    {
        shouldSave = true;
        const TagLib::String comment = Qt4QStringToTString( changes.value( Meta::Field::COMMENT ).toString() );
        tag->setComment( comment );
    }

    if( changes.contains( Meta::Field::GENRE ) )
    {
        shouldSave = true;
        const TagLib::String genre = Qt4QStringToTString( changes.value( Meta::Field::GENRE ).toString() );
        tag->setGenre( genre );
    }
    if( changes.contains( Meta::Field::YEAR ) )
    {
        shouldSave = true;
        const unsigned int year = changes.value( Meta::Field::YEAR ).toUInt();
        tag->setYear( year );
    }
    if( changes.contains( Meta::Field::TRACKNUMBER ) )
    {
        shouldSave = true;
        const unsigned int trackNumber = changes.value( Meta::Field::TRACKNUMBER ).toUInt();
        tag->setTrack( trackNumber );
    }
    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::Field::COMPOSER ) )
        {
            shouldSave = true;
            if ( file->ID3v2Tag() )
            {
                file->ID3v2Tag()->removeFrames( "TCOM" );
            }
            QString composer = changes.value( Meta::Field::COMPOSER ).toString();
            if ( !composer.isEmpty() )
            {
                TagLib::ID3v2::TextIdentificationFrame* frame =
                        new TagLib::ID3v2::TextIdentificationFrame( "TCOM" );
                frame->setText( Qt4QStringToTString( composer ) );
                file->ID3v2Tag(true)->addFrame( frame );
            }
        }
        if( changes.contains( Meta::Field::DISCNUMBER ) )
        {
            shouldSave = true;
            if( file->ID3v2Tag() )
                file->ID3v2Tag()->removeFrames( "TPOS" );
            const QString discNumber = changes.value( Meta::Field::DISCNUMBER ).toString();
            if( !discNumber.isEmpty() )
            {
                TagLib::ID3v2::TextIdentificationFrame *frame =
                        new TagLib::ID3v2::TextIdentificationFrame( "TPOS" );
                frame->setText( Qt4QStringToTString( discNumber ) );
                file->ID3v2Tag(true)->addFrame( frame );
            }
        }
        if( changes.contains( Meta::Field::BPM ) )
        {
            shouldSave = true;
            if( file->ID3v2Tag() )
                file->ID3v2Tag()->removeFrames( "TBPM" );
            if (changes.value( Meta::Field::BPM ).toDouble() > 0) {
                const QString bpm = changes.value( Meta::Field::BPM ).toString();
                TagLib::ID3v2::TextIdentificationFrame *frame =
                        new TagLib::ID3v2::TextIdentificationFrame( "TBPM" );
                frame->setText( Qt4QStringToTString( bpm ) );
                file->ID3v2Tag(true)->addFrame( frame );
            }
        }
    }
    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::Field::COMPOSER ) )
        {
            shouldSave = true;
            const TagLib::String composer = Qt4QStringToTString( changes.value( Meta::Field::COMPOSER ).toString() );
            file->tag()->addField("COMPOSER", composer);
        }
        if( changes.contains( Meta::Field::DISCNUMBER ) )
        {
            shouldSave = true;
            const TagLib::String disc = Qt4QStringToTString( changes.value( Meta::Field::DISCNUMBER ).toString() );
            file->tag()->addField("DISCNUMBER", disc);
        }
        if( changes.contains( Meta::Field::BPM ) )
        {
            shouldSave = true;
            if (changes.value( Meta::Field::BPM ).toDouble() > 0) {
                const TagLib::String bpm = Qt4QStringToTString( changes.value( Meta::Field::BPM ).toString() );
                file->tag()->addField("BPM", bpm);
            } else {
                file->tag()->removeField("BPM");
            }
        }
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::Field::COMPOSER ) )
        {
            shouldSave = true;
            const TagLib::String composer = Qt4QStringToTString( changes.value( Meta::Field::COMPOSER ).toString() );
            file->tag()->addField("COMPOSER", composer);
        }
        if( changes.contains( Meta::Field::DISCNUMBER ) )
        {
            shouldSave = true;
            const TagLib::String disc = Qt4QStringToTString( changes.value( Meta::Field::DISCNUMBER ).toString() );
            file->tag()->addField("DISCNUMBER", disc);
        }
        if( changes.contains( Meta::Field::BPM ) )
        {
            shouldSave = true;
            if (changes.value( Meta::Field::BPM ).toDouble() > 0) {
                const TagLib::String bpm = Qt4QStringToTString( changes.value( Meta::Field::BPM ).toString() );
                file->tag()->addField("BPM", bpm);
            } else {
                file->tag()->removeField("BPM");
            }
        }
    }
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::Field::COMPOSER ) )
        {
            shouldSave = true;
            const TagLib::String composer = Qt4QStringToTString( changes.value( Meta::Field::COMPOSER ).toString() );
            file->xiphComment()->addField("COMPOSER", composer);
        }
        if( changes.contains( Meta::Field::DISCNUMBER ) )
        {
            shouldSave = true;
            const TagLib::String disc = Qt4QStringToTString( changes.value( Meta::Field::DISCNUMBER ).toString() );
            file->xiphComment()->addField("DISCNUMBER", disc);
        }
        if( changes.contains( Meta::Field::BPM ) )
        {
            shouldSave = true;
            if (changes.value( Meta::Field::BPM ).toDouble() > 0) {
                const TagLib::String bpm = Qt4QStringToTString( changes.value( Meta::Field::BPM ).toString() );
                file->xiphComment()->addField("BPM", bpm);
            } else {
                file->xiphComment()->removeField("BPM");
            }
        }
    }
    else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::Field::COMPOSER ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            const TagLib::String composer = Qt4QStringToTString( changes.value( Meta::Field::COMPOSER ).toString() );
            mp4tag->itemListMap()["\xa9wrt"] = TagLib::StringList( composer );
        }
        if( changes.contains( Meta::Field::DISCNUMBER ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            int discnumber = changes.value( Meta::Field::DISCNUMBER ).toInt();
            mp4tag->itemListMap()["disk"] = TagLib::MP4::Item( discnumber, 0 );
        }
        if( changes.contains( Meta::Field::BPM ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            int bpm = changes.value( Meta::Field::BPM ).toInt();
            mp4tag->itemListMap()["bpm"] = TagLib::MP4::Item( bpm, 0 );
        }
    }
    if( shouldSave )
        fileref.save();
}

#undef Qt4QStringToTString
