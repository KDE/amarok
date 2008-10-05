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


VerifyUserRequest::VerifyUserRequest()
        : Request( TypeVerifyUser, "VerifyUser" ),
          m_userAuthCode( AUTH_ERROR )
{
    setOverrideCursor();
}


void
VerifyUserRequest::start()
{
    time_t now; 
    time( &now );
    QString const time = QString::number( now );

    // Concatenate pw hash with time
    QString auth = m_passwordMd5 + time;
    QString authLower = m_passwordMd5Lower + time;
    
    // Hash the concatenated string to create auth code
    QString authMd5 = UnicornUtils::md5Digest( auth.toUtf8() );
    QString authMd5Lower = UnicornUtils::md5Digest( authLower.toUtf8() );

    QString const path = "/ass/pwcheck.php?"
                    "time=" + QString( QUrl::toPercentEncoding( time ) ) +
                    "&username=" + QString( QUrl::toPercentEncoding( m_username ) ) +
                    "&auth=" + authMd5 +
                    "&auth2=" + authMd5Lower +
                    "&defaultplayer="
                    #ifdef WIN32
                    + QString( QUrl::toPercentEncoding( UnicornUtils::findDefaultPlayer() ) )
                    #endif
                    ;

    get( path );
}


void
VerifyUserRequest::success( QByteArray data )
{
    QString response = data;
    response = response.trimmed();

    //TODO mxcl do in baseclass?
    LOG( 4, "Verify response: " << response << "\n" );

    m_bootStrapCode = response.contains( "BOOTSTRAP" ) 
                ? BOOTSTRAP_ALLOWED
                : BOOTSTRAP_DENIED;

    if (response.contains( "OK2" ))
        m_userAuthCode = AUTH_OK_LOWER;
    else if (response.contains( "OK" ))
        m_userAuthCode = AUTH_OK;
    else if (response.contains( "INVALIDUSER" ))
        m_userAuthCode = AUTH_BADUSER;
    else if (response.contains( "BADPASSWORD" ))
        m_userAuthCode = AUTH_BADPASS;
    else
        m_userAuthCode = AUTH_ERROR;
}
