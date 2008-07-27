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

#include "Request.h"
#include "WebService.h"
#include "Settings.h"

GetXspfPlaylistRequest::GetXspfPlaylistRequest( const QString& session,
                                                const QString& basePath,
                                                const QString& version,
                                                bool discovery ) :
    Request( TypeGetXspfPlaylist, "GetXspfPlaylist" ),
    m_session( session ),
    m_basePath( basePath ),
    m_version( version ),
    m_discovery( discovery )
{

}

void
GetXspfPlaylistRequest::start()
{
    QString path = m_basePath + "/xspf.php?" +
        "sk=" + m_session + 
        "&discovery=" + QString::number( static_cast<int>( m_discovery ) )  +
        "&desktop=" + m_version;

    get( path );
}


bool
GetXspfPlaylistRequest::headerReceived( const QHttpResponseHeader& /*header*/ )
{
    if ( responseHeaderCode() == 401 )
    {
        setFailed( Playlist_InvalidSession,
            tr( "Invalid session. Please re-handshake." ) );
    }
    else if ( responseHeaderCode() == 503 )
    {
        setFailed( Playlist_RecSysDown,
            tr( "Sorry, the playlist service is not responding.\n"
                "Please try again later." ) );
    }
    return true;
}


void
GetXspfPlaylistRequest::success( QByteArray /*data*/ )
{
    ///@see RadioPlaylist.cpp
}
