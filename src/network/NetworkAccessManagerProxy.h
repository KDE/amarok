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
#include "core/support/Debug.h"


#include <QNetworkReply>
#include <QPointer>
#include <QUrl>
#include <QThread>
#include <QTimer>


class NetworkAccessManagerProxy;
#ifdef DEBUG_BUILD_TYPE
class NetworkAccessViewer;
#endif // DEBUG_BUILD_TYPE


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KIO/AccessManager>
typedef KIO::Integration::AccessManager NetworkAccessManagerProxyBase;
#else
#include <QNetworkAccessManager>
typedef QNetworkAccessManager NetworkAccessManagerProxyBase;
#endif

namespace The
{
    AMAROK_EXPORT NetworkAccessManagerProxy *networkAccessManager();
}

class AMAROK_EXPORT NetworkAccessManagerProxy : public NetworkAccessManagerProxyBase
{
    Q_OBJECT

public:
    static NetworkAccessManagerProxy *instance();
    static void destroy();
    ~NetworkAccessManagerProxy() override;

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
     * @param type the Qt::ConnectionType used for calling the @p method.
     * @return a QNetworkReply object for custom monitoring.
     */
    template<typename Return, typename Object, typename... Args>
    QNetworkReply *getData( const QUrl &url, Object *receiver, Return ( Object::*method )( Args... ),
                            Qt::ConnectionType type = Qt::AutoConnection )
    {
        if( !url.isValid() )
        {
            const QMetaObject *mo = receiver->metaObject();
            debug() << QStringLiteral( "Error: URL '%1' is invalid (from %2)" ).arg( url.url(), QLatin1String(mo->className()) );
            return nullptr;
        }

        QNetworkReply *r = get( QNetworkRequest(url) );
        m_urlMap.insert( url, r );
        auto lambda = [this, r, receiver, method, type] ()
        {
            replyFinished( r, QPointer<Object>( receiver ), method, type );
        };
        connect( r, &QNetworkReply::finished, this, lambda );
        return r;
    }

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
    void requestRedirectedUrl( const QUrl &sourceUrl, const QUrl &targetUrl );
    void requestRedirectedReply( QNetworkReply* oldReply, QNetworkReply *newReply );

public Q_SLOTS:
    void slotError( QObject *reply );

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req,
                                         QIODevice *outgoingData = nullptr) override;

private:
    NetworkAccessManagerProxy( QObject *parent = nullptr );

    template <typename Return, typename Object, typename... Args>
    void replyFinished( QNetworkReply *reply, QPointer<Object> receiver, Return ( Object::*method )( Args... ), Qt::ConnectionType type )
    {
        if( !reply || !receiver )
            return;

        QUrl url = reply->request().url();
        QByteArray data = reply->readAll();
        data.detach(); // detach so the bytes are not deleted before methods are invoked

        // There may have been a redirect.
        QUrl redirectUrl = getRedirectUrl( reply );

        // Check if there's no redirect.
        if( redirectUrl.isEmpty() )
        {
            Error err = { reply->error(), reply->errorString() };

            if( type == Qt::AutoConnection )
            {
                if( QThread::currentThread() == receiver->thread() )
                    type = Qt::DirectConnection;
                else
                    type = Qt::QueuedConnection;
            }

            if( type == Qt::DirectConnection )
                ( receiver->*method )( url, data, err );
            else
            {
                auto lambda = [receiver, method, url, data, err] ()
                {
                    ( receiver->*method )( url, data, err );
                };
                QTimer::singleShot( 0, receiver, lambda );
            }
        }
        else
        {
            debug() << "the server is redirecting the request to: " << redirectUrl;

            // Let's try to fetch the data again, but this time from the new url.
            QNetworkReply *newReply = getData( redirectUrl, receiver.data(), method, type );

            Q_EMIT requestRedirectedUrl( url, redirectUrl );
            Q_EMIT requestRedirectedReply( reply, newReply );
        }

        reply->deleteLater();
    }

    static NetworkAccessManagerProxy *s_instance;

    QMultiHash<QUrl, QNetworkReply*> m_urlMap;
    QString m_userAgent;

#ifdef DEBUG_BUILD_TYPE
    NetworkAccessViewer *m_viewer;
#endif // DEBUG_BUILD_TYPE

    Q_DISABLE_COPY( NetworkAccessManagerProxy )
};

Q_DECLARE_METATYPE( NetworkAccessManagerProxy::Error )
#endif // AMAROK_NETWORKACCESSMANAGERPROXY
