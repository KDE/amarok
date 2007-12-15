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


TopTagsRequest::TopTagsRequest()
        : TagsRequest( TypeTopTags, "TopTags" )
{}

void
TopTagsRequest::success( QByteArray data )
{
    QDomDocument xml;
    xml.setContent( data );
    
    QDomNodeList values = xml.elementsByTagName( "tag" );
    for (int i = 0; i < values.count(); i++)
    {
        QDomNamedNodeMap attrs = values.item( i ).attributes(); 
    
        QString name = attrs.namedItem( "name" ).nodeValue();
        int count = attrs.namedItem( "count" ).nodeValue().toInt();

        m_tags += WeightedString::counted( name, count );
    }
}

void
TopTagsRequest::start()
{
    get( "/1.0/tag/toptags.xml" );
}
