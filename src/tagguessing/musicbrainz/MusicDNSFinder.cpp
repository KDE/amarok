/****************************************************************************************
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

#include "MusicDNSFinder.h"

#include "core/support/Debug.h"
#include "ThreadWeaver/Weaver"

MusicDNSFinder::MusicDNSFinder( QObject *parent,
                                const QString &host,
                                const int port,
                                const QString &pathPrefix,
                                const QString &clietnId,
                                const QString &clientVersion )
    : TagGuessing::WebRequestsHandler(
          parent,
          host,
          port,
          pathPrefix,
          clietnId,
          clientVersion )
{
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
MusicDNSFinder::checkDone()
{
    if( m_parsers.isEmpty() && m_requests.isEmpty() && m_replyes.isEmpty() && decodingComplete )
    {
        debug() << "There is no any queued requests. Stopping timer.";
        _timer->stop();
        emit done();
    }
}

#include "MusicDNSFinder.moc"
