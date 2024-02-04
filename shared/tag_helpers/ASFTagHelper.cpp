/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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


#include "ASFTagHelper.h"

#include "StringHelper.h"

#include <QBuffer>
#include <QImage>

#include <asfattribute.h>

using namespace Meta::Tag;

ASFTagHelper::ASFTagHelper( TagLib::Tag *tag, TagLib::ASF::Tag *asfTag, Amarok::FileType fileType )
            : TagHelper( tag, fileType )
            , m_tag( asfTag )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "WM/AlbumArtist" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "WM/BeatsPerMinute" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "Amarok/Compilation" ) );  //Not standard tag
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "WM/Composer" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "WM/PartOfSet" ) );
    m_fieldMap.insert( Meta::valHasCover,    TagLib::String( "WM/Picture" ) );
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "FMPS/Playcount" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "FMPS/Rating" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "FMPS/Rating_Amarok_Score" ) );
    m_fieldMap.insert( Meta::valLyrics,      TagLib::String( "WM/Lyrics" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "Amarok/AFTv1" ) );
}

Meta::FieldHash
ASFTagHelper::tags() const
{
    Meta::FieldHash data = TagHelper::tags();

    TagLib::ASF::AttributeListMap map = m_tag->attributeListMap();
    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        if( !it->second.size() )
            continue;

        qint64 field;
        TagLib::ASF::Attribute value = it->second[0];
        QString strValue = TStringToQString( value.toString() );
        if( ( field = fieldName( it->first ) ) )
        {
            if( field == Meta::valBpm || field == Meta::valPlaycount )
                data.insert( field, value.toUInt() );
            else if( field == Meta::valRating )
                data.insert( field, qRound( strValue.toFloat() * 10.0 ) );
            else if( field == Meta::valScore )
                data.insert( field, strValue.toFloat() * 100.0 );
            else if( field == Meta::valDiscNr )
                data.insert( field, value.toUInt() );
            else if( field == Meta::valCompilation )
                data.insert( field, value.toBool() );
            else if( field == Meta::valHasCover )
            {
                for( TagLib::ASF::AttributeList::ConstIterator cover = it->second.begin(); cover != it->second.end(); ++cover )
                {
                    if( cover->type() != TagLib::ASF::Attribute::BytesType )
                        continue;

                    TagLib::ASF::Picture pict = cover->toPicture();
                    if( ( pict.type() == TagLib::ASF::Picture::FrontCover ||
                        pict.type() == TagLib::ASF::Picture::Other ) &&
                        pict.dataSize() > MIN_COVER_SIZE )
                    {
                        data.insert( field, true );
                        break;
                    }
                }
            }
            else
                data.insert( field, strValue );
        }
        else if( it->first == uidFieldName( UIDAFT ) && isValidUID( strValue, UIDAFT ) )
            data.insert( Meta::valUniqueId, strValue );
    }

    return data;
}

bool
ASFTagHelper::setTags( const Meta::FieldHash &changes )
{
    bool modified = TagHelper::setTags( changes );

    foreach( const qint64 key, changes.keys() )
    {
        QVariant value = changes.value( key );
        TagLib::String field = fieldName( key );

        if( !field.isEmpty() )
        {
            if( key == Meta::valHasCover )
                continue;
            // http://gitorious.org/~jefferai/xdg-specs/jefferais-xdg-specs/blobs/mediaspecs/specifications/FMPSpecs/specification.txt sais that mp4 tags should be saved as strings
            if( key == Meta::valHasCover )
                continue;
            else if( key == Meta::valRating )
                m_tag->setAttribute( field, TagLib::ASF::Attribute( Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) ) ) );
            else if( key == Meta::valScore )
                m_tag->setAttribute( field, TagLib::ASF::Attribute( Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) ) ) );
            else if( key == Meta::valBpm || key == Meta::valDiscNr )
                m_tag->setAttribute( field, TagLib::ASF::Attribute( value.toUInt() ) );
            else if( key == Meta::valCompilation )
                m_tag->setAttribute( field, TagLib::ASF::Attribute( value.toBool() ) );
            else
                m_tag->setAttribute( field, TagLib::ASF::Attribute( Qt4QStringToTString( value.toString() ) ) );

            modified = true;
        }
        else if( key == Meta::valUniqueId )
        {
            QPair < UIDType, QString > uidPair = splitUID( value.toString() );
            if( uidPair.first == UIDInvalid )
                continue;

            m_tag->setAttribute( uidFieldName( uidPair.first ),
                                 TagLib::ASF::Attribute( Qt4QStringToTString( uidPair.second ) ) );
            modified = true;
        }
    }

    return modified;
}

bool
ASFTagHelper::hasEmbeddedCover() const
{
    TagLib::ASF::AttributeListMap map = m_tag->attributeListMap();
    TagLib::String name = fieldName( Meta::valHasCover );
    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        if( it->first == name )
        {
            TagLib::ASF::AttributeList coverList = it->second;
            for( TagLib::ASF::AttributeList::ConstIterator cover = coverList.begin(); cover != coverList.end(); ++cover )
            {
                if( cover->type() != TagLib::ASF::Attribute::BytesType )
                    continue;

                TagLib::ASF::Picture pict = cover->toPicture();
                if( ( pict.type() == TagLib::ASF::Picture::FrontCover ||
                      pict.type() == TagLib::ASF::Picture::Other ) &&
                    pict.dataSize() > MIN_COVER_SIZE )
                {
                    return true;
                }
            }
        }
    }

    return false;
}

QImage
ASFTagHelper::embeddedCover() const
{
    TagLib::ASF::AttributeListMap map = m_tag->attributeListMap();
    TagLib::String name = fieldName( Meta::valHasCover );

    TagLib::ASF::Picture cover, otherCover;
    bool hasCover = false, hasOtherCover = false;

    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        if( it->first == name )
        {
            TagLib::ASF::AttributeList coverList = it->second;
            for( TagLib::ASF::AttributeList::ConstIterator it = coverList.begin(); it != coverList.end(); ++it )
            {
                if( it->type() != TagLib::ASF::Attribute::BytesType )
                    continue;

                TagLib::ASF::Picture pict = it->toPicture();

                if( pict.dataSize() < MIN_COVER_SIZE )
                    continue;

                if( pict.type() == TagLib::ASF::Picture::FrontCover )
                {
                    cover = pict;
                    hasCover = true;
                }
                else if( pict.type() == TagLib::ASF::Picture::Other )
                {
                    otherCover = pict;
                    hasOtherCover = true;
                }
            }
        }
    }

    if( !hasCover && hasOtherCover )
    {
        cover = otherCover;
        hasCover = true;
    }

    if( !hasCover )
        return QImage();

    return QImage::fromData( ( uchar * ) cover.picture().data(), cover.picture().size() );
}

bool
ASFTagHelper::setEmbeddedCover( const QImage &cover )
{
    QByteArray bytes;
    QBuffer buffer( &bytes );

    buffer.open( QIODevice::WriteOnly );

    if( !cover.save( &buffer, "JPEG" ) )
    {
        buffer.close();
        return false;
    }

    buffer.close();

    TagLib::String name = fieldName( Meta::valHasCover );

    // remove all covers
    m_tag->removeItem( name );

    // add new cover
    TagLib::ASF::Picture picture;
    picture.setPicture( TagLib::ByteVector( bytes.data(), bytes.count() ) );
    picture.setType( TagLib::ASF::Picture::FrontCover );
    picture.setMimeType( "image/jpeg" );
    m_tag->addAttribute( name, TagLib::ASF::Attribute( picture.render() ) );

    return true;
}
