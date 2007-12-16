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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "UnicornCommon.h"
#include "logger.h"
#include "Request.h"
#include "XmlRpc.h"
#include "WebService.h"
#include "DragMimeData.h"


SetTagRequest::SetTagRequest()
        : Request( TypeSetTag, "SetTag" )
        , m_mode( TAG_OVERWRITE )
{
    setOverrideCursor();
}


SetTagRequest*
SetTagRequest::append( Track track, QString const tag ) ///static
{
    SetTagRequest *request = new SetTagRequest;
    request->setType( UnicornEnums::ItemTrack );
    request->setMode( TAG_APPEND );
    request->setTag( tag );
    request->setArtist( track.artist() );
    request->setToken( track.title() );
    request->start();

    return request;
}


SetTagRequest*
SetTagRequest::append( const QMimeData* mimedata, QString const tag ) ///static
{
    SetTagRequest *request = new SetTagRequest;
    request->setType( static_cast<const DragMimeData*>(mimedata)->itemType() );
    request->setMode( TAG_APPEND );
    request->setTag( tag );

    switch (request->m_type)
    {
        case UnicornEnums::ItemArtist:
            request->setArtist( QString::fromUtf8(mimedata->data( "item/artist" )) );
            break;

        case UnicornEnums::ItemAlbum:
            request->setArtist( QString::fromUtf8(mimedata->data( "item/artist" )) );
            request->setToken( QString::fromUtf8(mimedata->data( "item/album" )) );
            break;

        case UnicornEnums::ItemTrack:
            request->setArtist( QString::fromUtf8(mimedata->data( "item/artist" )) );
            request->setToken( QString::fromUtf8(mimedata->data( "item/track" )) );
            break;

        default:
            break;
    }

    request->start();

    return request;
}


QString
SetTagRequest::title() const
{
    QString title = m_artist;

    if ( !m_album.isEmpty() )
        title += " - " + m_album;
    else if ( !m_track.isEmpty() )
        title += " - " + m_track;

    return title;
}


void
SetTagRequest::start()
{
    if ( m_username.isEmpty() )
        m_username = The::webService()->currentUsername();

    for ( int i = 0; i < m_tags.count(); i++ )
        m_tags[i] = m_tags.at( i ).trimmed();

    XmlRpc xml_rpc;
    QString const challenge = The::webService()->challengeString();

    xml_rpc << m_username
            << challenge
            << UnicornUtils::md5Digest( (The::webService()->currentPassword() + challenge).toUtf8() )
            << m_artist;

    switch ( m_type )
    {
        case UnicornEnums::ItemArtist:
            xml_rpc.setMethod( "tagArtist" );
            break;

        case UnicornEnums::ItemAlbum:
            m_album = m_token;
            xml_rpc.setMethod( "tagAlbum" );
            xml_rpc.addParameter( m_token );
            break;

        case UnicornEnums::ItemTrack:
            m_track = m_token;
            xml_rpc.setMethod( "tagTrack" );
            xml_rpc.addParameter( m_token );
            break;
    }

    xml_rpc << m_tags
            << (m_mode == TAG_OVERWRITE ? "set" : "append");

    request( xml_rpc );
}


void
SetTagRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;
    bool parsed = XmlRpc::parse( data, retVals, error );

    if ( !parsed )
    {
        LOGL( 1, error );
        //TODO mxcl show error message in debug and perhaps as part of error message to user
        setFailed( WebRequestResult_Custom, "Couldn't parse Xml response" );
    }
    else
    {
        QString response = retVals.at( 0 ).toString();
        if ( response != "OK" )
        {
            setFailed( WebRequestResult_Custom, "Tag request failed, returned: " + response );
            LOGL( 1, "Tag request failed, returned: " << response );
        }
    }
}
