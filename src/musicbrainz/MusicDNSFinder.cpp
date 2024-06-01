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

#include <ThreadWeaver/Queue>

#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QUrlQuery>

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

    connect( net, &NetworkAccessManagerProxy::finished, this, &MusicDNSFinder::gotReply );
    connect( _timer, &QTimer::timeout, this, &MusicDNSFinder::sendNewRequest );
}

void
MusicDNSFinder::run( const Meta::TrackList &tracks )
{
    MusicDNSAudioDecoder *decoder = new MusicDNSAudioDecoder( tracks );
    connect( decoder, &MusicDNSAudioDecoder::trackDecoded,
             this, &MusicDNSFinder::trackDecoded );
    connect( decoder, &MusicDNSAudioDecoder::done,
             this, &MusicDNSFinder::decodingDone );

    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(decoder) );

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
    connect( reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
             this, &MusicDNSFinder::replyError );
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

        connect( parser, &MusicDNSXmlParser::done, this, &MusicDNSFinder::parsingDone );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(parser) );
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

    disconnect( reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                this, &MusicDNSFinder::replyError );

    debug() << "Error occurred during network request: " << reply->errorString();
    m_replyes.remove( reply );
    reply->deleteLater();
    checkDone();
}

void
MusicDNSFinder::parsingDone( ThreadWeaver::JobPointer _parser )
{
    DEBUG_BLOCK

    MusicDNSXmlParser *parser = dynamic_cast< MusicDNSXmlParser * >( _parser.data() );
    disconnect( parser, &MusicDNSXmlParser::done, this, &MusicDNSFinder::parsingDone );
    if( m_parsers.contains( parser ) )
    {
        bool found = false;
        foreach( QString PUID, parser->puid() )
            if( PUID != "00000000-0000-0000-0000-000000000000" )
            {
                found = true;
                Q_EMIT trackFound( m_parsers.value( parser ), PUID );
                break;
            }

        if( !found )
            Q_EMIT progressStep();

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
MusicDNSFinder::decodingDone( ThreadWeaver::JobPointer _decoder )
{
    DEBUG_BLOCK
    MusicDNSAudioDecoder *decoder = dynamic_cast<MusicDNSAudioDecoder*>(_decoder.data());
    disconnect( decoder, &MusicDNSAudioDecoder::trackDecoded,
                this, &MusicDNSFinder::trackDecoded );
    disconnect( decoder, &MusicDNSAudioDecoder::done,
                this, &MusicDNSFinder::decodingDone );
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
        Q_EMIT done();
    }
}

QNetworkRequest
MusicDNSFinder::compileRequest( const QString &fingerprint, const Meta::TrackPtr track )
{
    QUrl url;
    QUrlQuery query;
    url.setScheme( "http" );
    url.setHost( mdns_host );
    url.setPort( mdns_port );
    url.setPath( mdns_pathPrefix+"/track/" );
    query.addQueryItem( "gnr", "" );
    query.addQueryItem( "art", track->artist().isNull()?"":track->artist()->name() );
    query.addQueryItem( "rmd", "0" );
    query.addQueryItem( "cid", mdns_clientId );
    query.addQueryItem( "alb", track->album().isNull()?"":track->album()->name() );
    query.addQueryItem( "fmt", "" );
    query.addQueryItem( "brt", QString::number( track->bitrate() ) );
    query.addQueryItem( "cvr", mdns_clientVersion );
    query.addQueryItem( "fpt", fingerprint );
    query.addQueryItem( "ttl", track->name().isNull()?track->playableUrl().fileName().remove(
                             QRegularExpression( "^.*(\\.+(?:\\w{2,5}))$" ) ):track->name() );
    query.addQueryItem( "tnm", "" );
    query.addQueryItem( "lkt", "" );
    query.addQueryItem( "dur", QString::number( track->length() ) );
    query.addQueryItem( "yrr", "" );
    url.setQuery( query );

    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent" , "Amarok" );
    req.setRawHeader( "Connection", "Keep-Alive" );

    if( !_timer->isActive() )
        _timer->start();

    return req;
}

