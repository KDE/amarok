/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
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

#include "ScrobblerHttp.h"
#include "Scrobbler.h"
#include "knetworkreply.h"

#include <KDebug>
#include <kio/job.h>

#include <QDebug>
#include <QTimer>


ScrobblerHttp::ScrobblerHttp( QObject* parent )
             : QNetworkAccessManager( parent ),
               m_reply( 0 )
{
    m_retry_timer = new QTimer( this );
    m_retry_timer->setSingleShot( true );
    connect( m_retry_timer, SIGNAL(timeout()), SLOT(request()) );
    resetRetryTimer();

    connect( this, SIGNAL(finished( QNetworkReply* )), SLOT(onFinished( QNetworkReply* )) );
}


void
ScrobblerHttp::onFinished( QNetworkReply* reply )
{
    if (reply == m_reply)
    {
        if ( m_reply->error() == QNetworkReply::OperationCanceledError )
            return;
		
        QByteArray const data = m_reply->readAll();

        if (m_reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "ERROR! QNetworkReply error code: " << m_reply->error();
            emit done( QByteArray() );
        }
        else
        {
            resetRetryTimer();
            emit done( data );
        }
		
	// just in case
        m_reply = 0;
    }
}


void
ScrobblerPostHttp::setUrl( const QUrl& url )
{
    m_request.setUrl( url );
}


void 
ScrobblerHttp::retry()
{
    int const i = m_retry_timer->interval();
    if (i < 120 * 60 * 1000)
        m_retry_timer->setInterval( i * 2 );

    qDebug() << "Will retry in" << m_retry_timer->interval() / 1000 << "seconds";

    m_retry_timer->start();
}


void
ScrobblerHttp::resetRetryTimer()
{
    m_retry_timer->setInterval( 30 * 1000 );
}


void
ScrobblerPostHttp::request()
{
    if (m_data.isEmpty() || m_session.isEmpty())
        return;

    m_request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );

    qDebug() << "HTTP POST:" << m_request.url().toString() + m_data;

    m_reply = QNetworkAccessManager::post( m_request, "s=" + m_session + m_data );
}


QNetworkReply *ScrobblerHttp::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    KIO::Job *kioJob = 0;

    switch (op) {
        case HeadOperation: {
            kDebug() << "HeadOperation:" << req.url();

            kioJob = KIO::mimetype(req.url(), KIO::HideProgressInfo);

            break;
        }
        case GetOperation: {
            kDebug() << "GetOperation:" << req.url();

            kioJob = KIO::get(req.url(), KIO::NoReload, KIO::HideProgressInfo);

            break;
        }
        case PutOperation: {
            kDebug() << "PutOperation:" << req.url();

            kioJob = KIO::put(req.url(), -1, KIO::HideProgressInfo);

            break;
        }
        case PostOperation: {
            kDebug() << "PostOperation:" << req.url();

            kioJob = KIO::http_post(req.url(), outgoingData->readAll(), KIO::HideProgressInfo);

            break;
        }
        default:
            kDebug() << "Unknown operation";
            return 0;
    }

    KNetworkReply *reply = new KNetworkReply(req, kioJob, this);

    kioJob->addMetaData(metaDataForRequest(req));

    connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        reply, SLOT(appendData(KIO::Job *, const QByteArray &)));
    connect(kioJob, SIGNAL(result(KJob *)), reply, SIGNAL(finished()));
    connect(kioJob, SIGNAL(mimetype(KIO::Job *, const QString&)),
        reply, SLOT(setMimeType(KIO::Job *, const QString&)));

    return reply;
}


KIO::MetaData ScrobblerHttp::metaDataForRequest(QNetworkRequest request)
{
    KIO::MetaData metaData;

    metaData.insert("PropagateHttpHeader", "true");

    metaData.insert("UserAgent", request.rawHeader("User-Agent"));
    request.setRawHeader("User-Agent", QByteArray());

    metaData.insert("accept", request.rawHeader("Accept"));
    request.setRawHeader("Accept", QByteArray());

    request.setRawHeader("content-length", QByteArray());
    request.setRawHeader("Connection", QByteArray());

    QString additionHeaders;
    Q_FOREACH(const QByteArray &headerKey, request.rawHeaderList()) {
        const QByteArray value = request.rawHeader(headerKey);
        if (value.isNull())
            continue;

        if (additionHeaders.length() > 0) {
            additionHeaders += "\r\n";
        }
        additionHeaders += headerKey + ": " + value;
    }
    metaData.insert("customHTTPHeader", additionHeaders);

    return metaData;
}

