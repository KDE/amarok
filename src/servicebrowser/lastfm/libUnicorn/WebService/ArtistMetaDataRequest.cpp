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
#include "Settings.h"
#include "UnicornCommon.h"
#include "XmlRpc.h"


ArtistMetaDataRequest::ArtistMetaDataRequest()
        : Request( TypeArtistMetaData, "ArtistMetaData" )
{}

void
ArtistMetaDataRequest::start() 
{
    Q_DEBUG_BLOCK << m_artist;

    XmlRpc rpc;
    rpc << m_artist;
    rpc << language();
    
    rpc.setMethod( "artistMetadata" );
    rpc.setUseCache( true );
    
    request( rpc );
}

void
ArtistMetaDataRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;
    bool parsed = XmlRpc::parse( data, retVals, error );

    if ( !parsed )
    {
        LOGL( 1, error );
        setFailed( WebRequestResult_Custom, "Couldn't parse response" );
        return;
    }

    // There was previously no proper fault struct being returned so we're
    // checking for error (item not found etc) by seeing whether the returned
    // param is a map (struct) or not.
    if ( retVals.at( 0 ).type() != QVariant::Map )
    {
        LOGL( 2, "Result wasn't a <struct>, artist not found?" );
        setFailed( WebRequestResult_Custom, "Result wasn't a <struct>, artist not found?" );
        return;
    }
    
    QMap<QString, QVariant> map = retVals.at( 0 ).toMap();

    if ( map.contains( "faultCode" ) )
    {
        QString faultString = map.value( "faultString" ).toString();
        LOGL( 2, faultString );
        setFailed( WebRequestResult_Custom, faultString );
        return;
    }

    m_meta_data.setArtist( map.value( "artistName" ).toString() );
    m_meta_data.setArtistPageUrl( map.value( "artistPageUrl" ).toString() );

    QStringList tags;
    foreach( QVariant val, map.value( "artistTags" ).toList() )
    {
        tags << val.toString();
    }
    m_meta_data.setArtistTags( tags );
    m_meta_data.setNumListeners( map.value( "numListeners" ).toInt() );
    m_meta_data.setNumPlays( map.value( "numPlays" ).toInt() );
    m_meta_data.setArtistPicUrl( map.value( "picture" ).toString() );

    QStringList similar;
    foreach( QVariant val, map.value( "similar" ).toList() )
    {
        similar << val.toString();
    }
    m_meta_data.setSimilarArtists( similar );

    QStringList fans;
    foreach( QVariant val, map.value( "topFans" ).toList() )
    {
        fans << val.toString();
    }
    m_meta_data.setTopFans( fans );

    m_meta_data.setWikiPageUrl( map.value( "wikiPageUrl" ).toString() );

    QString wiki = map.value( "wikiText" ).toString();
    UnicornUtils::stripBBCode( wiki );
    m_meta_data.setWiki( wiki );
}
