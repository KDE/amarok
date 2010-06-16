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

#include "core/support/Debug.h"
#include "NetworkAccessManagerProxy.h"
#include "NetworkAccessViewer.h"

#include <KProtocolManager>

#include <QBasicTimer>
#include <QNetworkReply>
#include <QPointer>

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
    NetworkAccessManagerProxyPrivate( NetworkAccessManagerProxy *parent ) : viewer( 0 ), q( parent ) {}

    ~NetworkAccessManagerProxyPrivate()
    {
        qDeleteAll( replyTimer.values() );
        qDeleteAll( timerIds.values() );
    }

    void addReplyTimeout( QPointer<QNetworkReply> reply )
    {
        QBasicTimer *timer = new QBasicTimer();
        timer->start( 15000, q );
        int id = timer->timerId();
        replyTimer.insert( id, reply );
        timerIds.insert( id, timer );
    }

    void restartTimeout( QPointer<QNetworkReply> reply )
    {
        const int timerId  = replyTimer.key( reply );
        QBasicTimer *timer = timerIds.value( timerId );
        timer->stop();
        timer->start( 15000, q );
    }

    void removeReply( QPointer<QNetworkReply> reply )
    {
        const int timerId = replyTimer.key( reply );
        replyTimer.remove( timerId );
        QBasicTimer *timer = timerIds.take( timerId );
        delete timer;
    }

    void pruneReplies()
    {
        foreach( const QPointer<QNetworkReply> &reply, replyTimer )
        {
            if( reply.isNull() )
                removeReply( reply );
        }
    }

    NetworkAccessViewer *viewer;
    QHash<int, QPointer<QNetworkReply> > replyTimer;
    QHash<int, QBasicTimer*> timerIds;
    QString userAgent;

    NetworkAccessManagerProxy *const q;
};

NetworkAccessManagerProxy::NetworkAccessManagerProxy( QObject *parent )
    : KIO::Integration::AccessManager( parent )
    , d( new NetworkAccessManagerProxyPrivate( this ) )
{
    setCache(0);   // disable QtWebKit cache to just use KIO one..
    d->userAgent = KProtocolManager::defaultUserAgent();
}

NetworkAccessManagerProxy::~NetworkAccessManagerProxy()
{
    delete d;
    s_instance = 0;
}

NetworkAccessViewer *
NetworkAccessManagerProxy::networkAccessViewer()
{
    return d->viewer;
}

void
NetworkAccessManagerProxy::setNetworkAccessViewer( NetworkAccessViewer *viewer )
{
    if( viewer )
    {
        if( d->viewer )
            delete d->viewer;
        d->viewer = viewer;
    }
}

QNetworkReply *
NetworkAccessManagerProxy::createRequest( Operation op, const QNetworkRequest &req, QIODevice *outgoingData )
{
    QNetworkRequest request = req;
    request.setAttribute( QNetworkRequest::HttpPipeliningAllowedAttribute, true );
    request.setRawHeader( "User-Agent", d->userAgent.toLocal8Bit() );

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

    QNetworkReply *reply = KIO::Integration::AccessManager::createRequest( op, request, outgoingData );
    connect( reply, SIGNAL(finished()), SLOT(replyFinished()) );

    switch( reply->operation() )
    {
    case QNetworkAccessManager::GetOperation:
    case QNetworkAccessManager::HeadOperation:
        connect( reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateProgress(qint64,qint64)) );
        d->addReplyTimeout( reply );
        break;

    case QNetworkAccessManager::PutOperation:
    case QNetworkAccessManager::PostOperation:
        connect( reply, SIGNAL(uploadProgress(qint64,qint64)), SLOT(updateProgress(qint64,qint64)) );
        d->addReplyTimeout( reply );
        break;

    default:
        break;
    }

    if( d->viewer )
        d->viewer->addRequest( op, request, outgoingData, reply );
    return reply;
}

void
NetworkAccessManagerProxy::timerEvent( QTimerEvent *event )
{
    const int tid = event->timerId();
    if( d->timerIds.contains( tid ) )
    {
        // timeout reached
        QBasicTimer *timer   = d->timerIds.take( tid );
        QPointer<QNetworkReply> reply = d->replyTimer.take( tid );
        if( reply )
            reply->abort();
        delete timer;
    }
    else
    {
        KIO::Integration::AccessManager::timerEvent( event );
    }
}

void
NetworkAccessManagerProxy::replyFinished()
{
    d->removeReply( qobject_cast<QNetworkReply*>(sender()) );
    d->pruneReplies();
}

void
NetworkAccessManagerProxy::updateProgress( qint64 bytes, qint64 total )
{
    Q_UNUSED( bytes )
    Q_UNUSED( total )

    QPointer<QNetworkReply> reply = qobject_cast<QNetworkReply*>( sender() );
    if( reply )
        d->restartTimeout( reply );
}

namespace The
{
    NetworkAccessManagerProxy *networkAccessManager()
    {
        return NetworkAccessManagerProxy::instance();
    }
}

#include "NetworkAccessManagerProxy.moc"
