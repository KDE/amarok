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

#include <QUrl>

#include "CachedHttp.h"
#include "logger.h"
#include "Request.h"
#include "Settings.h"
#include "WebService.h"
#include "UnicornCommon.h"


Handshake::Handshake()
        : Request( TypeHandshake, "Handshake" ),
          m_isSubscriber( false ),
          m_permitBootstrap( false )
{}


void
Handshake::start()
{
  #ifdef WIN32
    static const char *PLATFORM = "win32";
  #elif defined Q_WS_X11
    static const char *PLATFORM = "linux";
  #elif defined Q_WS_MAC
    static const char *PLATFORM = "mac";
  #else
    static const char *PLATFORM = "unknown";
  #endif

    QString path = "/radio/handshake.php"
            "?version=" + m_version +
            "&platform=" + PLATFORM +
            "&platformversion=" + QString( QUrl::toPercentEncoding( UnicornUtils::getOSVersion() ) ) +
            "&username=" + QString( QUrl::toPercentEncoding( m_username ) ) +
            "&passwordmd5=" + m_password +
            "&language=" + language();

  #ifdef WIN32
    // this, requested by RJ
    QString player = UnicornUtils::findDefaultPlayer();
    if (player.isEmpty())
        player = "unknown";
    player.remove( ".exe", Qt::CaseInsensitive );

    path += "&player=" + QUrl::toPercentEncoding( player.replace( ' ', '_' ) );
  #endif

    get( path );
}


void
Handshake::success( QByteArray data )
{
    Q_DEBUG_BLOCK << data;

    QString const result( data );

    LOGL( 3, "Handshake response:\n" << result );

    m_session = parameter( "session", result );
    m_baseHost = parameter( "base_url", result );
    m_basePath = parameter( "base_path", result );
    m_streamUrl.setUrl( parameter( "stream_url", result ) );
    m_isSubscriber = parameter( "subscriber", result ) == "1";
    m_message = parameter( "info_message", result );
    m_permitBootstrap = parameter( "permit_bootstrap", result ) == "1";
    bool const is_banned = parameter( "banned", result ) == "1";
    m_fingerprintUploadUrl = parameter( "fingerprint_upload_url", result );


    if ( m_session.toLower() == "failed" )
    {
        LOGL( 1, "Radio handshake failed: session == 'failed'" );

        QString msg = parameter( "msg", result ).toLower();

        if ( msg == "no such user" || msg == "padd md5 not 32 len" )
            setFailed( Request_WrongUserNameOrPassword );
        else
            setFailed( Handshake_SessionFailed,
                tr("Could not connect to server.") );
    }
    else if ( is_banned )
    {
        LOGL( 1, "Client version is banned" );

        setFailed( Handshake_Banned,
           tr( "This client version is obsolete. Please update." ) );
    }
    else if ( m_session.isEmpty() ||
              m_baseHost.isEmpty() || 
              m_basePath.isEmpty() ||
              m_streamUrl.isEmpty() )
    {
        setFailed( Handshake_SessionFailed, tr("Could not connect to server.") );
    }
    else if ( m_session.length() != 32 )
    {
        LOGL( 1, "Radio handshake failed: session length not 32 bytes. Retrying." );
        tryAgain();
    }
    else
    {
        Request::setBaseHost( m_baseHost );

        LOGL( 3, "Radio handshake successful. Host: " << m_baseHost << ", path: " << m_basePath );
    }
}
