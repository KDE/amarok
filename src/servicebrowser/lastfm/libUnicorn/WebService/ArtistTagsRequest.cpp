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


ArtistTagsRequest::ArtistTagsRequest( QString artist )
        : TagsRequest( TypeArtistTags, "ArtistTags" )
        , m_artist( artist )
{}

void
ArtistTagsRequest::start()
{
    get( "/1.0/artist/" + UnicornUtils::urlEncodeItem( m_artist ) + "/toptags.xml" );
}

void
ArtistTagsRequest::success( QByteArray data )
{
    QDomDocument xml;
    xml.setContent( data );
    
//     qDebug() << data;
    
    QDomNodeList values = xml.elementsByTagName( "tag" );
    for ( int i = 0; i < values.count(); i++ )
        m_tags += WeightedString( values.item( i ).namedItem( "name" ).toElement().text() );
}
