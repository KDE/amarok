/***************************************************************************
                     Proxy.cpp  -  description
                        -------------------
begin                : Nov 20 14:35:18 CEST 2003
copyright            : (C) 2003 by Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "metabundle.h"
#include "titleproxy.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kurl.h>

#include <qobject.h>
#include <qstring.h>

// Some info on the shoutcast metadata protocol can be found at:
// http://www.smackfu.com/stuff/programming/shoutcast.html

// Proxy Concept:
// 1. Connect to streamserver
// 2. Listen on localhost, let aRts connect to proxyserver
// 3. Read HTTP GET request from proxyserver (sent by aRts)
// 4. Modify GET request by adding Icy-MetaData:1 token
// 5. Write request to streamserver
// 6. Read MetaInt token from streamserver (==metadata offset)
//
// 7. Read stream data (mp3 + metadata) from streamserver
// 8. Filter out metadata, send to app
// 9. Write mp3 data to proxyserver
//10. Goto 7

using namespace TitleProxy;

static const uint MIN_PROXYPORT = 6666;
static const uint MAX_PROXYPORT = 7777;
static const int BUFSIZE = 16384;


Proxy::Proxy( KURL url )
        : QObject()
        , m_url( url )
        , m_initSuccess( false )
        , m_metaInt( 0 )
        , m_byteCount( 0 )
        , m_metaLen( 0 )
        , m_headerFinished( false )
        , m_usedPort( 0 )
        , m_pBuf( 0 )
{
    connect( this, SIGNAL( error() ), this, SLOT( deleteLater() ) );    //delete yourself in case of error

    kdDebug() << k_funcinfo << endl;

    //FIXME this needs a timeout check    
    m_sockRemote.connectToHost( url.host(), url.port() );
    while ( m_sockRemote.state() != QSocket::Connected )
        kapp->processEvents();
    
    kdDebug() << k_funcinfo << "sock.connectToHost() state: " << m_sockRemote.state() << endl;
    
    if ( m_sockRemote.state() != QSocket::Connected ) {
        emit error();
        return ;
    }

    uint i;
    Server* server;
    for ( i = MIN_PROXYPORT; i <= MAX_PROXYPORT; i++ ) {
        server = new Server( i, this );
        kdDebug() << k_funcinfo <<
        "Trying to bind to port: " << i << endl;
        if ( server->ok() )     // found a free port
            break;
        delete server;
    }
    
    if ( i > MAX_PROXYPORT ) {
        emit error();
        return ;
    }
    
    m_pBuf = new char[ BUFSIZE ];
    m_usedPort = i;
    m_initSuccess = true;
    connect( server, SIGNAL( connected( int ) ), this, SLOT( accept( int ) ) );
}


Proxy::~Proxy()
{
    kdDebug() << k_funcinfo << endl;

    delete[] m_pBuf;
}


KURL Proxy::proxyUrl()
{
    if ( m_initSuccess ) {
        KURL url;
        url.setPort( m_usedPort );
        url.setHost( "localhost" );
        url.setProtocol( "http" );
        return url;

    } else
        return m_url;
}


void Proxy::accept( int socket )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    
    m_sockProxy.setSocket( socket );

    m_sockProxy.waitForMore( 5000 );
    int bytesRead = m_sockProxy.readBlock( m_pBuf, BUFSIZE );
    kdDebug() << k_funcinfo << "m_sockProxy bytesRead = " << bytesRead << endl;
    
    QCString str( m_pBuf, bytesRead );
    int index1 = str.find( "GET / HTTP/1.1" );
    int index2 = str.find( "\n", index1 ) + 1;
    m_sockRemote.writeBlock( m_pBuf, index1 );
    
    //This is our new, modified request, containing the correct path to the stream
    QString request = QString( "GET %1 HTTP/1.1\nIcy-MetaData:1\r\n" )
                      .arg( m_url.path( -1 ) );
    
    kdDebug() << "request: " << request << endl;
    
    m_sockRemote.writeBlock( request.latin1(), request.length() );
    m_sockRemote.writeBlock( m_pBuf + index2, bytesRead - index2 );
    
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
    
    kdDebug() << "END " << k_funcinfo << endl;
}


void Proxy::readRemote()
{
    Q_LONG index = 0;
    Q_LONG bytesWrite = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_pBuf, BUFSIZE );
    if ( bytesRead == -1 ) { emit error(); return; }
    
    if ( !m_headerFinished )
        if ( !processHeader( index, bytesRead ) ) return;

    //This is the main loop which processes the stream data
    while ( index < bytesRead ) {
        if ( m_metaInt && ( m_byteCount == m_metaInt ) ) {
            m_byteCount = 0;
            m_metaLen = m_pBuf[ index++ ] << 4;
        }
        else if ( m_metaLen ) {
            m_metaData.append( m_pBuf[ index++ ] );
            --m_metaLen;

            if ( !m_metaLen ) {
                transmitData( m_metaData );
                m_metaData = "";
            }
        }
        else {
            bytesWrite = bytesRead - index;

            if ( bytesWrite > m_metaInt - m_byteCount )
                bytesWrite = m_metaInt - m_byteCount;

            bytesWrite = m_sockProxy.writeBlock( m_pBuf + index, bytesWrite );
            if ( bytesWrite == -1 ) { emit error(); return; }
            
            index += bytesWrite;
            m_byteCount += bytesWrite;
        }
    }
}


bool Proxy::processHeader( Q_LONG &index, Q_LONG bytesRead )
{
    while ( index < bytesRead ) {
        m_headerStr.append( m_pBuf[ index++ ] );
        if ( m_headerStr.endsWith( "\r\n\r\n" ) ) {

            /*kdDebug() << k_funcinfo <<
                  "Got shoutcast header: '" << m_headerStr << "'" << endl;*/
            /*
            TODO: emit error including helpful error message on headers like this:

            ICY 401 Service Unavailable
            icy-notice1:<BR>SHOUTcast Distributed Network Audio Server/Linux v1.9.2<BR>
            icy-notice2:The resource requested is currently unavailable<BR>
            */
            m_metaInt = m_headerStr.section( "icy-metaint:", 1, 1,
                                             QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 )
                        .toInt();
            m_bitRate = m_headerStr.section( "icy-br:", 1, 1,
                                             QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 ).toInt();
            m_streamName = m_headerStr.section( "icy-name:", 1, 1,
                                                QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );
            m_streamGenre = m_headerStr.section( "icy-genre:", 1, 1,
                                                 QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );
            m_streamUrl = m_headerStr.section( "icy-url:", 1, 1,
                                               QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );
            if ( m_streamUrl.startsWith( "www.", true ) )
                m_streamUrl.prepend( "http://" );
            m_sockProxy.writeBlock( m_headerStr.latin1(), m_headerStr.length() );
            m_headerFinished = true;

            if ( !m_metaInt ) {
                emit error();
                return false;
            }
            return true;
        }
    }
    return false;
}


void Proxy::transmitData( const QString &data )
{
    kdDebug() << k_funcinfo << " received new metadata: '" << data << "'" << endl;

    MetaBundle bundle( extractStr( data, "StreamTitle" ),
                       extractStr( data, "StreamUrl" ),
                       m_bitRate,
                       m_streamGenre,
                       m_streamName,
                       m_streamUrl );

    emit metaData( bundle );
}


QString Proxy::extractStr( const QString &str, const QString &key )
{
    int index = str.find( key, 0, true );
    if ( index == -1 ) {
        return QString::null;

    } else {
        index = str.find( "'", index ) + 1;
        int indexEnd = str.find( "'", index );
        return str.mid( index, indexEnd - index );

    }
}

#include "titleproxy.moc"
