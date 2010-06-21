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

#include <config-amarok.h>

#include "amarok_export.h"

#include <kio/accessmanager.h>

class NetworkAccessManagerProxy;
#ifdef DEBUG_BUILD_TYPE
class NetworkAccessViewer;
#endif // DEBUG_BUILD_TYPE

namespace The
{
    AMAROK_EXPORT NetworkAccessManagerProxy *networkAccessManager();
}

class AMAROK_EXPORT NetworkAccessManagerProxy : public KIO::Integration::AccessManager
{
    Q_OBJECT

public:
    static NetworkAccessManagerProxy *instance();
    static void destroy();
    virtual ~NetworkAccessManagerProxy();

#ifdef DEBUG_BUILD_TYPE
    NetworkAccessViewer *networkAccessViewer();
    void setNetworkAccessViewer( NetworkAccessViewer *viewer );
#endif // DEBUG_BUILD_TYPE

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);
    virtual void timerEvent( QTimerEvent *event );

private slots:
    void replyFinished();
    void updateProgress( qint64 bytes, qint64 total );

private:
    NetworkAccessManagerProxy( QObject *parent = 0 );
    static NetworkAccessManagerProxy *s_instance;

    class NetworkAccessManagerProxyPrivate;
    NetworkAccessManagerProxyPrivate* const d;
    friend class NetworkAccessManagerProxyPrivate;

    Q_DISABLE_COPY( NetworkAccessManagerProxy );
};

#endif // AMAROK_NETWORKACCESSMANAGERPROXY
