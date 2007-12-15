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
#include "WebService.h"


NeighboursRequest::NeighboursRequest()
        : Request( TypeNeighbours, "Neighbours" )
{}

void
NeighboursRequest::start()
{
    if ( m_username.isEmpty() )
        m_username = The::webService()->currentUsername();

    QString size;
    switch( imageSize() )
    {
        case SIZE_PAGE:
            size = "page";
            break;

        case SIZE_LARGE:
            size = "large";
            break;

        case SIZE_MEDIUM:
            size = "medium";
            break;

        default:
            size = "small";
            break;
    }

    get( "/1.0/user/" + UnicornUtils::urlEncodeItem( m_username ) + "/neighbours.xml?imagesize=" + size );
}

void
NeighboursRequest::success( QByteArray data )
{
    Q_DEBUG_BLOCK;

    QDomDocument xml;
    xml.setContent( data );

    QDomNodeList values = xml.elementsByTagName( "user" );
    for ( int i = 0; i < values.count(); i++ )
    {
        QString name = values.item( i ).attributes().namedItem( "username" ).nodeValue();
        int match = (int)values.item( i ).namedItem( "match" ).toElement().text().toFloat();

        m_usernames += WeightedString::weighted( name, match );

        QString image;
        QDomNode imageNode = values.item( i ).namedItem( "image" );
        if ( !imageNode.isNull() )
            image = values.item( i ).namedItem( "image" ).toElement().text();

        m_avatars.insert( name, image );
    }
}
