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

#include <kdebug.h>
#include <kextsock.h>
#include <kurl.h>

#include <qobject.h>
#include <qsocketnotifier.h>
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

static const int MIN_PROXYPORT = 6666;
static const int MAX_PROXYPORT = 7777;
static const int IN_BUFSIZE    = (8192 * 4);
static const int BUFSIZE       = 8192;


Proxy::Proxy(KURL url) : QObject(),
        m_url(url),
        m_initSuccess(false),
        m_metaInt(0),
        m_byteCount(0),
        m_metaLen(0),
        m_headerFinished(false),
        m_usedPort(0),
        m_pBuf(0),
        m_pSockProxy(0)
{
    //kdDebug() << k_funcinfo << "Called" << endl;
    m_sockRemote.setSocketFlags( KExtendedSocket::inetSocket |
                                 KExtendedSocket::bufferedSocket |
                                 KExtendedSocket::streamSocket );
    m_sockRemote.setAddress(url.host(), url.port());
    m_sockRemote.setTimeout(8);

    int connectResult = m_sockRemote.connect();
    kdDebug() << k_funcinfo << "sock.connect() result: " << connectResult << endl;
    if ( connectResult != 0 ) {
        emit error();
        return;
    }

    int listenResult = -1;
    unsigned int i = 0;
    for(i = MIN_PROXYPORT; i <= MAX_PROXYPORT; i++)
    {
        m_sockPassive.setPort(i);
        m_sockPassive.setHost("localhost");
        m_sockPassive.setSocketFlags(KExtendedSocket::passiveSocket);
        listenResult = m_sockPassive.listen();
        kdDebug() << k_funcinfo <<
        "Trying to bind to port " << i << ", listen() result: " << listenResult << endl;
        if(listenResult == 0)                     // found a free port
            break;
        m_sockPassive.reset();

    }

    kdDebug() << k_funcinfo << "final listen() result: " << listenResult << endl;
    if ( listenResult != 0 ) {
        emit error();
        return;
    }
    m_usedPort = i;

    m_pBuf = new char[ BUFSIZE ];

    connect( &m_sockPassive, SIGNAL( readyAccept() ), this, SLOT( accept() ) );
    m_initSuccess = true;
}


Proxy::~Proxy()
{
    //kdDebug() << k_funcinfo << "Called" << endl;
    delete[] m_pBuf;
    delete m_pSockProxy;
}


KURL Proxy::proxyUrl()
{
    if(m_initSuccess)
    {
        KURL url;
        url.setPort(m_usedPort);
        url.setHost("localhost");
        url.setProtocol("http");
        return url;

    }
    else
        return m_url;
}


void Proxy::accept()
{
    m_sockPassive.accept( m_pSockProxy );
    m_sockPassive.close();                        // don't take another connection
    m_pSockProxy->setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::bufferedSocket );

    int bytesRead = m_pSockProxy->readBlock( m_pBuf, BUFSIZE );
    m_pSockProxy->setBlockingMode( false );

    QString str = QString::fromAscii( m_pBuf, bytesRead );
    int index = str.find( "\n", str.find( "GET / HTTP/1.1" ) ) + 1;

    m_sockRemote.setBufferSize( IN_BUFSIZE );
    m_sockRemote.enableRead( true );
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
    m_sockRemote.writeBlock( m_pBuf, index );

    QString icyStr( "Icy-MetaData:1\r\n" );
    m_sockRemote.writeBlock( icyStr.latin1(), icyStr.length() );
    m_sockRemote.writeBlock( m_pBuf + index, bytesRead - index );
}


void Proxy::readRemote()
{
    Q_LONG index = 0;
    Q_LONG bytesWrite = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_pBuf, BUFSIZE );

    if(!m_headerFinished)
        processHeader(index, bytesRead);

    //This is the main loop which processes the stream data
    while ( index < bytesRead )
    {
        if ( m_metaInt && ( m_byteCount == m_metaInt ) )
        {
            m_byteCount = 0;
            m_metaLen = m_pBuf[ index++ ] << 4;

        }
        else if ( m_metaLen )
        {
            m_metaData.append( m_pBuf[ index++ ] );
            --m_metaLen;

            if ( !m_metaLen )
            {
                transmitData( m_metaData );
                m_metaData = "";

            }

        }
        else
        {
            bytesWrite = bytesRead - index;

            if ( bytesWrite > m_metaInt - m_byteCount )
                bytesWrite = m_metaInt - m_byteCount;

            m_pSockProxy->writeBlock( m_pBuf + index, bytesWrite );

            index += bytesWrite;
            m_byteCount += bytesWrite;

        }

    }
}


void Proxy::processHeader(Q_LONG &index, Q_LONG bytesRead)
{
    while ( index < bytesRead )
    {
        m_headerStr.append( m_pBuf[ index++ ] );
        if ( m_headerStr.endsWith( "\r\n\r\n" ) )
        {

            /*kdDebug() << k_funcinfo <<
                  "Got shoutcast header: '" << m_headerStr << "'" << endl;*/
            /*
            TODO: emit error including helpful error message on headers like this:

            ICY 401 Service Unavailable
            icy-notice1:<BR>SHOUTcast Distributed Network Audio Server/Linux v1.9.2<BR>
            icy-notice2:The resource requested is currently unavailable<BR>
            */
            m_metaInt = m_headerStr.section( "icy-metaint:", 1, 1,
                                             QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0)
                        .toInt();
            m_bitRate = m_headerStr.section( "icy-br:", 1, 1,
                                             QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0).toInt();
            m_streamName = m_headerStr.section( "icy-name:", 1, 1,
                                                QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0);
            m_streamGenre = m_headerStr.section( "icy-genre:", 1, 1,
                                                 QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0);
            m_streamUrl = m_headerStr.section( "icy-url:", 1, 1,
                                               QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0);
            if(m_streamUrl.startsWith("www.", true))
                m_streamUrl.prepend("http://");
            m_pSockProxy->writeBlock(m_headerStr.latin1(), m_headerStr.length());
            m_headerFinished = true;

            if ( !m_metaInt )
                emit error();
            break;

        }

    }
}


void Proxy::transmitData(const QString &data)
{
    /*kdDebug() << k_funcinfo <<
      "received new metadata: '" << data << "'" << endl;*/

    MetaBundle bundle( extractStr( data, "StreamTitle" ),
                       m_streamGenre,
                       m_bitRate );
    
    emit metaData( bundle );

/*    packet.title       = extractStr( data, "StreamTitle" );
    packet.url         = extractStr( data, "StreamUrl" );
    packet.bitRate     = m_bitRate;
    packet.streamGenre = m_streamGenre;
    packet.streamName  = m_streamName;
    packet.streamUrl   = m_streamUrl;*/
}


QString Proxy::extractStr(const QString &str, const QString &key)
{
    int index = str.find(key, 0, true);
    if(index == -1)
    {
        return QString::null;

    }
    else
    {
        index = str.find( "'", index ) + 1;
        int indexEnd = str.find( "'", index );
        return str.mid( index, indexEnd - index );

    }
}

#include "titleproxy.moc"
