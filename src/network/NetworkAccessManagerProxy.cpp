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

#define DEBUG_PREFIX "NetworkAccessManagerProxy"

#include "NetworkAccessManagerProxy.h"
#ifdef DEBUG_BUILD_TYPE
#include "NetworkAccessViewer.h"
#endif // DEBUG_BUILD_TYPE

#include "Version.h"

#include <KProtocolManager>

NetworkAccessManagerProxy *NetworkAccessManagerProxy::s_instance = nullptr;

NetworkAccessManagerProxy *NetworkAccessManagerProxy::instance()
{
    if( s_instance == nullptr )
        s_instance = new NetworkAccessManagerProxy();
    return s_instance;
}

void NetworkAccessManagerProxy::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

NetworkAccessManagerProxy::NetworkAccessManagerProxy( QObject *parent )
    : NetworkAccessManagerProxyBase( parent )
    , m_userAgent( QStringLiteral( "Amarok/" ) + QStringLiteral(AMAROK_VERSION) )
#ifdef DEBUG_BUILD_TYPE
    , m_viewer( nullptr )
#endif // DEBUG_BUILD_TYPE
{
    setCache(nullptr);   // disable QtWebKit cache to just use KIO one..
    qRegisterMetaType<NetworkAccessManagerProxy::Error>();
}

NetworkAccessManagerProxy::~NetworkAccessManagerProxy()
{
    s_instance = nullptr;
}

#ifdef DEBUG_BUILD_TYPE
NetworkAccessViewer *
NetworkAccessManagerProxy::networkAccessViewer()
{
    return m_viewer;
}

void
NetworkAccessManagerProxy::setNetworkAccessViewer( NetworkAccessViewer *viewer )
{
    if( viewer )
    {
        if( m_viewer )
            delete m_viewer;
        m_viewer = viewer;
    }
}
#endif // DEBUG_BUILD_TYPE

int
NetworkAccessManagerProxy::abortGet( const QList<QUrl> &urls )
{
    int removed = 0;
    const QSet<QUrl> urlSet(urls.begin(), urls.end());
    for( const QUrl &url : urlSet )
        removed += abortGet( url );
    return removed;
}

int
NetworkAccessManagerProxy::abortGet( const QUrl &url )
{
    if( m_urlMap.contains(url) )
        return 0;

    qDeleteAll( m_urlMap.values( url ) );
    int removed = m_urlMap.remove( url );
    return removed;
}

QUrl
NetworkAccessManagerProxy::getRedirectUrl( QNetworkReply *reply )
{
    QUrl targetUrl;

    // Get the original URL.
    QUrl originalUrl = reply->request().url();

    // Get the redirect attribute.
    QVariant redirectAttribute = reply->attribute( QNetworkRequest::RedirectionTargetAttribute );

    // Get the redirect URL from the attribute.
    QUrl redirectUrl = QUrl( redirectAttribute.toUrl() );

    // If the redirect URL is valid and if it differs from the original
    // URL then we return the redirect URL. Otherwise an empty URL will
    // be returned.
    if( !redirectUrl.isEmpty() && redirectUrl != originalUrl )
    {
        targetUrl = redirectUrl;
    }

    return targetUrl;
}

void
NetworkAccessManagerProxy::slotError( QObject *obj )
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( obj );
    if( !reply )
        return;
    QUrl url = reply->request().url();
    m_urlMap.remove( url );
    reply->deleteLater();
}

QNetworkReply *
NetworkAccessManagerProxy::createRequest( Operation op, const QNetworkRequest &req, QIODevice *outgoingData )
{
    QNetworkRequest request = req;
    request.setAttribute( QNetworkRequest::HttpPipeliningAllowedAttribute, true );
    if ( request.hasRawHeader( "User-Agent" ) )
        request.setRawHeader( "User-Agent", m_userAgent.toLocal8Bit() + ' ' + request.rawHeader( "User-Agent" ) );
    else
        request.setRawHeader( "User-Agent", m_userAgent.toLocal8Bit() );

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    KIO::CacheControl cc = KProtocolManager::cacheControl();
    switch (cc)
    {
    case KIO::CC_CacheOnly:      // Fail request if not in cache.
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
        break;

    case KIO::CC_Refresh:        // Always validate cached entry with remote site.
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork);
        break;

    case KIO::CC_Reload:         // Always fetch from remote site
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        break;

    case KIO::CC_Cache:          // Use cached entry if available.
    case KIO::CC_Verify:         // Validate cached entry with remote site if expired.
    default:
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        break;
    }
#else
    //TODO Qt6 cache functionality
#endif

    QNetworkReply *reply = NetworkAccessManagerProxyBase::createRequest( op, request, outgoingData );

#ifdef DEBUG_BUILD_TYPE
    if( m_viewer )
        m_viewer->addRequest( op, request, outgoingData, reply );
#endif // DEBUG_BUILD_TYPE
    return reply;
}

namespace The
{
    NetworkAccessManagerProxy *networkAccessManager()
    {
        return NetworkAccessManagerProxy::instance();
    }
}

#include "moc_NetworkAccessManagerProxy.cpp"
