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

#include "Request.h"
#include "Settings.h"
#include "XmlRpc.h"


TrackToIdRequest::TrackToIdRequest( Track track ) :
        Request( TypeTrackMetaData, "TrackMetaData" ),
        m_track( track )
{}

void
TrackToIdRequest::start()
{
    XmlRpc xmlrpc;
    xmlrpc << m_track.artist();
    xmlrpc << m_track.title();
    
    xmlrpc.setMethod( "trackToId" );
    
    request( xmlrpc );
}

void
TrackToIdRequest::success( QByteArray data )
{
    QList<QVariant> values;
    QString error;

    if (!XmlRpc::parse( data, values, error )) {
        setFailed( WebRequestResult_Custom, error );
        return;
    }
    
    QMap<QString, QVariant> map = values.value( 0 ).toMap();
    
//     qDebug() << map;
    
    m_id = map["trackID"].toInt();
    m_isStreamable = map["isLastfm"].toBool();
}
