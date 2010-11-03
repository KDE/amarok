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

#include "core/meta/support/MetaConstants.h"
#include <core/support/Debug.h>

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
#include <speexfile.h>
#include <oggflacfile.h>
#include <vorbisfile.h>
#include <uniquefileidentifierframe.h>
#include <mp4file.h>
#include <mp4item.h>
#include <mp4tag.h>
#include <asftag.h>

#include <popularimeterframe.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include <attachedpictureframe.h>
#include <commentsframe.h>
#include <xiphcomment.h>

// Local version of taglib's QStringToTString macro. It is here, because taglib's one is
// not Qt3Support clean (uses QString::utf8()). Once taglib will be clean of qt3support
// it is safe to use QStringToTString again
#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

static void
replaceField( TagLib::FileRef fileref,
              const char* id3v2TagId, const char* xiphTagId, const char* mp4TagId,
              const QString &value )
{
    TagLib::String tValue = Qt4QStringToTString( value );

    // id3v2
    if( id3v2TagId )
    {
        if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            TagLib::ID3v2::Tag *tag = file->ID3v2Tag( true ); // true ^= create
            if( value.isEmpty() )
            {
                tag->removeFrames( id3v2TagId );
            }
            else
            {
                TagLib::ID3v2::TextIdentificationFrame *frame = 0;
                if( !tag->frameListMap()[id3v2TagId].isEmpty() )
                {
                    frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(tag->frameListMap()[id3v2TagId].front());
                }

                if( !frame )
                {
                    frame = new TagLib::ID3v2::TextIdentificationFrame(id3v2TagId);
                    tag->addFrame(frame);
                }

                // note: TagLib is smart enough to automatically set UTF8 encoding if needed.
                frame->setText(tValue);
            }
        }
    }

    if( xiphTagId )
    {
        // ogg
        if( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
        {
            file->tag()->addField(xiphTagId,  Qt4QStringToTString( value ), true);
        }

        // flac
        else if( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            file->xiphComment()->addField(xiphTagId, tValue, true);
        }

        // speex
        else if( TagLib::Ogg::Speex::File *file = dynamic_cast<TagLib::Ogg::Speex::File *>( fileref.file() ) )
        {
            file->tag()->addField(xiphTagId, tValue, true);
        }
    }

    // mp4
    if( mp4TagId )
    {
        if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
        {
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            mp4tag->itemListMap()[mp4TagId] = TagLib::StringList( tValue );
        }
    }
}

static void
replaceFMPS( TagLib::FileRef fileref, const QString &name, const QString &value )
{
    TagLib::String tName  = Qt4QStringToTString( name );
    TagLib::String tUpperName  = Qt4QStringToTString( name.toUpper() );
    TagLib::String tValue = Qt4QStringToTString( value );

    // id3v2
    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        // search for an already existing comment frame
        if ( file->ID3v2Tag() )
        {
            for( TagLib::ID3v2::FrameList::ConstIterator it = file->ID3v2Tag()->frameList().begin(); it != file->ID3v2Tag()->frameList().end(); ++it)
            {
                TagLib::String name  = TagLib::String((*it)->frameID());
                if( name == "TXXX" )
                {
                    TagLib::ID3v2::UserTextIdentificationFrame* frame = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);
                    if( !frame )
                        continue;

                    if( frame->description() == tName )
                    {
                        if ( !value.isEmpty() )
                            frame->setText( tValue );
                        else
                            file->ID3v2Tag()->removeFrame(frame, true);
                       return; // finished
                    }
                }
            }
        }

        // else, create a new comment frame
        if ( !value.isEmpty() )
        {
            TagLib::ID3v2::UserTextIdentificationFrame* frame = new TagLib::ID3v2::UserTextIdentificationFrame( "TXXX" );

            frame->setDescription( tName );
            frame->setText( tValue );
            file->ID3v2Tag( true )->addFrame( frame );
        }
    }

    // ogg
    if( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        file->tag()->addField(tUpperName, tValue, true);
    }

    // flac
    if( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        file->xiphComment()->addField(tUpperName, tValue, true);
    }

    // mp4
    if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        QString qName = QString("----:com.apple.iTunes:") + name;
        TagLib::String tNewName = Qt4QStringToTString( qName );

        TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
        mp4tag->itemListMap()[tNewName] = TagLib::StringList( tValue );
    }
}


