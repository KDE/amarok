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
#include <mpcfile.h>
#include <apetag.h>
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

const char *
fieldName( const qint64 &field, const FileTypes &type )
{
    if( type == FLAC ||
        type == OGG  ||
        type == SPEEX )
    {
        switch ( field )
        {
        case Meta::valAlbumArtist:
            return "ALBUMARTIST";
        case Meta::valBpm:
            return "BPM";
        case Meta::valCompilation:
            return "COMPILATION";
        case Meta::valComposer:
            return "COMPOSER";
        case Meta::valDiscNr:
            return "DISCNUMBER";
        case Meta::valPlaycount:
            return "FMPS_PLAYCOUNT";
        case Meta::valRating:
            return "FMPS_RATING";
        case Meta::valScore:
            return "FMPS_RATING_AMAROK_SCORE";
        }
    }
    else if( type == MPEG )
    {
        switch ( field )
        {
        case Meta::valAlbumArtist:
            return "TPE2";
        case Meta::valBpm:
            return "TBPM";
        case Meta::valCompilation:
            return "TCMP";
        case Meta::valComposer:
            return "TCOM";
        case Meta::valDiscNr:
            return "TPOS";
        case Meta::valPlaycount:
            return "POPM";
        case Meta::valRating:
            return "POPM";
        case Meta::valScore:
            return "TXX";
        case Meta::valUniqueId:
            return "UFID";
        }
    }
    else if( type == MP4 )
    {
        switch ( field )
        {
        case Meta::valAlbumArtist:
            return "aART";
        case Meta::valBpm:
            return "tmpo";
        case Meta::valCompilation:
            return "cpil";
        case Meta::valComposer:
            return "\xA9wrt";
        case Meta::valDiscNr:
            return "disk";
        case Meta::valRating:
            return "----:com.apple.iTunes:FMPS_Rating";
        case Meta::valScore:
            return "----:com.apple.iTunes:FMPS_Rating_Amarok_Score";
        case Meta::valPlaycount:
            return "----:com.apple.iTunes:FMPS_Playcount";
        }
    }
    else if( type == MPC )
    {
        switch ( field )
        {
        case Meta::valAlbumArtist:
            return "Album Artist";
        case Meta::valBpm:
            return "BPM";
        case Meta::valCompilation:
            return "Compilation";
        case Meta::valComposer:
            return "Composer";
        case Meta::valDiscNr:
            return "Disc";
        case Meta::valPlaycount:
            return "FMPS_PLAYCOUNT";
        case Meta::valRating:
            return "FMPS_RATING";
        case Meta::valScore:
            return "FMPS_RATING_AMAROK_SCORE";
        }
    }

    return "";
}

/** When we say uid we really mean url. It should be something like "some-service://123459" */
QPair< QString, QString >
explodeUidUrl( const QString &uidUrl, const FileTypes type )
{
    QString owner = QString( "Amarok 2 AFTv1 - amarok.kde.org" );   //TODO: Make version more global?
    QString uid = uidUrl;

    if( uid.startsWith( "amarok-" ) )
        uid = uid.remove( QRegExp( "^(amarok-\\w+://).+$" ) );

    if( uid.startsWith( "mb-" ) )
    {
        switch( type )
        {
        case FLAC:
        case MPC:
        case OGG:
        case SPEEX:
            owner = "MUSICBRAINZ_TRACKID";
            break;
        case MPEG:
            owner = "http://musicbrainz.org";
            break;
        case MP4:
            owner = "----:com.apple.iTunes:MusicBrainz Track Id";
            break;
        }

        uid = uid.mid( 3 );
    }
    else if( type == FLAC || type == MPC || type == OGG || type == SPEEX )
        owner = owner.toUpper();
    else if( type == MP4 )
        owner = owner.prepend( "----:com.apple.iTunes:" );

    return qMakePair( owner, uid );
}

