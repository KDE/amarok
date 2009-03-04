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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "WsRequestBuilder.h"
#include "WsKeys.h"
#include "WsReply.h"
#include "WsAccessManager.h"
#include <QCoreApplication>
#include <QEventLoop>

WsAccessManager* WsRequestBuilder::nam = 0;


WsRequestBuilder::WsRequestBuilder( const QString& method )
    : request_method( GET )
{
    if (!nam) nam = new WsAccessManager( qApp );
    
    params.add( "method", method );
}


WsReply*
WsRequestBuilder::start()
{
#ifdef WIN32
    // we don't do this on unix because it was inexplicably crashing! -- Qt 4.4.3
    QUrl url( !qApp->arguments().contains( "--debug")
            ? "http://ws.audioscrobbler.com/2.0/"
            : "http://ws.staging.audioscrobbler.com/2.0/" );
#else
    QUrl url( "http://ws.audioscrobbler.com/2.0/" );    
#endif

	//Only GET requests should include query parameters
	if( request_method == GET )
		url.setQueryItems( params );
	
    QNetworkRequest request( url );
    request.setRawHeader( "User-Agent", Ws::UserAgent );

    switch (request_method)
    {
        case GET:
            return new WsReply( nam->get( request ) );

        case POST: 
		{
			//Build encoded query for use in the POST Content
			QByteArray query;
            typedef QPair<QString, QString> Pair;
            QList<Pair> params = this->params;
			foreach (Pair param, params)
			{
				query += QUrl::toPercentEncoding( param.first, "!$&'()*+,;=:@/?" )
					  + "="
					  + QUrl::toPercentEncoding( param.second, "!$&'()*+,;=:@/?" )
					  + "&";
			}
			return new WsReply( nam->post( request, query ) );
		}
    }
	return 0;
}

void 
WsRequestBuilder::setWAM( WsAccessManager* wam )
{
    nam = wam;
}

