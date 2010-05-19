/****************************************************************************************
 * Copyright (c) 2007 Trolltech ASA <copyright@trolltech.com>                           *
 * Copyright (c) 2008 Urs Wolfer <uwolfer@kde.org>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "NetworkAccessManagerProxy.h"

#include <QNetworkReply>

NetworkAccessManagerProxy *NetworkAccessManagerProxy::s_instance = 0;

NetworkAccessManagerProxy *NetworkAccessManagerProxy::instance()
{
    if( s_instance == 0 )
        s_instance = new NetworkAccessManagerProxy();
    return s_instance;
}

class NetworkAccessManagerProxy::NetworkAccessManagerProxyPrivate
{
public:
    NetworkAccessManagerProxyPrivate() {}
};

NetworkAccessManagerProxy::NetworkAccessManagerProxy( QObject *parent )
    : KIO::Integration::AccessManager( parent )
    , d( new NetworkAccessManagerProxyPrivate() )
{
}

NetworkAccessManagerProxy::~NetworkAccessManagerProxy()
{
    s_instance = 0;
    delete d;
}

QNetworkReply *NetworkAccessManagerProxy::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkReply *reply = KIO::Integration::AccessManager::createRequest( op, req, outgoingData );
    return reply;
}

namespace The
{
    NetworkAccessManagerProxy *networkAccessManager()
    {
        return NetworkAccessManagerProxy::instance();
    }
}

#include "NetworkAccessManagerProxy.moc"
