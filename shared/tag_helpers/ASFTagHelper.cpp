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

#include <asfattribute.h>

#ifndef UTILITIES_BUILD
    #include <QBuffer>
#endif  //UTILITIES_BUILD

#include "StringHelper.h"

using namespace Meta::Tag;


class ASFPicture
{
    public:
        enum PictureType
        {
            Other                       = 0,
            PNGIcon                     = 1,
            OtherIcon                   = 2,
            FrontCover                  = 3,
            BackCover                   = 4,
            Leaflet                     = 5,
            Media                       = 6,
            LeadArtist                  = 7,
            Artist                      = 8,
            Conductor                   = 9,
            Band                        = 10,
            Composer                    = 11,
            Lyricist                    = 12,
            RecordingStudioOrLocation   = 13,
            RecordingSession            = 14,
            Performance                 = 15,
            CaptureFromMovieOrVideo     = 16,
            BrightColoredFish           = 17,
            Illustration                = 18,
            BandLogo                    = 19,
            PublisherLogo               = 20
        };

        ASFPicture();
        ASFPicture( const TagLib::ByteVector &bv );

        ASFPicture &operator=( const ASFPicture &picture );

        PictureType type() const;
        QString mimeType() const;
        QString description() const;

        QByteArray data() const;
        int size() const;
#ifndef UTILITIES_BUILD
        ASFPicture( const QImage &image );
        QImage image() const;
        TagLib::ByteVector toByteVector() const;
#endif  //UTILITIES_BUILD
    private:
        PictureType m_type;
        QString m_mimeType;
        QString m_description;
        QByteArray m_data;
};


ASFPicture::ASFPicture()
          : m_type( PublisherLogo )             // We won't use such
{
}

ASFPicture::ASFPicture( const TagLib::ByteVector &bv )
{
    if( bv.isEmpty() || bv.isNull() )
    {
        m_type = PublisherLogo;
        return;
    }

    qint32 pos = 0;
    const char *data = bv.data();
    m_type = PictureType( *data );
    pos++;
    quint32 pictSize = *( quint32 *)(data + pos );
    pos += 4;
    m_mimeType = QString::fromUtf16( ( const ushort *)( data + pos ) );
    pos += 2 * ( m_mimeType.length() + 1 );
    m_description = QString::fromUtf16( ( const ushort *)( data + pos ) );
    pos += 2 * ( m_description.length() + 1 );
    m_data = QByteArray( data + pos, pictSize );
}

ASFPicture &
ASFPicture::operator=( const ASFPicture &picture )
{
    m_type = picture.m_type;
    m_mimeType = picture.m_mimeType;
    m_description = picture.m_description;
    m_data = picture.m_data;

    return *this;
}

ASFPicture::PictureType
ASFPicture::type() const
{
    return m_type;
}

QString
ASFPicture::mimeType() const
{
    return m_mimeType;
}

QString
ASFPicture::description() const
{
    return m_description;
}

QByteArray
ASFPicture::data() const
{
    return m_data;
}

int
ASFPicture::size() const
{
    return m_data.size();
}

#ifndef UTILITIES_BUILD
ASFPicture::ASFPicture( const QImage &image )
          : m_type( FrontCover )
          , m_mimeType( "image/jpeg" )
{
    QBuffer buffer( &m_data );

    buffer.open( QIODevice::WriteOnly );
    image.save( &buffer, "JPEG" );
    buffer.close();
}

QImage
ASFPicture::image() const
{
    return QImage::fromData( m_data );
}

TagLib::ByteVector
ASFPicture::toByteVector() const
{
    QByteArray data;

    uchar type = m_type;
    qint32 pictSize = m_data.size();

    QBuffer buffer( &data );
    buffer.open( QIODevice::WriteOnly );

    buffer.write( ( const char *)&type, 1 );
    buffer.write( ( const char *)&pictSize, 4 );
    buffer.write( ( const char *)m_mimeType.utf16(), ( m_mimeType.length() + 1 ) * 2 );
    buffer.write( ( const char *)m_description.utf16(), ( m_description.length() + 1 ) * 2 );
    buffer.write( m_data.data(), pictSize );

    buffer.close();

    return TagLib::ByteVector( data.data(), data.size() );
}

#endif  //UTILITIES_BUILD

