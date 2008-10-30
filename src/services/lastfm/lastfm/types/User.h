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

#ifndef LASTFM_USER_H
#define LASTFM_USER_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/core/WeightedStringList.h>
#include <QString>
#include <QUrl>
class WsReply;


class LASTFM_TYPES_DLLEXPORT User
{
    QString m_name;
	
public:    
    User( const QString& name ) : m_name( name ), m_match( -1.0f )
    {}

    operator QString() const { return m_name; }
	
    /** You can get a WeightedStringList using Tag::getTopTags() */
	WsReply* getTopTags() const;

    /** get a QList<User> from User::list() */
    WsReply* getFriends() const;
	WsReply* getNeighbours() const;
    
    static QList<User> list( WsReply* );
    
//////
	QUrl smallImageUrl() const { return m_smallImage; }
	QUrl mediumImageUrl() const { return m_mediumImage; }
	QUrl largeImageUrl() const { return m_largeImage; }
	
    QString realName() const { return m_realName; }
    
    /** the user's profile page at www.last.fm */
    QUrl www() const;
    
	/** Returns the match between the logged in user and the user which this
	  *	object represents (if < 0.0f then not set) */
	float match() const { return m_match; }
	
private:
	QUrl m_smallImage;
	QUrl m_mediumImage;
	QUrl m_largeImage;
	
	float m_match;
    
    QString m_realName;
};



#include <lastfm/ws/WsKeys.h>
/** The authenticated user is special, as some webservices only work for him */
class LASTFM_TYPES_DLLEXPORT AuthenticatedUser : public User
{
    using User::match;
    
public:
    /** the authenticated User */
    AuthenticatedUser() : User( Ws::Username )
    {}

	/** you can only get information about the autheticated user */
	static WsReply* getInfo();       
};

#endif