static void
replaceField( TagLib::FileRef fileref, const qint64 &field, const QVariant &value )
{
    TagLib::String tName;
    TagLib::String tValue = Qt4QStringToTString( value.toString() );

    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        TagLib::ByteVector tName( fieldName( field, MPEG ) );

        if( tName.isEmpty() )
            return;

        TagLib::ID3v2::Tag *tag = file->ID3v2Tag( true ); // true ^= create

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), MPEG );
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
                       return; // finished
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

            if( field == Meta::valRating || field == Meta::valPlaycount )       //tName == "POPM"
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
                       return; // finished
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
            }
        }
        else
        {
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
        }
    }
    // ogg
    else if( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        tName = fieldName( field, OGG );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), OGG );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if( tName.isEmpty() )
            return;

        file->tag()->addField( tName,  tValue );
    }
    // flac
    else if( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        tName = fieldName( field, FLAC );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), FLAC );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return;

        file->xiphComment()->addField( tName, tValue );
    }
    // speex
    else if( TagLib::Ogg::Speex::File *file = dynamic_cast<TagLib::Ogg::Speex::File *>( fileref.file() ) )
    {
        tName = fieldName( field, SPEEX );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), SPEEX );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return;

        file->tag()->addField( tName, tValue );
    }
    //MP4
    else if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        tName = fieldName( field, MP4 );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), MP4 );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }

        if ( tName.isEmpty() )
            return;

        TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
        if( field == Meta::valBpm || field == Meta::valDiscNr ||
            field == Meta::valRating || field == Meta::valScore ||
            field == Meta::valPlaycount )
            mp4tag->itemListMap()[tName] = TagLib::MP4::Item( value.toInt(), 0 );
        else if( field == Meta::valCompilation )
            mp4tag->itemListMap()[tName] = TagLib::MP4::Item( value.toBool() );
        else
            mp4tag->itemListMap()[tName] = TagLib::StringList( tValue );
    }
    //MPC
    else if( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( fileref.file() ) )
    {
        tName = fieldName( field, MPC );

        if( field == Meta::valUniqueId )
        {
            QPair< QString, QString > uidPair = explodeUidUrl( value.toString(), MPC );
            tName = Qt4QStringToTString( uidPair.first );
            tValue = Qt4QStringToTString( uidPair.second );
        }
        else if( field == Meta::valRating )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) );
        else if( field == Meta::valScore )
            tValue = Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) );

        if ( tName.isEmpty() )
            return;

        file->APETag( true )->addValue( tName, tValue );
    }
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

    TagLib::String str;
    TagLib::uint val;

    foreach( const qint64 field, changes.keys() )
    {
        switch( field )
        {
        case Meta::valTitle:
            str = Qt4QStringToTString( changes.value( Meta::valTitle ).toString() );
            tag->setTitle( str );
            break;
        case Meta::valAlbum:
            str = Qt4QStringToTString( changes.value( Meta::valAlbum ).toString() );
            tag->setAlbum( str );
            break;
        case Meta::valArtist:
            str = Qt4QStringToTString( changes.value( Meta::valArtist ).toString() );
            tag->setArtist( str );
            break;
        case Meta::valComment:
            str = Qt4QStringToTString( changes.value( Meta::valComment ).toString() );
            tag->setComment( str );
            break;
        case Meta::valGenre:
            str = Qt4QStringToTString( changes.value( Meta::valGenre ).toString() );
            tag->setGenre( str );
            break;
        case Meta::valYear:
            val = changes.value( Meta::valYear ).toUInt();
            tag->setYear( val );
            break;
        case Meta::valTrackNr:
            val = changes.value( Meta::valTrackNr ).toUInt();
            tag->setTrack( val );
            break;
        default:
            replaceField( fileref, field, changes.value( field ) );
        }
    }

    if( !changes.isEmpty() )
        fileref.save();
}

#undef Qt4QStringToTString
