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

#include "enginebase.h"
#include "metabundle.h"
#include "titleproxy.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kurl.h>

#include <qobject.h>
#include <qstring.h>


using namespace TitleProxy;

static const uint MIN_PROXYPORT = 6666;
static const uint MAX_PROXYPORT = 7777;
static const int BUFSIZE = 16384;


Proxy::Proxy( KURL url, int streamingMode )
        : QObject()
        , m_url( url )
        , m_streamingMode( streamingMode )
        , m_initSuccess( true )
        , m_metaInt( 0 )
        , m_byteCount( 0 )
        , m_metaLen( 0 )
        , m_headerFinished( false )
        , m_usedPort( 0 )
        , m_pBuf( 0 )
{
    kdDebug() << k_funcinfo << endl;

    m_pBuf = new char[ BUFSIZE ];
    
    if ( streamingMode == EngineBase::Socket ) {
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
            kdWarning() << k_funcinfo << "Unable to find a free local port. Aborting.\n";
            m_initSuccess = false;
            return;
        }
        m_usedPort = i;
        connect( server, SIGNAL( connected( int ) ), this, SLOT( accept( int ) ) );
    }
    else
        sendRequest( true );
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
    m_sockProxy.setSocket( socket );
    m_sockProxy.waitForMore( 5000 );

    sendRequest( true );    
}


void Proxy::sendRequest( bool meta )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
  
    { //connect to server
        //FIXME this needs a timeout check    
        m_sockRemote.connectToHost( m_url.host(), m_url.port() );
        while ( m_sockRemote.state() != QSocket::Connected )
            kapp->processEvents();
        
        if ( m_sockRemote.state() != QSocket::Connected ) {
            kdWarning() << k_funcinfo << "Unable to connect to remote server. Aborting.\n";
            return;
        }
    }
    
    { //send request                                 
        QString request = QString( "GET %1 HTTP/1.1\r\n" )
                                .arg( m_url.path( -1 ).isEmpty() ? "/" : m_url.path( -1 ) );
        //request metadata
        if ( meta ) request += "GNAAIcy-MetaData:1\r\n";
    
        request += "Connection: Keep-Alive\r\n"
                "User-Agent: aRts/1.2.2\r\n"
                "Accept: audio/x-mp3, video/mpeg, application/ogg\r\n"
                "Accept-Encoding: x-gzip, x-deflate, gzip, deflate\r\n"
                "Accept-Charset: iso-8859-15, utf-8;q=0.5, *;q=0.5\r\n"
                "Accept-Language: de, en\r\n\r\n";
                                
        connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
        m_sockRemote.writeBlock( request.latin1(), request.length() );
    }
        
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

            if ( m_streamingMode == EngineBase::Socket )
                bytesWrite = m_sockProxy.writeBlock( m_pBuf + index, bytesWrite );
            else 
                emit streamData( m_pBuf + index, bytesWrite );
            
            if ( bytesWrite == -1 ) { error(); return; }
            
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

            kdDebug() << k_funcinfo << "Got shoutcast header: '" << m_headerStr << "'" << endl;
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
            if ( m_streamingMode == EngineBase::Socket )
                m_sockProxy.writeBlock( m_headerStr.latin1(), m_headerStr.length() );
            m_headerFinished = true;

            if ( !m_metaInt ) {
                error();
                return false;
            }
            return true;
        }
    }
    return false;
}


void Proxy::error()
{
    kdDebug() <<  "TitleProxy error. Restarting stream in non-metadata mode.\n";
    
    m_sockRemote.close();
    //open stream again,  but this time without metadata, please
    sendRequest( false );
}


void Proxy::transmitData( const QString &data )
{
    kdDebug() << k_funcinfo << " received new metadata: '" << data << "'" << endl;

    //prevent metadata spam by ignoring repeated identical data
    //(some servers repeat it every 10 seconds)
    if ( data == m_lastMetadata ) return;
    m_lastMetadata = data;
        
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
