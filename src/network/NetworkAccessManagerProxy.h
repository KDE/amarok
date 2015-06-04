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
#include <config.h>

#include <KIO/AccessManager>
#include <QUrl>

#include <QNetworkReply>

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

    struct Error
    {
        QNetworkReply::NetworkError code;
        QString description;
    };

    /**
     * Gets the contents of the target @p url. It is a convenience wrapper
     * around QNetworkAccessManager::get() where the user supplies a
     * slot @p method to be called when the content is retrieved.
     * NOTE: On redirects requestRedirected is emitted.
     *
     * @param url the url to get the content from.
     * @param receiver the receiver object to call @p method on.
     * @param method the method to call when content is retrieved.
     * @param type the #Qt::ConnectionType used for calling the @p method.
     * @return a QNetworkReply object for custom monitoring.
     */
    QNetworkReply *getData( const QUrl &url, QObject *receiver, const char *method,
                            Qt::ConnectionType type = Qt::AutoConnection );

    int abortGet( const QUrl &url );
    int abortGet( const QList<QUrl> &urls );

    /**
     * Gets the URL to which a server redirects the request.
     * An empty QUrl will be returned if the request was not redirected.
     *
     * @param reply The QNetworkReply which contains all information about
     *              the reply from the server.
     *
     * @return The URL to which the server redirected the request or an empty
     *         URL if there was no redirect.
     */
    QUrl getRedirectUrl( QNetworkReply *reply );

#ifdef DEBUG_BUILD_TYPE
    NetworkAccessViewer *networkAccessViewer();
    void setNetworkAccessViewer( NetworkAccessViewer *viewer );
#endif // DEBUG_BUILD_TYPE

Q_SIGNALS:
    void requestRedirected( const QUrl &sourceUrl, const QUrl &targetUrl );
    void requestRedirected( QNetworkReply* oldReply, QNetworkReply *newReply );

public Q_SLOTS:
    void slotError( QObject *reply );

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req,
                                         QIODevice *outgoingData = 0);

private:
    NetworkAccessManagerProxy( QObject *parent = 0 );
    static NetworkAccessManagerProxy *s_instance;

    class NetworkAccessManagerProxyPrivate;
    NetworkAccessManagerProxyPrivate* const d;
    friend class NetworkAccessManagerProxyPrivate;

    Q_DISABLE_COPY( NetworkAccessManagerProxy )
    Q_PRIVATE_SLOT( d, void _replyFinished() )
};

Q_DECLARE_METATYPE( NetworkAccessManagerProxy::Error )
#endif // AMAROK_NETWORKACCESSMANAGERPROXY