static void
id3v2SetPOPM( TagLib::FileRef fileref, int rating, int playcount )
{
    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        TagLib::ID3v2::Tag *tag = file->ID3v2Tag( true ); // true ^= create

        TagLib::ID3v2::PopularimeterFrame* frame = 0;
        if(!tag->frameListMap()["POPM"].isEmpty())
        {
            frame = dynamic_cast<TagLib::ID3v2::PopularimeterFrame*>(tag->frameListMap()["POPM"].front());
        }

        if( !frame )
        {
            frame = new TagLib::ID3v2::PopularimeterFrame("POPM");
            tag->addFrame(frame);
        }

        if( rating >= 0 )
            frame->setRating( rating );
        if( playcount >= 0 )
            frame->setCounter( playcount );
    }
}

/** When we say uid we really mean url. It should be something like "some-service://123459" */
static void
replaceUid( TagLib::FileRef fileref, const QString &uid )
{
    QString owner;
    QString newUid = uid;

    if( uid.startsWith( "amarok-sqltrackuid://" ) )
    {
        newUid = uid.mid( 20 );
        if( newUid.startsWith( "mb-" ) )
        {
            owner = "http://musicbrainz.org";
            newUid = newUid.mid( 3 );
        }
        else
        {
            int currentVersion = 1; //TODO: Make this more global?
            owner = QString( "Amarok 2 AFTv" + QString::number( currentVersion ) + " - amarok.kde.org" );
        }
    }
    else if( int index = uid.lastIndexOf("://") > 0 )
    {
        owner = uid.left( index );
        newUid = uid.mid( index+3 );
    }
    else
    {
        return;
    }

debug() << "TagLibUtils writing uid with owner:" << owner << "uid:" << newUid;

    if( owner.isEmpty() )
    {
        warning() << "Could not determine owner of uid"<<uid;
        return;
    }

    TagLib::String tOwner  = Qt4QStringToTString( owner );
    TagLib::ByteVector tUid( newUid.toAscii().data() );

    // id3v2
    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        // search for an already existing uid frame
        if ( file->ID3v2Tag() )
        {
            for( TagLib::ID3v2::FrameList::ConstIterator it = file->ID3v2Tag()->frameList().begin(); it != file->ID3v2Tag()->frameList().end(); ++it)
            {
                TagLib::String name  = TagLib::String((*it)->frameID());
                if( name == "UIDF" )
                {
                    TagLib::ID3v2::UniqueFileIdentifierFrame* frame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*it);
                    if( !frame )
                        continue;

                    if( frame->owner() == tOwner )
                    {
                        if ( !newUid.isEmpty() )
                            frame->setIdentifier( tUid );
                        else
                            file->ID3v2Tag()->removeFrame(frame, true);
                       return; // finished
                    }
                }
            }
        }

        // else, create a new uid frame
        if ( !newUid.isEmpty() )
        {
            TagLib::ID3v2::UniqueFileIdentifierFrame* frame = new TagLib::ID3v2::UniqueFileIdentifierFrame( tOwner, tUid );
            file->ID3v2Tag( true )->addFrame( frame );
        }
    }

    // the rest
    if( uid.startsWith( "mb-" ) )
        replaceField( fileref, 0, "MUSICBRAINZ_TRACKID", "----:com.apple.iTunes:MusicBrainz Track Id", newUid);

    else if( uid.startsWith( "amarok-sqltrackuid://" ) )
        replaceField( fileref, 0, "AMAROK_TRACKID", "----:com.apple.iTunes:Amarok Track Id", newUid);
}



