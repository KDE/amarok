/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
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

#include "Tuner.h"
#include "Playlist.h"
#include <QBuffer>
#include <QDebug>
#include "../ws/WsRequestBuilder.h"
#include "../ws/WsReply.h"
#include <QtXml>


//TODO discovery mode
//TODO skips left
//TODO multiple locations for the same track
//TODO set rtp flag in getPlaylist (whether user is scrobbling this radio session or not)


Tuner::Tuner( const RadioStation& station )
     : m_retry_counter( 0 )
{
    qDebug() << "radio.tune:" << station.url();
    
    WsReply* reply = WsRequestBuilder( "radio.tune" )
			.add( "station", station.url() )
			.post();
	connect( reply, SIGNAL(finished( WsReply* )), SLOT(onTuneReturn( WsReply* )) );
}


void
Tuner::onTuneReturn( WsReply* reply )
{
	if (reply->error() != Ws::NoError) {
		emit error( reply->error() );
		return;
	}

	try {
		m_stationName = reply->lfm()["station"]["name"].text();
		emit stationName( m_stationName );
	}
	catch (CoreException&)
	{}
	
	fetchFiveMoreTracks();
}


void
Tuner::fetchFiveMoreTracks()
{
    WsReply* reply = WsRequestBuilder( "radio.getPlaylist" ).add( "rtp", "1" ).get();
	connect( reply, SIGNAL(finished( WsReply* )), SLOT(onGetPlaylistReturn( WsReply* )) );
}


bool
Tuner::tryAgain()
{
	if (++m_retry_counter > 5)
		return false;
	fetchFiveMoreTracks();
	return true;
}


void
Tuner::onGetPlaylistReturn( WsReply* reply )
{
	switch (reply->error())
	{
		case Ws::NoError:
			break;

		case Ws::TryAgainLater:
			if (!tryAgain())
				emit error( Ws::TryAgainLater );
			return;

		default:
			emit error( reply->error() );
			return;
	}
			
	Playlist p( reply );

	if (p.tracks().isEmpty())
	{
		// sometimes the recs service craps out and gives us a blank playlist
		
		if (!tryAgain())
		{
			// an empty playlist is a bug, if there is no content
			// NotEnoughContent should have been returned with the WsReply
			emit error( Ws::MalformedResponse );
		}
	}
	else {
		m_retry_counter = 0;
		emit tracks( p.tracks() );
	}
}
