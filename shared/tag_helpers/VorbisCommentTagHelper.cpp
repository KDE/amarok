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


#include "VorbisCommentTagHelper.h"

#include "StringHelper.h"

#include <taglib/flacpicture.h>

#ifndef UTILITIES_BUILD
    #include <QBuffer>
#endif  //UTILITIES_BUILD

using namespace Meta::Tag;

VorbisCommentTagHelper::VorbisCommentTagHelper( TagLib::Tag *tag, TagLib::Ogg::XiphComment *commentsTag, Amarok::FileType fileType )
                      : TagHelper( tag, fileType )
                      , m_tag( commentsTag )
                      , m_flacFile( 0 )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "ALBUMARTIST" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "BPM" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "COMPILATION" ) );
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "COMPOSER" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "DISCNUMBER" ) );
    m_fieldMap.insert( Meta::valHasCover,    TagLib::String( "COVERART" ) ); // non-standard but frequently used
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "FMPS_PLAYCOUNT" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "FMPS_RATING" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "FMPS_RATING_AMAROK_SCORE" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "AMAROK 2 AFTV1 - AMAROK.KDE.ORG" ) );
    m_uidFieldMap.insert( UIDMusicBrainz,    TagLib::String( "MUSICBRAINZ_TRACKID" ) );
}

VorbisCommentTagHelper::VorbisCommentTagHelper( TagLib::Tag *tag, TagLib::Ogg::XiphComment *commentsTag, TagLib::FLAC::File *file, Amarok::FileType fileType )
                      : TagHelper( tag, fileType )
                      , m_tag( commentsTag )
                      , m_flacFile( file )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "ALBUMARTIST" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "BPM" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "COMPILATION" ) );
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "COMPOSER" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "DISCNUMBER" ) );
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "FMPS_PLAYCOUNT" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "FMPS_RATING" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "FMPS_RATING_AMAROK_SCORE" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "AMAROK 2 AFTV1 - AMAROK.KDE.ORG" ) );
    m_uidFieldMap.insert( UIDMusicBrainz,    TagLib::String( "MUSICBRAINZ_TRACKID" ) );
}

Meta::FieldHash
VorbisCommentTagHelper::tags() const
{
    Meta::FieldHash data = TagHelper::tags();

    TagLib::Ogg::FieldListMap map = m_tag->fieldListMap();
    for( TagLib::Ogg::FieldListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        qint64 field;
        QString value = TStringToQString( it->second.toString( '\n' ) );

        if( ( field = fieldName( it->first ) ) )
        {
            if( field == Meta::valDiscNr )
            {
                int disc;
                if( ( disc = splitDiscNr( value ).first ) )
                    data.insert( field, disc );
            }
            else if( field == Meta::valRating )
                data.insert( field, qRound( value.toFloat() * 10.0 ) );
            else if( field == Meta::valScore )
                data.insert( field, value.toFloat() * 100.0 );
            else if( field == Meta::valHasCover )
                data.insert( Meta::valHasCover, true );
            else
                data.insert( field, value );
        }
        else if( it->first == uidFieldName( UIDAFT ) && isValidUID( value, UIDAFT ) )
            data.insert( Meta::valUniqueId, value );
        else if( it->first == uidFieldName( UIDMusicBrainz ) && isValidUID( value, UIDMusicBrainz ) )
        {
            if( !data.contains( Meta::valUniqueId ) ) // we prefere AFT uids
                data.insert( Meta::valUniqueId, value.prepend( "mb-" ) );
        }
    }

    if( m_flacFile )
    {
        const TagLib::List<TagLib::FLAC::Picture*> picturelist = m_flacFile->pictureList();
        for( TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++ )
        {
            TagLib::FLAC::Picture* picture = *it;

            if( ( picture->type() == TagLib::FLAC::Picture::FrontCover ||
                picture->type() == TagLib::FLAC::Picture::Other ) &&
                picture->data().size() > 1024 ) // must be at least 1kb
            {
                data.insert( Meta::valHasCover, true );
                break;
            }
        }
    }

    return data;
}

bool
VorbisCommentTagHelper::setTags( const Meta::FieldHash &changes )
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
            else if( key == Meta::valRating )
                m_tag->addField( field, Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) ) );
            else if( key == Meta::valScore )
                m_tag->addField( field, Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) ) );
            else
                m_tag->addField( field, Qt4QStringToTString( value.toString() ) );

            modified = true;
        }
        else if( key == Meta::valUniqueId )
        {
            QPair < UIDType, QString > uidPair = splitUID( value.toString() );
            if( uidPair.first == UIDInvalid )
                continue;

            m_tag->addField( uidFieldName( uidPair.first ), Qt4QStringToTString( uidPair.second ) );
            modified = true;
        }
    }

    return modified;
}

