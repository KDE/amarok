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

#include <qcstring.h> 
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qstring.h>

#define PROXYPORT 6666     //FIXME port should not be hardcoded. what to do if it's in use?
#define BUFSIZE 5000

// Some info on the protocol can be found on: 
// http://www.smackfu.com/stuff/programming/shoutcast.html


TitleProxy::TitleProxy( KURL url ) : 
        QObject(),
        m_initSuccess( 0 ),
        m_metaInt( -1 ),
        m_pSockServer( NULL )
{
    m_urlRemote = url;    
    
    m_sockRemote.setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::bufferedSocket );
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

    connect( &m_sockLocal, SIGNAL( readyAccept() ), this, SLOT( accept() ) );
 
    m_initSuccess = true;
}


TitleProxy::~TitleProxy()
{
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

    m_bufIn.resize( BUFSIZE );
    m_bufOut.resize( BUFSIZE );
        
    int acceptResult = m_sockLocal.accept( m_pSockServer );        
    m_pSockServer->setSocketFlags( KExtendedSocket::inetSocket | KExtendedSocket::bufferedSocket );
    kdDebug() << "acceptResult: " << acceptResult << endl;
    
    int bytesRead = m_pSockServer->readBlock( m_bufIn.data(), BUFSIZE );
    kdDebug() << "TitleProxy::readLocal() received block: " << endl << QCString( m_bufIn, bytesRead ) << endl;

    m_pSockServer->setBlockingMode( false );

    QString str = QString::fromAscii( m_bufIn.data(), bytesRead );
    int index = str.find( "GET / HTTP/1.1" );
    index = str.find( "\n", index );
    index++;    
    
    m_sockRemote.setBlockingMode( false );
    m_sockRemote.setBufferSize( BUFSIZE );
    m_sockRemote.enableRead( true );
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );
    m_sockRemote.writeBlock( m_bufIn.data(), index );
    
    QCString icyStr( "Icy-MetaData:1\r\n" );
    m_sockRemote.writeBlock( icyStr.data(), icyStr.length() );

    m_sockRemote.writeBlock( m_bufIn.data() + index, bytesRead - index );
}


void TitleProxy::readRemote()
{
    int indexMp3 = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_bufIn.data(), BUFSIZE );
//    kdDebug() << "TitleProxy::readRemote(): bytesRead = " << bytesRead << endl;
        
    if ( bytesRead > 0 )
    {
        //<Detect MetaInt>
        if ( m_metaInt == -1 )
        {
            QString str = QString::fromAscii( m_bufIn.data(), bytesRead );
            QString icyStr( "icy-metaint:" );
            int index = str.find( icyStr, 0, false );

            if ( index != -1 )
            {        
                int indexEnd = str.find( "\r", index ); 
                index += icyStr.length();
                m_metaInt = str.mid( index,  indexEnd - index ).toInt();
                kdDebug() << "m_metaInt: " << m_metaInt << endl;
            
                indexMp3 = str.find( "\r\n\r\n", index );
                indexMp3 += 4;

                m_byteCount = 0;            
                m_pSockServer->writeBlock( m_bufIn.data(), indexMp3 );
            }
        }
        //</Detect MetaInt>
                
        QByteArray::Iterator itOut( m_bufOut.begin() );
        int bytesWrite = 0;
                        
        while ( indexMp3 < bytesRead )
        {
            if ( m_byteCount == m_metaInt )
            {
                int metaLen = m_bufIn.at( indexMp3++ ) * 16;
                
                if ( metaLen )
                {
//                     kdDebug() << "metadata-length:" << metaLen << endl;
                    QString metaData;
                                
                    for ( int i = 0; i < metaLen; i++ )
                    {    
                        if ( indexMp3 >= bytesRead )
                            kdDebug() << "FIXME indexMp3 >= bytesRead !!" << endl;
                        metaData.append( m_bufIn.at( indexMp3++ ) );
                    }
                    transmitData( metaData );
                }
                
                m_byteCount = 0;
                continue;
            }
            
            *itOut = m_bufIn.at( indexMp3 );
            ++itOut;
            
            ++m_byteCount;
            ++indexMp3;
            ++bytesWrite;
        }
        
        m_pSockServer->writeBlock( m_bufOut.data(), bytesWrite );
    }
}


void TitleProxy::transmitData( QString data )
{
    kdDebug() << "metaData:" << data << endl;

    QString title = extractStr( data, "StreamTitle" );
    QString url = extractStr( data, "StreamUrl" );

    kdDebug() << "title: " << title << endl;
    kdDebug() << "url: " << url << endl;
    
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
