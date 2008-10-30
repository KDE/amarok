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

#include "Artist.h"
#include "User.h"
#include "../core/CoreUrl.h"
#include "../ws/WsRequestBuilder.h"


WsReply* 
Artist::share( const User& user, const QString& message )
{
    return WsRequestBuilder( "artist.share" )
        .add( "recipient", user )
        .add( "artist", m_name )
        .addIfNotEmpty( "message", message )
        //TODO this must be post! you're testing here
        .get();
}


QUrl 
Artist::www() const
{
	return "http://www.last.fm/music/" + CoreUrl::encode( m_name );
}

WsReply* 
Artist::getInfo() const
{
	return WsRequestBuilder( "artist.getInfo" ).add( "artist", m_name ).get();
}

WsReply* 
Artist::getTags() const
{
	return WsRequestBuilder( "artist.getTags" ).add( "artist", m_name ).get();
}


WsReply* 
Artist::getSimilar() const
{
	return WsRequestBuilder( "artist.getSimilar" ).add( "artist", m_name ).get();
}


WsReply* 
Artist::search() const
{
	return WsRequestBuilder( "artist.search" ).add( "artist", m_name ).get();
}


WeightedStringList /* static */
Artist::getSimilar( WsReply* r )
{
	WeightedStringList artists;
	try
	{
		foreach (CoreDomElement e, r->lfm().children( "artist" ))
		{
			QString artistName = e["name"].text();
			float match = e["match"].text().toFloat();
			artists.push_back( WeightedString( artistName, match ));
		}
		
	}
	catch( CoreDomElement::Exception& e)
	{
		qWarning() << e;
	}
	return artists;
}


QStringList /* static */
Artist::search( WsReply* r )
{
	QStringList results;
	try
	{
		foreach( CoreDomElement e, r->lfm().children( "artist" ))
		{
			results += e["name"].text();
		}
	}
	catch( CoreDomElement::Exception& e)
	{
		qWarning() << e;
	}
	return results;
}


WsReply*
Artist::addTags( const QStringList& tags ) const
{
    if (tags.isEmpty())
        return 0;
    
    QString comma_separated_tags;
    foreach( QString const tag, tags)
        comma_separated_tags += tag;
    
    return WsRequestBuilder( "artist.addTags" )
            .add( "artist", m_name )
            .add( "tags", comma_separated_tags )
            .post();
}