ASFTagHelper::ASFTagHelper( TagLib::Tag *tag, TagLib::ASF::Tag *asfTag, Amarok::FileType fileType )
            : TagHelper( tag, fileType )
            , m_tag( asfTag )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "WM/AlbumTitle" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "WM/BeatsPerMinute" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "Amarok/Compilation" ) );  //Not standatd tag
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "WM/Composer" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "WM/PartOfSet" ) );
    m_fieldMap.insert( Meta::valHasCover,    TagLib::String( "WM/Picture" ) );
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "FMPS/Playcount" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "FMPS/Rating" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "FMPS/Rating_Amarok_Score" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "Amarok/AFTv1" ) );
    m_uidFieldMap.insert( UIDMusicBrainz,    TagLib::String( "MusicBrainz/Track Id" ) );
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

                    ASFPicture pict( cover->toByteVector() );
                    if( ( pict.type() == ASFPicture::FrontCover ||
                          pict.type() == ASFPicture::Other ) &&
                        pict.size() > 1024 )
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
        else if( it->first == uidFieldName( UIDMusicBrainz ) && isValidUID( strValue, UIDMusicBrainz ) )
        {
            if( !data.contains( Meta::valUniqueId ) ) // we prefere AFT uids
                data.insert( Meta::valUniqueId, strValue.prepend( "mb-" ) );
        }
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

        if( !field.isNull() && !field.isEmpty() )
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

#ifndef UTILITIES_BUILD
bool
ASFTagHelper::hasEmbeddedCover() const
{
    TagLib::ASF::AttributeListMap map = m_tag->attributeListMap();
    TagLib::String name = fieldName( Meta::valHasCover );
    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
        if( it->first == name )
        {
            TagLib::ASF::AttributeList coverList = it->second;
            for( TagLib::ASF::AttributeList::ConstIterator cover = coverList.begin(); cover != coverList.end(); ++cover )
            {
                if( cover->type() != TagLib::ASF::Attribute::BytesType )
                    continue;

                    ASFPicture pict( cover->toByteVector() );
                    if( ( pict.type() == ASFPicture::FrontCover ||
                          pict.type() == ASFPicture::Other ) &&
                        pict.size() > 1024 )
                    {
                        return true;
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

    ASFPicture other, front;
    bool hasFront = false, hasOther = false;
    int maxSize = 1024;

    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
        if( it->first == name )
        {
            TagLib::ASF::AttributeList coverList = it->second;
            for( TagLib::ASF::AttributeList::ConstIterator cover = coverList.begin(); cover != coverList.end(); ++cover )
            {
                if( cover->type() != TagLib::ASF::Attribute::BytesType )
                    continue;

                ASFPicture pict( cover->toByteVector() );
                if( pict.size() < maxSize )
                    continue;

                if( pict.type() == ASFPicture::FrontCover )
                {
                    front = pict;
                    maxSize = pict.size();
                    hasFront = true;
                }
                else if( pict.type() == ASFPicture::Other )
                {
                    other = pict;
                    maxSize = pict.size();
                    hasOther = true;
                }
            }
        }

    if( !hasFront && !hasOther )
        return QImage();

    //If Front and Other covers have the same size, we should use the Front one.
    if( hasFront && !hasOther )
        return front.image();
    else if( !hasFront && hasOther )
        return other.image();
    else if( front.size() >= other.size() )
        return front.image();
    //else
    return other.image();
}

bool
ASFTagHelper::setEmbeddedCover( const QImage &cover )
{
    TagLib::String name = fieldName( Meta::valHasCover );

    ASFPicture picture( cover );
    bool stored = false;

    TagLib::ASF::AttributeList coverList = m_tag->attributeListMap()[name];
    for( uint i = 0; i < coverList.size(); i++ )
    {
        if( coverList[i].type() != TagLib::ASF::Attribute::BytesType )
            continue;

        ASFPicture pict( coverList[i].toByteVector() );
        if( pict.type() == ASFPicture::FrontCover )
        {
            coverList[i] = TagLib::ASF::Attribute( picture.toByteVector() );
            stored = true;
        }
    }

    if( !stored )
        m_tag->addAttribute( name, TagLib::ASF::Attribute( picture.toByteVector() ) );

    return true;
}
#endif  //UTILITIES_BUILD
