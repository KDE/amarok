/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#include "User.h"
#include "lastfm/ws/WsRequestBuilder.h"
#include "lastfm/core/CoreUrl.h"


WsReply*
User::getFriends() const
{
    return WsRequestBuilder( "user.getFriends" ).add( "user", m_name ).get();
}


WsReply*
User::getTopTags() const
{
    return WsRequestBuilder( "user.getTopTags" ).add( "user", m_name ).get();
}


WsReply* 
User::getNeighbours() const
{
	return WsRequestBuilder( "user.getNeighbours" ).add( "user", m_name ).get();
}


QList<User> //static
User::list( WsReply* r )
{
	QList<User> users;
    try
    {
        foreach (CoreDomElement e, r->lfm().children( "user" ))
        {
            User u( e["name"].text() );
            u.m_smallImage = e["image size=small"].text();
            u.m_mediumImage = e["image size=medium"].text();
            u.m_largeImage = e["image size=large"].text();
            u.m_realName = e.optional( "realname" ).text();
            users += u;
        }
    }
    catch (CoreDomElement::Exception& e)
    {
        qWarning() << e;
    }    
    return users;
}


WsReply*
AuthenticatedUser::getInfo()
{
	return WsRequestBuilder( "user.getInfo" ).get();
}


QUrl
User::www() const
{ 
	return "http://www.last.fm/user/" + CoreUrl::encode( m_name ); 
}
