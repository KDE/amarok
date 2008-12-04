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
