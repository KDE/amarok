/***************************************************************************
                      titleproxy.cpp  -  description
                         -------------------
begin                : Nov 20 14:35:18 CEST 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <titleproxy.h>

#include <string.h>

#include <kdebug.h>
#include <kextsock.h>
#include <kurl.h>

#include <qobject.h>
#include <qsocketnotifier.h>
#include <qstring.h>

#define PROXYPORT 6666     //FIXME port should not be hardcoded. what to do if it's in use?
#define BUFSIZE 8192

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


TitleProxy::TitleProxy( KURL url ) :
        QObject(),
        m_initSuccess( 0 ),
        m_metaInt( -1 ),
        m_metaLen( 0 ),
        m_pBufIn( NULL ),
        m_pBufOut( NULL ),
        m_pSockServer( NULL )
{
    m_urlRemote = url;

    m_sockRemote.setSocketFlags( KExtendedSocket::inetSocket |
                                 KExtendedSocket::bufferedSocket |
                                 KExtendedSocket::streamSocket );
    
    m_sockRemote.setAddress( url.host(), url.port() );

    int connectResult = m_sockRemote.connect();
    kdDebug() << "sock.connect() result: " << connectResult << endl;

    if ( connectResult != 0 )
        return;

    m_sockLocal.setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::passiveSocket );
    m_sockLocal.setAddress( "localhost", PROXYPORT );
    int listenResult = m_sockLocal.listen();

    kdDebug() << "sock.listen() result: " << listenResult << endl;

    if ( listenResult != 0 )
        return;

    m_pBufIn = new char[ BUFSIZE ];
    m_pBufOut = new char[ BUFSIZE ];

    connect( &m_sockLocal, SIGNAL( readyAccept() ), this, SLOT( accept() ) );

    m_initSuccess = true;
}


TitleProxy::~TitleProxy()
{
    delete[] m_pBufIn;
    delete[] m_pBufOut;
    delete m_pSockServer;
}


KURL TitleProxy::proxyUrl()
{
    if ( m_initSuccess )
    {
        KURL url;
        url.setHost( "localhost" );
        url.setPort( PROXYPORT );
        url.setProtocol( "http" );
        return url;
    }
    else
        return m_urlRemote;
}


void TitleProxy::accept()
{
    kdDebug() << "TitleProxy::accept()" << endl;

    int acceptResult = m_sockLocal.accept( m_pSockServer );
    m_pSockServer->setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::bufferedSocket );
    kdDebug() << "acceptResult: " << acceptResult << endl;

    int bytesRead = m_pSockServer->readBlock( m_pBufIn, BUFSIZE );
    kdDebug() << "TitleProxy::readLocal() received block: " << endl << QCString( m_pBufIn, bytesRead ) << endl;

    m_pSockServer->setBlockingMode( false );

    QString str = QString::fromAscii( m_pBufIn, bytesRead );
    int index = str.find( "GET / HTTP/1.1" );
    index = str.find( "\n", index );
    index++;

    m_sockRemote.setBlockingMode( false );
    m_sockRemote.setBufferSize( 128 * 1024 );
    m_sockRemote.enableRead( true );
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
    m_sockRemote.writeBlock( m_pBufIn, index );

    QCString icyStr( "Icy-MetaData:1\r\n" );
    m_sockRemote.writeBlock( icyStr.data(), icyStr.length() );

    m_sockRemote.writeBlock( m_pBufIn + index, bytesRead - index );
}


void TitleProxy::readRemote()
{
    int indexMp3 = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_pBufIn, BUFSIZE );
    //    kdDebug() << "TitleProxy::readRemote(): bytesRead = " << bytesRead << endl;

    if ( bytesRead > 0 )
    {
        //<Detect MetaInt>
        if ( m_metaInt == -1 )
        {
            QString str = QString::fromAscii( m_pBufIn, bytesRead );
            QString icyStr( "icy-metaint:" );
            int index = str.find( icyStr, 0, false );

            if ( index != -1 )
            {
                int indexEnd = str.find( "\r", index );
                index += icyStr.length();
                m_metaInt = str.mid( index,  indexEnd - index ).toInt();
                kdDebug() << "m_metaInt: " << m_metaInt << "\n" << endl;

                indexMp3 = str.find( "\r\n\r\n", index );
                indexMp3 += 4;

                m_byteCount = 0;
                m_pSockServer->writeBlock( m_pBufIn, indexMp3 );
            }
        }
        //</Detect MetaInt>

        char *pOut = m_pBufOut;
        int bytesWrite = 0;

        while ( indexMp3 < bytesRead )
        {
            if ( m_byteCount == m_metaInt )
            {
                m_metaLen = (unsigned char) m_pBufIn[ indexMp3++ ];
                m_metaLen *= 16;

                if ( m_metaLen ) kdDebug() << "m_metaLen: " << m_metaLen << "\n" << endl;
                m_byteCount = 0;
                continue;
            }

            if ( m_metaLen )
            {
                m_metaData.append( m_pBufIn[ indexMp3++ ] );
                --m_metaLen;

                if ( !m_metaLen )
                {
                    transmitData( m_metaData );
                    m_metaData = "";
                }
                continue;
            }

            *pOut = m_pBufIn[ indexMp3++ ];
            ++pOut;

            ++m_byteCount;
            ++bytesWrite;
        }

        m_pSockServer->writeBlock( m_pBufOut, bytesWrite );
    }
}


void TitleProxy::transmitData( QString data )
{
    QString title = extractStr( data, "StreamTitle" );
    QString url = extractStr( data, "StreamUrl" );

    kdDebug() << "title: " << title << "\n" << endl;
    kdDebug() << "url: " << url << "\n" << endl;

    emit metaData( title, url );
}


QString TitleProxy::extractStr( QString str, QString key )
{
    int index = str.find( key, 0 );

    if ( index == -1 )
    {
        return QString();
    }
    else
    {
        index = str.find( "'", index );
        index++;
        int indexEnd = str.find( "'", index );
        return str.mid( index, indexEnd - index );
    }
}

