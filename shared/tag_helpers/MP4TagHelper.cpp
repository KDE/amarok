/****************************************************************************************
 * Copyright    (C) 2003-2005 Max Howell <max.howell@methylblue.com>                    *
 *              (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>                    *
 *              (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>                     *
 *              (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                     *
 *              (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>                          *
 *              (C) 2010 Ralf Engels <ralf-engels@gmx.de>                               *
 *              (c) 2010 Sergey Ivanov <123kash@gmail.com>                              *
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


#include "MP4TagHelper.h"

#include "StringHelper.h"

#include <QBuffer>
#include <QImage>

using namespace Meta::Tag;

MP4TagHelper::MP4TagHelper( TagLib::Tag *tag, TagLib::MP4::Tag *mp4Tag, Amarok::FileType fileType )
            : TagHelper( tag, fileType )
            , m_tag( mp4Tag )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "aART" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "tmpo" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "cpil" ) );
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "\xA9wrt" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "disk" ) );
    m_fieldMap.insert( Meta::valHasCover,    TagLib::String( "covr" ) );
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "----:com.apple.iTunes:FMPS_Playcount" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "----:com.apple.iTunes:FMPS_Rating" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "----:com.apple.iTunes:FMPS_Rating_Amarok_Score" ) );
    m_fieldMap.insert( Meta::valLyrics,      TagLib::String( "\xa9lyr" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "----:com.apple.iTunes:Amarok 2 AFTv1 - amarok.kde.org" ) );
}

Meta::FieldHash
MP4TagHelper::tags() const
{
    Meta::FieldHash data = TagHelper::tags();

    TagLib::MP4::ItemMap map = m_tag->itemMap();
    for( TagLib::MP4::ItemMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        qint64 field;
        QString value = TStringToQString( it->second.toStringList().toString( '\n' ) );
        if( ( field = fieldName( it->first ) ) )
        {
            if( field == Meta::valHasCover )
            {
                TagLib::MP4::CoverArtList coverList = it->second.toCoverArtList();
                for( TagLib::MP4::CoverArtList::ConstIterator it = coverList.begin(); it != coverList.end(); ++it )
                    if( it->data().size() > MIN_COVER_SIZE )
                    {
                        data.insert( field, true );
                        break;
                    }
            }

            // http://gitorious.org/~jefferai/xdg-specs/jefferais-xdg-specs/blobs/mediaspecs/specifications/FMPSpecs/specification.txt sais that mp4 tags should be saved as strings
            else if( field == Meta::valPlaycount )
                data.insert( field, value.toInt() );
            else if( field == Meta::valRating )
                data.insert( field, qRound( value.toFloat() * 10.0 ) );
            else if( field == Meta::valScore )
                data.insert( field, value.toFloat() * 100.0 );

            else if( field == Meta::valBpm )
                data.insert( field, it->second.toInt() );
            else if( field == Meta::valDiscNr )
                data.insert( field, it->second.toIntPair().first );

            else if( field == Meta::valCompilation )
                data.insert( field, it->second.toBool() );
            else
                data.insert( field, value );
        }
        else if( it->first == uidFieldName( UIDAFT ) && isValidUID( value, UIDAFT ) )
            data.insert( Meta::valUniqueId, value );
    }

    return data;
}

bool
MP4TagHelper::setTags( const Meta::FieldHash &changes )
{
    bool modified = TagHelper::setTags( changes );

    foreach( const qint64 key, changes.keys() )
    {
        QVariant value = changes.value( key );
        TagLib::String field = fieldName( key );

        if( !field.isEmpty() )
        {
            // http://gitorious.org/~jefferai/xdg-specs/jefferais-xdg-specs/blobs/mediaspecs/specifications/FMPSpecs/specification.txt sais that mp4 tags should be saved as strings
            if( key == Meta::valHasCover )
                continue;
            else if( key == Meta::valRating )
                m_tag->setItem(field, TagLib::StringList( Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) ) ));
            else if( key == Meta::valScore )
                m_tag->setItem(field, TagLib::StringList( Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) ) ));
            else if( key == Meta::valBpm || key == Meta::valDiscNr )
                m_tag->setItem(field, TagLib::MP4::Item( value.toInt(), 0 ));
            else if( key == Meta::valCompilation )
                m_tag->setItem(field, TagLib::MP4::Item( value.toBool() ));
            else
                m_tag->setItem(field, TagLib::StringList( Qt4QStringToTString( value.toString() ) ));

            modified = true;
        }
        else if( key == Meta::valUniqueId )
        {
            QPair < UIDType, QString > uidPair = splitUID( value.toString() );
            if( uidPair.first == UIDInvalid )
                continue;

            m_tag->setItem(uidFieldName( uidPair.first ), TagLib::StringList( Qt4QStringToTString( uidPair.second ) ));
            modified = true;
        }
    }

    return modified;
}

bool
MP4TagHelper::hasEmbeddedCover() const
{
    TagLib::MP4::ItemMap map = m_tag->itemMap();
    TagLib::String name = fieldName( Meta::valHasCover );
    for( TagLib::MP4::ItemMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        if( it->first == name )
        {
            TagLib::MP4::CoverArtList coverList = it->second.toCoverArtList();
            for( TagLib::MP4::CoverArtList::ConstIterator cover = coverList.begin(); cover != coverList.end(); ++cover )
            {
                if( cover->data().size() > MIN_COVER_SIZE )
                    return true;
            }
        }
    }

    return false;
}

QImage
MP4TagHelper::embeddedCover() const
{
    TagLib::MP4::ItemMap map = m_tag->itemMap();
    TagLib::String name = fieldName( Meta::valHasCover );
    for( TagLib::MP4::ItemMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        if( it->first == name )
        {
            TagLib::MP4::CoverArtList coverList = it->second.toCoverArtList();
            for( TagLib::MP4::CoverArtList::Iterator cover = coverList.begin(); cover != coverList.end(); ++cover )
            {
                if( cover->data().size() > MIN_COVER_SIZE )
                    return QImage::fromData( ( uchar * ) cover->data().data(), cover->data().size() );
            }
        }
    }

    return QImage();
}

bool
MP4TagHelper::setEmbeddedCover( const QImage &cover )
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

    TagLib::MP4::CoverArtList covers;

    covers.append( TagLib::MP4::CoverArt( TagLib::MP4::CoverArt::JPEG, TagLib::ByteVector( bytes.data(), bytes.count() ) ) );

    m_tag->setItem(fieldName( Meta::valHasCover ), TagLib::MP4::Item( covers ));

    return true;
}
