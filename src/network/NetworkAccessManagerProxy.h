/****************************************************************************************
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

#ifndef AMAROK_NETWORKACCESSMANAGERPROXY
#define AMAROK_NETWORKACCESSMANAGERPROXY

#include "amarok_export.h"

#include <kio/accessmanager.h>

class NetworkAccessManagerProxy;
class NetworkAccessViewer;

namespace The
{
    AMAROK_EXPORT NetworkAccessManagerProxy *networkAccessManager();
}

class AMAROK_EXPORT NetworkAccessManagerProxy : public KIO::Integration::AccessManager
{
    Q_OBJECT

public:
    static NetworkAccessManagerProxy *instance();
    virtual ~NetworkAccessManagerProxy();

    NetworkAccessViewer *networkAccessViewer();
    void setNetworkAccessViewer( NetworkAccessViewer *viewer );

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private:
    NetworkAccessManagerProxy( QObject *parent = 0 );
    static NetworkAccessManagerProxy *s_instance;

    class NetworkAccessManagerProxyPrivate;
    NetworkAccessManagerProxyPrivate* const d;
};

#endif // AMAROK_NETWORKACCESSMANAGERPROXY
