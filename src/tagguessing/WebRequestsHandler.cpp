/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#define DEBUG_PREFIX "WebRequestsHandler"

#include "WebRequestsHandler.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <ThreadWeaver/Weaver>

#include <QNetworkAccessManager>

using namespace TagGuessing;

WebRequestsHandler::WebRequestsHandler( QObject *parent,
                                        const QString &host,
                                        const int port,
                                        const QString &pathPrefix,
                                        const QString &clientId,
                                        const QString &clientVersion )
               : QObject( parent )
               , m_host( host )
               , m_port( port )
               , m_pathPrefix( pathPrefix )
               , m_clientId( clientId )
               , m_clientVersion( clientVersion )
{
    DEBUG_BLOCK
    debug() << "Initiating Tag search:";
    debug() << "\tHost:\t\t" << m_host;
    debug() << "\tPort:\t\t" << m_port;
    debug() << "\tPath Prefix:\t" << m_pathPrefix;
    debug() << "\tClient ID:\t" << m_clientId;
    debug() << "\tClient version:\t" << m_clientVersion;
    net = The::networkAccessManager();

    _timer = new QTimer( this );
    _timer->setInterval( 1000 );

    decodingComplete = false;

    connect( net, SIGNAL(finished(QNetworkReply*)), SLOT(gotReply(QNetworkReply*)) );
    connect( _timer, SIGNAL(timeout()), SLOT(sendNewRequest()) );
}

void
WebRequestsHandler::run( const Meta::TrackList &tracks )
{
    AudioToQStringDecoder *decoder = new AudioToQStringDecoder( tracks );
    connect( decoder, SIGNAL(trackDecoded(Meta::TrackPtr,QString)),
                      SLOT(trackDecoded(Meta::TrackPtr,QString)) );
    connect( decoder, SIGNAL(done(ThreadWeaver::Job*)),
                      SLOT(decodingDone(ThreadWeaver::Job*)) );

    ThreadWeaver::Weaver::instance()->enqueue( decoder );

    _timer->start();
}

void WebRequestsHandler::sendNewRequest()
{
    DEBUG_BLOCK
    if( m_requests.isEmpty() )
    {
        checkDone();
        return;
    }
    QPair < Meta::TrackPtr, QNetworkRequest > req = m_requests.takeFirst();
    QNetworkReply *reply = net->get( req.second );
    m_replyes.insert( reply, req.first );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)),
             this, SLOT(replyError(QNetworkReply::NetworkError)) );
    debug() << "Request sent: " << req.second.url().toString();
}

void
WebRequestsHandler::replyError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast< QNetworkReply * >( sender() );
    if( !reply )
        return;

    if( !m_replyes.contains( reply ) || code == QNetworkReply::NoError )
        return;

    disconnect( reply, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(replyError(QNetworkReply::NetworkError)) );

    debug() << "Error occurred during network request: " << reply->errorString();
    m_replyes.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
WebRequestsHandler::trackDecoded( const Meta::TrackPtr track, const QString fingerprint )
{
    DEBUG_BLOCK
    if( fingerprint.isEmpty() )
        return;
    m_requests.append( qMakePair( track, compileRequest( fingerprint, track ) ) );
}

void
WebRequestsHandler::decodingDone( ThreadWeaver::Job *_decoder )
{
    DEBUG_BLOCK
    AudioToQStringDecoder *decoder = ( AudioToQStringDecoder * )_decoder;
    disconnect( decoder, SIGNAL(trackDecoded(Meta::TrackPtr,QString)),
                this, SLOT(trackDecoded(Meta::TrackPtr,QString)) );
    disconnect( decoder, SIGNAL(done(ThreadWeaver::Job*)),
                this, SLOT(decodingDone(ThreadWeaver::Job*)) );
    decoder->deleteLater();
    decodingComplete = true;
    checkDone();
}

QNetworkRequest
WebRequestsHandler::compileRequest( const QString &fingerprint, const Meta::TrackPtr track )
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( m_host );
    url.setPort( m_port );
    url.setPath( m_pathPrefix+"/track/" );
    url.addQueryItem( "gnr", "" );
    url.addQueryItem( "art", track->artist().isNull()?"":track->artist()->name() );
    url.addQueryItem( "rmd", "0" );
    url.addQueryItem( "cid", m_clientId );
    url.addQueryItem( "alb", track->album().isNull()?"":track->album()->name() );
    url.addQueryItem( "fmt", "" );
    url.addQueryItem( "brt", QString::number( track->bitrate() ) );
    url.addQueryItem( "cvr", m_clientVersion );
    url.addQueryItem( "fpt", fingerprint );
    url.addQueryItem( "ttl", track->name().isNull()?track->playableUrl().fileName().remove(
                             QRegExp( "^.*(\\.+(?:\\w{2,5}))$" ) ):track->name() );
    url.addQueryItem( "tnm", "" );
    url.addQueryItem( "lkt", "" );
    url.addQueryItem( "dur", QString::number( track->length() ) );
    url.addQueryItem( "yrr", "" );

    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

#include "WebRequestsHandler.moc"
