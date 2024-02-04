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

#include "APETagHelper.h"

#include "StringHelper.h"

using namespace Meta::Tag;

APETagHelper::APETagHelper( TagLib::Tag *tag, TagLib::APE::Tag *apeTag, Amarok::FileType fileType )
            : TagHelper( tag, fileType )
            , m_tag( apeTag )
{
    m_fieldMap.insert( Meta::valAlbumArtist, TagLib::String( "ALBUM ARTIST" ) );
    m_fieldMap.insert( Meta::valBpm,         TagLib::String( "BPM" ) );
    m_fieldMap.insert( Meta::valCompilation, TagLib::String( "COMPILATION" ) );
    m_fieldMap.insert( Meta::valComposer,    TagLib::String( "Composer" ) );
    m_fieldMap.insert( Meta::valDiscNr,      TagLib::String( "DISC" ) );
    m_fieldMap.insert( Meta::valPlaycount,   TagLib::String( "FMPS_PLAYCOUNT" ) );
    m_fieldMap.insert( Meta::valRating,      TagLib::String( "FMPS_RATING" ) );
    m_fieldMap.insert( Meta::valScore,       TagLib::String( "FMPS_RATING_AMAROK_SCORE" ) );
    m_fieldMap.insert( Meta::valLyrics,      TagLib::String( "UNSYNCED LYRICS" ) );

    m_uidFieldMap.insert( UIDAFT,            TagLib::String( "Amarok 2 AFTv1 - amarok.kde.org" ) );
}

Meta::FieldHash
APETagHelper::tags() const
{
    Meta::FieldHash data = TagHelper::tags();

    TagLib::APE::ItemListMap map =  m_tag->itemListMap();
    for( TagLib::APE::ItemListMap::ConstIterator it = map.begin(); it != map.end(); ++it )
    {
        qint64 field;
        QString value = TStringToQString( it->second.toString() );
        if( ( field = fieldName( it->first ) ) )
        {
            if( field == Meta::valRating )
                data.insert( field, qRound( value.toFloat() * 10.0 ) );
            else if( field == Meta::valScore )
                data.insert( field, value.toFloat() * 100.0 );
            else if( field == Meta::valDiscNr ) {
                int disc;
                disc = splitDiscNr( value ).first;
                data.insert( field, disc );
            }
            else
                data.insert( field, value );
        }
        else if( it->first == uidFieldName( UIDAFT ) && isValidUID( value, UIDAFT ) )
            data.insert( Meta::valUniqueId, value );
    }

    return data;
}

bool
APETagHelper::setTags( const Meta::FieldHash &changes )
{
    bool modified = TagHelper::setTags( changes );

    foreach( const qint64 key, changes.keys() )
    {
        QVariant value = changes.value( key );
        TagLib::String field = fieldName( key );

        if( !field.isEmpty() )
        {
            if( key == Meta::valRating )
                m_tag->addValue( field, Qt4QStringToTString( QString::number( value.toFloat() / 10.0 ) ) );
            else if( key == Meta::valScore )
                m_tag->addValue( field, Qt4QStringToTString( QString::number( value.toFloat() / 100.0 ) ) );
            else
                m_tag->addValue( field, Qt4QStringToTString( value.toString() ) );

            modified = true;
        }
        else if( key == Meta::valUniqueId )
        {
            QPair < UIDType, QString > uidPair = splitUID( value.toString() );
            if( uidPair.first == UIDInvalid )
                continue;

            m_tag->addValue( uidFieldName( uidPair.first ), Qt4QStringToTString( uidPair.second ) );
            modified = true;
        }
    }

    return modified;
}

TagLib::ByteVector
APETagHelper::render() const
{
    return m_tag->render();
}
