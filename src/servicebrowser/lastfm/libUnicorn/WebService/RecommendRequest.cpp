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

#include "UnicornCommon.h"
#include "logger.h"
#include "Request.h"
#include "WebService.h"
#include "XmlRpc.h"
#include "Settings.h"
#include "DragMimeData.h"

RecommendRequest::RecommendRequest()
        : Request( TypeRecommend, "Recommend" )
        , m_type( UnicornEnums::ItemArtist )
{
    setOverrideCursor();
}


RecommendRequest::RecommendRequest( const QMimeData* mimedata, QString username )
        : Request( TypeRecommend, "Recommend" )
        , m_type( static_cast<const DragMimeData*>(mimedata)->itemType() )
{
    setOverrideCursor();
    setTargetUsername( username );

    switch (m_type)
    {
        case UnicornEnums::ItemArtist:
            setArtist( QString::fromUtf8(mimedata->data( "item/artist" )) );
            break;
    
        case UnicornEnums::ItemAlbum:
            setToken( QString::fromUtf8(mimedata->data( "item/album" )) );
            break;
    
        case UnicornEnums::ItemTrack:
            setArtist( QString::fromUtf8(mimedata->data( "item/artist" )) );
            setToken( QString::fromUtf8(mimedata->data( "item/track" )) );
            break;
            
        default:
            break;
    }
}


RecommendRequest::RecommendRequest( Track track, QString username )
        : Request( TypeRecommend, "Recommend" )
        , m_type( UnicornEnums::ItemTrack )
{
    setArtist( track.artist() );
    setTargetUsername( username );
    setToken( track.title() );
    setOverrideCursor();
}


void
RecommendRequest::start()
{
    XmlRpc xml_rpc;
    QString const challenge = The::webService()->challengeString();
    QString recommendation_type;

    xml_rpc << The::webService()->currentUsername()
            << challenge
            << UnicornUtils::md5Digest( QString( The::webService()->currentPassword() + challenge ).toUtf8() )
            << m_artist;
    
    xml_rpc.setMethod( "recommendItem" );
    
    switch (m_type)
    {
        case UnicornEnums::ItemArtist:
            xml_rpc.addParameter( "" );
            xml_rpc.addParameter( "artist" );
            break;
            
        case UnicornEnums::ItemAlbum:
            m_album = m_token;                      
            xml_rpc.addParameter( m_token );
            xml_rpc.addParameter( "album" );
            break;

        case UnicornEnums::ItemTrack:
            m_track = m_token;
            xml_rpc.addParameter( m_token );
            xml_rpc.addParameter( "track" );
            break;
    }
    
    xml_rpc << m_target_username
            << m_message
            << m_language;
    
    request( xml_rpc );
}