TagLib::ByteVector
VorbisCommentTagHelper::render() const
{
    return m_tag->render();
}

#ifndef UTILITIES_BUILD
bool
VorbisCommentTagHelper::hasEmbeddedCover() const
{
    if( m_flacFile )
    {
        const TagLib::List<TagLib::FLAC::Picture*> picturelist = m_flacFile->pictureList();
        for( TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++ )
        {
            const TagLib::FLAC::Picture *picture = *it;

            if( ( picture->type() == TagLib::FLAC::Picture::FrontCover ||
                picture->type() == TagLib::FLAC::Picture::Other ) &&
                picture->data().size() > 1024 ) // must be at least 1kb
            {
                return true;
            }
        }
    }
    else if( !fieldName( Meta::valHasCover ).isEmpty() )
    {
        TagLib::ByteVector field = fieldName( Meta::valHasCover ).toCString();

        return m_tag->fieldListMap().contains( field );
    }

    return false;
}

QImage
VorbisCommentTagHelper::embeddedCover() const
{
    QImage cover;

    if( m_flacFile )
    {
        QImage otherCover;

        const TagLib::List<TagLib::FLAC::Picture*> picturelist = m_flacFile->pictureList();
        for( TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++ )
        {
            const TagLib::FLAC::Picture *picture = *it;

            if( ( picture->type() == TagLib::FLAC::Picture::FrontCover ||
                picture->type() == TagLib::FLAC::Picture::Other ) &&
                picture->data().size() > 1024 ) // must be at least 1kb
            {
                QByteArray image_data( picture->data().data(), picture->data().size() );

                if( picture->type() == TagLib::FLAC::Picture::FrontCover )
                {
                    cover.loadFromData( image_data );
                    break;
                }
                else if( otherCover.isNull() )
                {
                    otherCover.loadFromData( image_data );
                }
            }
        }

        if( cover.isNull() && !otherCover.isNull() )
            cover = otherCover;
    }
    else if( !fieldName( Meta::valHasCover ).isEmpty() )
    {
        TagLib::ByteVector field = fieldName( Meta::valHasCover ).toCString();

        if( !m_tag->fieldListMap().contains( field ) )
            return cover;

        TagLib::StringList coverList = m_tag->fieldListMap()[field];

        for( uint i=0; i<coverList.size(); i++ )
        {
            QByteArray image_data_b64( coverList[i].toCString() );
            QByteArray image_data = QByteArray::fromBase64(image_data_b64);

            if( image_data.size() <= 1024 ) // must be at least 1kb
                continue;

            bool success = false;

            success = cover.loadFromData( image_data );
            if( !success )
                success = cover.loadFromData( image_data_b64 );

            if( success )
                break;
        }
    }

    return cover;
}

bool
VorbisCommentTagHelper::setEmbeddedCover( const QImage &cover )
{
// NOTE since the COVERART tag is not standardized and a proper way has been defined recently [1],
// we should wait until taglib provides an implementation.
// [1] http://wiki.xiph.org/index.php/VorbisComment#Cover_art

    if( m_flacFile )
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

        // back up covers
        TagLib::List<TagLib::FLAC::Picture*> backedUpPictures;
        const TagLib::List<TagLib::FLAC::Picture*> picturelist = m_flacFile->pictureList();
        for( TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++ )
        {
            const TagLib::FLAC::Picture *picture = *it;

            TagLib::FLAC::Picture *backedUpPicture = new TagLib::FLAC::Picture();
            backedUpPicture->setData( picture->data() );
            backedUpPicture->setMimeType( picture->mimeType() );
            backedUpPicture->setType( picture->type() );
            backedUpPicture->setDescription( picture->description() );

            backedUpPictures.append( backedUpPicture );
        }

        // remove all covers
        m_flacFile->removePictures();

        // add new cover
        TagLib::FLAC::Picture *newPicture = new TagLib::FLAC::Picture();
        newPicture->setData( TagLib::ByteVector( bytes.data(), bytes.size() ) );
        newPicture->setMimeType( "image/jpeg" );
        newPicture->setType( TagLib::FLAC::Picture::FrontCover );
        m_flacFile->addPicture( newPicture );

        // re-add all the backed-up covers except the front cover
        for( TagLib::List<TagLib::FLAC::Picture*>::Iterator it = backedUpPictures.begin(); it != backedUpPictures.end(); it++ )
        {
            TagLib::FLAC::Picture *picture = *it;

            if( picture->type() != TagLib::FLAC::Picture::FrontCover )
            {
                m_flacFile->addPicture( picture );
            }
        }

        return true;
    }

    return false;
}
#endif  //UTILITIES_BUILD
