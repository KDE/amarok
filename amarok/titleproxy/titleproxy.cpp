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

#include <kdebug.h>
#include <kextsock.h>
#include <kurl.h>

#include <qobject.h>
#include <qsocketnotifier.h>
#include <qstring.h>

#define PROXYPORT 6666     //FIXME maybe port should not be hardcoded?
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
        m_metaInt( 0 ),
        m_byteCount( 0 ),
        m_metaLen( 0 ),
        m_headerFinished( false ),
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

    m_sockPassive.setHost( "localhost" );
    m_sockPassive.setPort( PROXYPORT );
    m_sockPassive.setSocketFlags( KExtendedSocket::passiveSocket );
    
    int listenResult = m_sockPassive.listen();
    kdDebug() << "m_sockPassive.listen() result: " << listenResult << endl;

    if ( listenResult != 0 )
        return;

    m_pBuf = new char[ BUFSIZE ];

    connect( &m_sockPassive, SIGNAL( readyAccept() ), this, SLOT( accept() ) );
    m_initSuccess = true;
}


TitleProxy::~TitleProxy()
{
    delete[] m_pBuf;
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
    int acceptResult = m_sockPassive.accept( m_pSockServer );
    m_pSockServer->setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::bufferedSocket );
    kdDebug() << "acceptResult: " << acceptResult << endl;

    int bytesRead = m_pSockServer->readBlock( m_pBuf, BUFSIZE );
    kdDebug() << "TitleProxy::readLocal() received block: " << endl << QCString( m_pBuf, bytesRead ) << endl;
    m_pSockServer->setBlockingMode( false );

    QString str = QString::fromAscii( m_pBuf, bytesRead );
    int index = str.find( "GET / HTTP/1.1" );
    index = str.find( "\n", index );
    index++;
    
    m_sockRemote.setBufferSize( 128 * 1024 );
    m_sockRemote.enableRead( true );
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
    m_sockRemote.writeBlock( m_pBuf, index );

    QString icyStr( "Icy-MetaData:1\r\n" );
    m_sockRemote.writeBlock( icyStr.latin1(), icyStr.length() );
    m_sockRemote.writeBlock( m_pBuf + index, bytesRead - index );
}


void TitleProxy::readRemote()
{
    Q_LONG index = 0, bytesWrite = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_pBuf, BUFSIZE );
    //    kdDebug() << "TitleProxy::readRemote(): bytesRead = " << bytesRead << endl;

    if ( !m_headerFinished )
        processHeader( index, bytesRead );
        
    //This is the main loop which processes the stream data
    while ( index < bytesRead )
    {                
        if ( m_byteCount == m_metaInt )
        {
            m_byteCount = 0;
            m_metaLen = m_pBuf[ index++ ] * 16;
//            if ( m_metaLen ) kdDebug() << "m_metaLen: " << m_metaLen << "\n" << endl;
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
                        
            m_pSockServer->writeBlock( m_pBuf + index, bytesWrite );
        
            index += bytesWrite;
            m_byteCount += bytesWrite;
        }
    }
}        


void TitleProxy::processHeader( Q_LONG &index, Q_LONG bytesRead )
{    
    while ( index < bytesRead )
    {
        m_headerStr.append( m_pBuf[ index++ ] );
                                
        if ( m_headerStr.endsWith( "\r\n\r\n" ) )
        {
            kdDebug() << "DUMP:" << m_headerStr << endl;
            
            m_metaInt = m_headerStr.section( "icy-metaint:", 1, 1,
                        QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0)
                        .toInt();
            
            kdDebug() << "m_metaInt: " << m_metaInt << endl;
            
            m_bitRate = m_headerStr.section( "icy-br:", 1, 1,
                        QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0);
        
            m_pSockServer->writeBlock( m_headerStr.latin1(), m_headerStr.length() );
            m_headerFinished = true;
            
            break;
        }                                                      
    }
}     


void TitleProxy::transmitData( QString data )
{
    QString title = extractStr( data, "StreamTitle" );
    QString url = extractStr( data, "StreamUrl" );

    kdDebug() << "title: " << title << "\n" << endl;
    kdDebug() << "url: " << url << "\n" << endl;

    emit metaData( title, url, m_bitRate );
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