void
Meta::Field::writeFields( const QString &filename, const FieldHash &changes )
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
Meta::Field::writeFields( TagLib::FileRef fileref, const Meta::FieldHash &changes )
{
    if( fileref.isNull() || changes.isEmpty() )
        return;

    TagLib::Tag *tag = fileref.tag();
    if( !tag )
        return;

    // We should avoid rewriting files to disk if there haven't been any changes to the actual data tags
    // This method could be called when there are only non-tag attributes to change
    bool shouldSave = false;

    if( changes.contains( Meta::valTitle ) )
    {
        shouldSave = true;
        const TagLib::String title = Qt4QStringToTString( changes.value( Meta::valTitle ).toString() );
        tag->setTitle( title );
    }

    if( changes.contains( Meta::valAlbum ) )
    {
        shouldSave = true;
        const TagLib::String album = Qt4QStringToTString( changes.value( Meta::valAlbum ).toString() );
        tag->setAlbum( album );
    }

    if( changes.contains( Meta::valArtist ) )
    {
        shouldSave = true;
        const TagLib::String artist = Qt4QStringToTString( changes.value( Meta::valArtist ).toString() );
        tag->setArtist( artist );
    }

    if( changes.contains( Meta::valComment ) )
    {
        shouldSave = true;
        const TagLib::String comment = Qt4QStringToTString( changes.value( Meta::valComment ).toString() );
        tag->setComment( comment );
    }

    if( changes.contains( Meta::valGenre ) )
    {
        shouldSave = true;
        const TagLib::String genre = Qt4QStringToTString( changes.value( Meta::valGenre ).toString() );
        tag->setGenre( genre );
    }
    if( changes.contains( Meta::valYear ) )
    {
        shouldSave = true;
        const unsigned int year = changes.value( Meta::valYear ).toUInt();
        tag->setYear( year );
    }
    if( changes.contains( Meta::valTrackNr ) )
    {
        shouldSave = true;
        const unsigned int trackNumber = changes.value( Meta::valTrackNr ).toUInt();
        tag->setTrack( trackNumber );
    }
    if( changes.contains( Meta::valComposer ) )
    {
        shouldSave = true;
        replaceField( fileref, "TCOM", "COMPOSER", "\xA9wrt",
                      changes.value( Meta::valComposer ).toString());
    }
    if( changes.contains( Meta::valAlbumArtist ) )
    {
        shouldSave = true;
        replaceField( fileref, "TPE2", "ALBUM ARTIST", "aART",
                      changes.value( Meta::valAlbumArtist ).toString());
    }
    if( changes.contains( Meta::valDiscNr ) )
    {
        shouldSave = true;
        replaceField( fileref, "TPOS", "DISCNUMBER", 0,
                      changes.value( Meta::valDiscNr ).toString());
    }
    if( changes.contains( Meta::valBpm ) )
    {
        shouldSave = true;
        replaceField( fileref, "TBPM", "BPM", 0,
                      changes.value( Meta::valBpm ).toString());
    }
    if( changes.contains( Meta::valCompilation ) )
    {
        shouldSave = true;
        replaceField( fileref, "TCMP", "COMPILATION", 0,
                      changes.value( Meta::valCompilation ).toString());
    }
    if( changes.contains( Meta::valRating ) )
    {
        shouldSave = true;
        replaceFMPS( fileref, "FMPS_Rating",
                     QString::number(changes.value( Meta::valRating ).toFloat() / 10.0));

        id3v2SetPOPM( fileref,
                      changes.value( Meta::valRating ).toFloat() / 10.0 * 255,
                      -1 );
    }
    if( changes.contains( Meta::valScore ) )
    {
        shouldSave = true;
        replaceFMPS( fileref, "FMPS_Rating_Amarok_Score",
                     QString::number(changes.value( Meta::valScore ).toFloat() / 100.0));
    }
    if( changes.contains( Meta::valPlaycount ) )
    {
        shouldSave = true;
        replaceFMPS( fileref, "FMPS_Playcount",
                     QString::number(changes.value( Meta::valPlaycount ).toInt()));
        id3v2SetPOPM( fileref,
                      -1,
                      changes.value( Meta::valPlaycount ).toInt() );
    }
    if( changes.contains( Meta::valUniqueId ) )
    {
        shouldSave = true;
        replaceUid( fileref,
                    changes.value( Meta::valUniqueId ).toString() );
    }


    // MP4 has a special handling for numbers
    else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        if( changes.contains( Meta::valDiscNr ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            int discnumber = changes.value( Meta::valDiscNr ).toInt();
            mp4tag->itemListMap()["disk"] = TagLib::MP4::Item( discnumber, 0 );
        }
        if( changes.contains( Meta::valBpm ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            int bpm = changes.value( Meta::valBpm ).toInt();
            mp4tag->itemListMap()["bpm"] = TagLib::MP4::Item( bpm, 0 );
        }
        if( changes.contains( Meta::valCompilation ) )
        {
            shouldSave = true;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            bool compilation = changes.value( Meta::valCompilation ).toInt();
            mp4tag->itemListMap()["cpil"] = TagLib::MP4::Item( compilation );
        }
    }
    if( shouldSave )
        fileref.save();
}

#undef Qt4QStringToTString
