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


FriendsRequest::FriendsRequest() 
        : Request( TypeFriends, "Friends" )
{}


void
FriendsRequest::start()
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

    get( "/1.0/user/" + UnicornUtils::urlEncodeItem( m_username ) + "/friends.xml?imagesize=" + size ); //?showtracks=1" );
}


void
FriendsRequest::success( QByteArray data )
{
    QDomDocument document;
    if (!document.setContent( data ))
    {
        //TODO mxcl error
    }

    if (document.elementsByTagName( "friends" ).length() == 0)
        return;

    QString user = document.elementsByTagName( "friends" ).item( 0 ).attributes().namedItem( "user" ).nodeValue();
    QDomNodeList values = document.elementsByTagName( "user" );

    for ( int i = 0; i < values.count(); i++ )
    {
        QString image;
        QDomNode imageNode = values.item( i ).namedItem( "image" );
        if ( !imageNode.isNull() )
            image = values.item( i ).namedItem( "image" ).toElement().text();

        QString user = values.item( i ).attributes().namedItem( "username" ).nodeValue();
        m_avatars.insert( user, image );

        #if 0
        //NOTE I'm keeping this code so we don't forget this service provides this data
        UserMetaData umd;
        QDomNode image = values.item( i ).namedItem( "image" );
        umd.setName( values.item( i ).attributes().namedItem( "username" ).nodeValue() );

        if ( !image.isNull() )
            umd.setImage( QUrl( image.toElement().text() ) );

        // recent track
        QDomNode lasttrack = values.item( i ).namedItem( "lasttrack" );
        if ( !lasttrack.isNull() )
        {
            QDomNode recentArtist = lasttrack.namedItem( "artist" );
            QDomNode recentTrack = lasttrack.namedItem( "name" );
            QDomNode recentDate = lasttrack.namedItem( "date" );

            if ( !recentArtist.isNull() && !recentTrack.isNull() && !recentDate.isNull() )
            {
                umd.setLastActivity( recentDate.toElement().text() );
                umd.setRecentTrack( QStringList( QString( "%1 - %2" )
                                                    .arg( recentArtist.toElement().text() )
                                                    .arg( recentTrack.toElement().text() ) ) );
            }
        }

        m_metaDatas << umd;
        #endif
        
        m_usernames << values.item( i ).attributes().namedItem( "username" ).nodeValue();
    }
    
    m_usernames = UnicornUtils::sortCaseInsensitively( m_usernames );
}
