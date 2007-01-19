/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "amarok.h"
#include "amarokconfig.h"
#include "daapclient.h"
#include "daapreader/authentication/hasher.h"
#include "debug.h"
#include "proxy.h"

#include <kapplication.h>

using namespace Daap;

//input url: daap://host:port/databaseId/music.ext
/*
        bundle->setUrl( Amarok::QStringx("http://%1:3689/databases/%2/items/%3.%4?%5").args(
            QStringList() << m_host
                        << m_databaseId
                        << QString::number( (*it).asMap()["miid"].asList()[0].asInt() )
                        << (*it).asMap()["asfm"].asList()[0].asString()
                        << m_loginString ) );

*/
Proxy::Proxy(KURL stream, DaapClient* client, const char* name)
    : QObject(client, name)
    , m_proxy( new Amarok::ProcIO() )
{
    DEBUG_BLOCK
    //find the request id and increment it
    const QString hostKey = stream.host() + ':' + QString::number(stream.port());
    const int revisionId = client->incRevision( hostKey );
    const int sessionId = client->getSession( hostKey );
    //compose URL
    KURL realStream = realStreamUrl( stream, sessionId );

    //get hash
    char hash[33] = {0};
    GenerateHash( 3
        , reinterpret_cast<const unsigned char*>((realStream.path() + realStream.query()).ascii())
        , 2
        , reinterpret_cast<unsigned char*>(hash)
        , revisionId );

    // Find free port
    MyServerSocket* socket = new MyServerSocket();
    const int port = socket->port();
    debug() << "Proxy server using port: " << port << endl;
    delete socket;
    m_proxyUrl = KURL( QString("http://localhost:%1/daap.mp3").arg( port ) );
    //start proxy
    m_proxy->setComm( KProcess::Communication( KProcess::AllOutput ) );
    *m_proxy << "amarok_proxy.rb";
    *m_proxy << "--daap";
    *m_proxy << QString::number( port );
    *m_proxy << realStream.url();
    *m_proxy << AmarokConfig::soundSystem();
    *m_proxy << hash;
    *m_proxy << QString::number( revisionId );
    *m_proxy << Amarok::proxyForUrl( realStream.url() );

    if( !m_proxy->start( KProcIO::NotifyOnExit, true ) ) {
        error() << "Failed to start amarok_proxy.rb" << endl;
        return;
    }

    QString line;
    while( true ) {
        kapp->processEvents();
        m_proxy->readln( line );
        if( line == "AMAROK_PROXY: startup" ) break;
    }
    debug() << "started amarok_proxy.rb --daap " << QString::number( port ) << ' ' << realStream.url() << ' ' << AmarokConfig::soundSystem() << ' ' << hash << ' ' << revisionId << endl;
    connect( m_proxy, SIGNAL( processExited( KProcess* ) ), this, SLOT( playbackStopped() ) );
    connect( m_proxy, SIGNAL( readReady( KProcIO* ) ), this, SLOT( readProxy() ) );
}

Proxy::~Proxy()
{
    delete m_proxy;
}

void
Proxy::playbackStopped()
{
    deleteLater();
}

void
Proxy::readProxy()
{
    QString line;

    while( m_proxy->readln( line ) != -1 )
    {
        debug() << line << endl;
    }
}

KURL Proxy::realStreamUrl( KURL fakeStream, int sessionId )
{
    KURL realStream;
    realStream.setProtocol( "http" );
    realStream.setHost(fakeStream.host());
    realStream.setPort(fakeStream.port());
    realStream.setPath( "/databases" + fakeStream.directory() + "/items/" + fakeStream.fileName() );
    realStream.setQuery( QString("?session-id=") + QString::number(sessionId) );
    return realStream;
}

#include "proxy.moc"
