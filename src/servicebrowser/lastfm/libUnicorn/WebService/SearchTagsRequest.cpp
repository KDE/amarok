/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "logger.h"
#include "Request.h"
#include "UnicornCommon.h"


SearchTagRequest::SearchTagRequest()
        : TagsRequest( TypeSearchTag, "SearchTags" )
{}

void
SearchTagRequest::start()
{
    get( "/1.0/tag/" + UnicornUtils::urlEncodeItem( m_tag ) + "/search.xml?showtop10=1" );
}
    
void
SearchTagRequest::success( QByteArray data )
{
    QDomDocument xml;
    xml.setContent( data );
    
    QList<QStringList> topArtistList;
    Q_UNUSED( topArtistList );

    QDomNodeList values = xml.elementsByTagName( "tag" );
    for ( int i = 0; i < values.count(); i++ )
    {
        QDomNode item = values.item( i ).namedItem( "name" );
        QDomNode match = values.item( i ).namedItem( "match" );
        int match_pc = (int)match.toElement().text().toFloat() * 100;
        
        m_tags += WeightedString::weighted( item.toElement().text(), match_pc );
    
        #if 0
        QStringList topArtists;
        QDomElement topartistsNode = values.item( i ).firstChildElement( "topartists" );
        if ( !topartistsNode.isNull() )
        {
            QDomNodeList artistsNode = topartistsNode.elementsByTagName( "artist" );
            for ( int x = 0; x < artistsNode.count(); x++ )
            {
                topArtists << artistsNode.item( x ).toElement().text();
            }
        }

        topArtistList.append( topArtists );
        #endif
    }
}
