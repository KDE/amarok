/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicDNSFinder"

#include "MusicDNSFinder.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <ThreadWeaver/Weaver>

#include <QNetworkAccessManager>

MusicDNSFinder::MusicDNSFinder( QObject *parent,
                                const QString &host, const int port, const QString &pathPrefix,
                                const QString &clietnId, const QString &clientVersion )
               : QObject( parent )
               , mdns_host( host )
               , mdns_port( port )
               , mdns_pathPrefix( pathPrefix )
               , mdns_clientId( clietnId )
               , mdns_clientVersion( clientVersion )
{
    DEBUG_BLOCK
    debug() << "Initiating MusicDNS search:";
    debug() << "\tHost:\t\t" << mdns_host;
    debug() << "\tPort:\t\t" << mdns_port;
    debug() << "\tPath Prefix:\t" << mdns_pathPrefix;
    debug() << "\tClient ID:\t" << mdns_clientId;
    debug() << "\tClient version:\t" << mdns_clientVersion;
    net = The::networkAccessManager();

    _timer = new QTimer( this );
    _timer->setInterval( 1000 );

    decodingComplete = false;

    connect( net, SIGNAL(finished(QNetworkReply*)), SLOT(gotReply(QNetworkReply*)) );
    connect( _timer, SIGNAL(timeout()), SLOT(sendNewRequest()) );
}

void
MusicDNSFinder::run( const Meta::TrackList &tracks )
{
    MusicDNSAudioDecoder *decoder = new MusicDNSAudioDecoder( tracks );
    connect( decoder, SIGNAL(trackDecoded(Meta::TrackPtr,QString)),
                      SLOT(trackDecoded(Meta::TrackPtr,QString)) );
    connect( decoder, SIGNAL(done(ThreadWeaver::Job*)),
                      SLOT(decodingDone(ThreadWeaver::Job*)) );

    ThreadWeaver::Weaver::instance()->enqueue( decoder );

    _timer->start();
}

void MusicDNSFinder::sendNewRequest()
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
MusicDNSFinder::gotReply( QNetworkReply *reply )
{
    DEBUG_BLOCK
    if( reply->error() == QNetworkReply::NoError && m_replyes.contains( reply ) )
    {
        QString document( reply->readAll() );
        MusicDNSXmlParser *parser = new MusicDNSXmlParser( document );
        if( !m_replyes.value( reply ).isNull() )
            m_parsers.insert( parser, m_replyes.value( reply ) );

        connect( parser, SIGNAL(done(ThreadWeaver::Job*)), SLOT(parsingDone(ThreadWeaver::Job*)) );
        ThreadWeaver::Weaver::instance()->enqueue( parser );
    }

    m_replyes.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicDNSFinder::replyError( QNetworkReply::NetworkError code )
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
MusicDNSFinder::parsingDone( ThreadWeaver::Job *_parser )
{
    DEBUG_BLOCK

    MusicDNSXmlParser *parser = qobject_cast< MusicDNSXmlParser * >( _parser );
    disconnect( parser, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(parsingDone(ThreadWeaver::Job*)) );
    if( m_parsers.contains( parser ) )
    {
        bool found = false;
        foreach( QString PUID, parser->puid() )
            if( PUID != "00000000-0000-0000-0000-000000000000" )
            {
                found = true;
                emit trackFound( m_parsers.value( parser ), PUID );
                break;
            }

        if( !found )
            emit progressStep();

        m_parsers.remove( parser );
    }

    parser->deleteLater();
    checkDone();
}

void
MusicDNSFinder::trackDecoded( const Meta::TrackPtr track, const QString fingerprint )
{
    DEBUG_BLOCK
    if( fingerprint.isEmpty() )
        return;
    m_requests.append( qMakePair( track, compileRequest( fingerprint, track ) ) );
}

void
MusicDNSFinder::decodingDone( ThreadWeaver::Job *_decoder )
{
    DEBUG_BLOCK
    MusicDNSAudioDecoder *decoder = ( MusicDNSAudioDecoder * )_decoder;
    disconnect( decoder, SIGNAL(trackDecoded(Meta::TrackPtr,QString)),
                this, SLOT(trackDecoded(Meta::TrackPtr,QString)) );
    disconnect( decoder, SIGNAL(done(ThreadWeaver::Job*)),
                this, SLOT(decodingDone(ThreadWeaver::Job*)) );
    decoder->deleteLater();
    decodingComplete = true;
    checkDone();
}

void
MusicDNSFinder::checkDone()
{
    if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() && decodingComplete )
    {
        debug() << "There is no any queued requests. Stopping timer.";
        _timer->stop();
        emit done();
    }
}

QNetworkRequest
MusicDNSFinder::compileRequest( const QString &fingerprint, const Meta::TrackPtr track )
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( mdns_host );
    url.setPort( mdns_port );
    url.setPath( mdns_pathPrefix+"/track/" );
    url.addQueryItem( "gnr", "" );
    url.addQueryItem( "art", track->artist().isNull()?"":track->artist()->name() );
    url.addQueryItem( "rmd", "0" );
    url.addQueryItem( "cid", mdns_clientId );
    url.addQueryItem( "alb", track->album().isNull()?"":track->album()->name() );
    url.addQueryItem( "fmt", "" );
    url.addQueryItem( "brt", QString::number( track->bitrate() ) );
    url.addQueryItem( "cvr", mdns_clientVersion );
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

