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
#include "XmlRpc.h"
#include "WebService.h"


ActionRequest::ActionRequest( const char *method, RequestType t )
            : Request( t, method ), m_method( method )
{
    setOverrideCursor();
}

void
ActionRequest::start()
{
    XmlRpc xmlrpc;
    xmlrpc.setMethod( m_method );

    QString const challenge = The::webService()->challengeString();

    xmlrpc  << The::webService()->currentUsername()
            << challenge
            << UnicornUtils::md5Digest( QString( The::webService()->currentPassword() + challenge ).toUtf8() )
            << m_artist
            << m_title;

    request( xmlrpc );
}
