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
#include "XmlRpc.h"


TrackMetaDataRequest::TrackMetaDataRequest()
        : Request( TypeTrackMetaData, "TrackMetaData" )
{}

void
TrackMetaDataRequest::start()
{
    XmlRpc xmlrpc;
    xmlrpc << m_track.artist();
    xmlrpc << m_track.track();
    xmlrpc << m_track.album();
    xmlrpc << language();
    
    xmlrpc.setMethod( "trackMetadata" );
    xmlrpc.setUseCache( true );

    request( xmlrpc );
}

void
TrackMetaDataRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;

    if (!XmlRpc::parse( data, retVals, error )) {
        setFailed( WebRequestResult_Custom, error );
        return;
    }

    // There was previously no proper fault struct being returned so we're
    // checking for error (item not found etc) by seeing whether the returned
    // param is a map (struct) or not.
    if (retVals.at( 0 ).type() != QVariant::Map) {
        setFailed( WebRequestResult_Custom, "Result wasn't a <struct>, track not found?" );
        return;
    }

    QMap<QString, QVariant> map = retVals.at( 0 ).toMap();

    if (map.contains( "faultCode" )) {
        QString faultString = map.value( "faultString" ).toString();
        setFailed( WebRequestResult_Custom, faultString );
        return;
    }
    
    m_track = TrackInfo();

    m_track.setArtist( map.value( "artistName" ).toString() );
    m_track.setAlbumPicUrl( map.value( "albumCover" ).toString() );
    m_track.setLabel( map.value( "albumLabel" ).toString() );
    m_track.setLabelUrl( map.value( "albumLabelUrl" ).toString() );
    m_track.setAlbum( map.value( "albumName" ).toString() );
    m_track.setNumTracks( map.value( "albumNumTracks" ).toInt() );
    QString dateStr = map.value( "albumReleaseDate" ).toString();
    m_track.setReleaseDate( QDate::fromString( dateStr, Qt::ISODate ) );
    m_track.setAlbumPageUrl( map.value( "albumUrl" ).toString() );
    m_track.setBuyTrackUrl( map.value( "trackBuyURL" ).toString() );
    m_track.setBuyTrackString( map.value( "buyTrackString" ).toString() );
    m_track.setBuyAlbumUrl( map.value( "buyAlbumURL" ).toString() );
    m_track.setBuyAlbumString( map.value( "buyAlbumString" ).toString() );

    QStringList tags;
    foreach (QVariant v, map.value( "trackTags" ).toList())
        tags << v.toString();
    
    m_track.setTrackTags( tags );

    m_track.setTrack( map.value( "trackTitle" ).toString() );
    m_track.setTrackPageUrl( map.value( "trackUrl" ).toString() );
}
