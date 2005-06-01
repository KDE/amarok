/***************************************************************************
                     streamprovider.cpp - shoutcast streaming client
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

#define DEBUG_PREFIX "StreamProvider"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "metabundle.h"
#include "statusbar.h"
#include "streamprovider.h"

#include <qregexp.h>
#include <qtextcodec.h>
#include <qtimer.h>

#include <klocale.h>
#include <kmdcodec.h>
#include <kprotocolmanager.h>

using namespace amaroK;


StreamProvider::StreamProvider( KURL url, const QString& streamingMode )
        : QObject()
        , m_url( url )
        , m_streamingMode( streamingMode )
        , m_initSuccess( true )
        , m_metaInt( 0 )
        , m_byteCount( 0 )
        , m_metaLen( 0 )
        , m_usedPort( 0 )
        , m_pBuf( new char[BUFSIZE] )
{
    DEBUG_BLOCK

    // Don't try to get metdata for ogg streams (different protocol)
    m_icyMode = url.path().endsWith( ".ogg" ) ? false : true;
    // If no port is specified, use default shoutcast port
    if ( !m_url.port() ) m_url.setPort( 80 );

    connect( &m_sockRemote, SIGNAL( error( int ) ),                 SLOT( connectError() ) );
    connect( &m_sockRemote, SIGNAL( connected() ),                  SLOT( sendRequest() ) );
    connect( &m_sockRemote, SIGNAL( readyRead() ),                  SLOT( readRemote() ) );
    connect( &m_resolver,   SIGNAL( finished( KResolverResults ) ), SLOT( resolved( KResolverResults ) ) );

    if ( streamingMode == "Socket" ) {
        uint i;
        StreamProxy* server;
        for ( i = MIN_PROXYPORT; i <= MAX_PROXYPORT; i++ ) {
            server = new StreamProxy( i, this );
            debug() << "Trying to bind to port: " << i << endl;
            if ( server->ok() )     // found a free port
                break;
            delete server;
        }
        if ( i > MAX_PROXYPORT ) {
            warning() << "Unable to find a free local port. Aborting.\n";
            m_initSuccess = false;
            return;
        }
        m_usedPort = i;
        connect( server, SIGNAL( connected( int ) ), this, SLOT( accept( int ) ) );
    }
    else
        connectToHost();
}


StreamProvider::~StreamProvider()
{
    DEBUG_FUNC_INFO

    delete[] m_pBuf;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

KURL
StreamProvider::proxyUrl()
{
    if ( m_initSuccess ) {
        KURL url;
        url.setPort( m_usedPort );
        url.setHost( "127.0.0.1" );
        url.setProtocol( "http" );
        return url;

    } else
        return m_url;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
StreamProvider::accept( int socket ) //SLOT
{
    DEBUG_FUNC_INFO

    m_sockProxy.setSocket( socket );
    m_sockProxy.waitForMore( KProtocolManager::proxyConnectTimeout() * 1000 );

    connectToHost();
}


void
StreamProvider::connectToHost() //SLOT
{
    DEBUG_BLOCK

    { //initialisations
        m_connectSuccess = false;
        m_headerFinished = false;
        m_headerStr = "";
    }

    { //connect to server
        QTimer::singleShot( KProtocolManager::connectTimeout() * 1000, this, SLOT( connectError() ) );
        m_resolver.setNodeName( m_url.host() );
        m_resolver.setFamily( KResolver::InetFamily );
        m_resolver.start();
    }
}


void
StreamProvider::resolved( KResolverResults result) // SLOT
{
    DEBUG_BLOCK

    if ( result.error() != KResolver::NoError || result.isEmpty() )
        connectError();
    else
        m_sockRemote.connectToHost( result[0].address().asInet().nodeName(), m_url.port() );
}


void
StreamProvider::sendRequest() //SLOT
{
    DEBUG_BLOCK

    const QCString username = m_url.user().utf8();
    const QCString password = m_url.pass().utf8();
    const QString authString = KCodecs::base64Encode( username + ":" + password );
    const bool auth = !( username.isEmpty() && password.isEmpty() );

    // Extract major+minor version number from APP_VERSION
    QRegExp reg( "[0-9]*\\.[0-9]*" );
    reg.search( APP_VERSION );
    const QString version = reg.cap();

    const QString request = QString( "GET %1 HTTP/1.0\r\n"
                                     "Host: %2\r\n"
                                     "User-Agent: amaroK/%3\r\n"
                                     "Accept: */*\r\n"
                                     "%4"
                                     "%5"
                                     "\r\n" )
                                     .arg( m_url.path( -1 ).isEmpty() ? "/" : m_url.path( -1 ) + m_url.query() )
                                     .arg( m_url.host() )
                                     .arg( version )
                                     .arg( m_icyMode ? "Icy-MetaData:1\r\n" : "" )
                                     .arg( auth ? "Authorization: Basic " + authString + "\r\n" : "" );

    debug() << "Sending request:\n" << request << endl;
    m_sockRemote.writeBlock( request.latin1(), request.length() );
}


void
StreamProvider::readRemote() //SLOT
{
    m_connectSuccess = true;
    Q_LONG index = 0;
    Q_LONG bytesWrite = 0;
    const Q_LONG bytesRead = m_sockRemote.readBlock( m_pBuf, BUFSIZE );
    if ( bytesRead == -1 ) { emit sigError(); return; }

    if ( !m_headerFinished )
        if ( !processHeader( index, bytesRead ) ) return;

    // This is the main loop which processes the stream data
    while ( index < bytesRead ) {
        if ( m_icyMode && m_metaInt && ( m_byteCount == m_metaInt ) ) {
            m_byteCount = 0;
            m_metaLen = m_pBuf[ index++ ] << 4;
        }
        // Are we in a metadata interval?
        else if ( m_metaLen ) {
            uint length = ( (Q_LONG)m_metaLen > bytesRead - index ) ? ( bytesRead - index ) : ( m_metaLen );
            m_metaData.append( QString::fromAscii( m_pBuf + index, length ) );
            index += length;
            m_metaLen -= length;

            if ( m_metaLen == 0 ) {
                // Transmit metadata string
                transmitData( m_metaData );
                m_metaData = "";
            }
        }
        else {
            bytesWrite = bytesRead - index;

            if ( m_icyMode && bytesWrite > m_metaInt - m_byteCount )
                bytesWrite = m_metaInt - m_byteCount;

            if ( m_streamingMode == "Socket" )
                bytesWrite = m_sockProxy.writeBlock( m_pBuf + index, bytesWrite );
            else
                emit streamData( m_pBuf + index, bytesWrite );

            if ( bytesWrite == -1 ) { restartNoIcy(); return; }

            index += bytesWrite;
            m_byteCount += bytesWrite;
        }
    }
}


void
StreamProvider::connectError() //SLOT
{
    if ( !m_connectSuccess ) {
        warning() << "Unable to connect to this stream server." << endl;
        // it might have been timed out attempt to resolve host
        m_resolver.cancel( false );
        amaroK::StatusBar::instance()->longMessage( i18n( "Unable to connect to this stream server." ), amaroK::StatusBar::Sorry );

        emit sigError();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

bool
StreamProvider::processHeader( Q_LONG &index, Q_LONG bytesRead )
{
    DEBUG_BLOCK

    while ( index < bytesRead ) {
        m_headerStr.append( m_pBuf[ index++ ] );
        if ( m_headerStr.endsWith( "\r\n\r\n" ) ) {
            debug() << "Got shoutcast header: '" << m_headerStr << "'" << endl;
            // Handle redirection
            QString loc( "Location: " );
            int index = m_headerStr.find( loc );
            if ( index >= 0 ) {
                int start = index + loc.length();
                int end = m_headerStr.find( "\n", index );
                m_url = m_headerStr.mid( start, end - start - 1 );
                debug() << "Stream redirected to: " << m_url << endl;
                m_sockRemote.close();
                connectToHost();
                return false;
            }
            m_metaInt = m_headerStr.section( "icy-metaint:", 1, 1, QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 ).toInt();
            m_bitRate = m_headerStr.section( "icy-br:", 1, 1, QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 ).toInt();
            m_streamName = m_headerStr.section( "icy-name:", 1, 1, QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );
            m_streamGenre = m_headerStr.section( "icy-genre:", 1, 1, QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );
            m_streamUrl = m_headerStr.section( "icy-url:", 1, 1, QString::SectionCaseInsensitiveSeps ).section( "\r", 0, 0 );

            if ( m_streamUrl.startsWith( "www.", true ) )
                m_streamUrl.prepend( "http://" );
            if ( m_streamingMode == "Socket" )
                m_sockProxy.writeBlock( m_headerStr.latin1(), m_headerStr.length() );

            m_headerFinished = true;

            if ( m_icyMode && !m_metaInt ) {
                restartNoIcy();
                return false;
            }

            transmitData( QString::null );
            connect( &m_sockRemote, SIGNAL( connectionClosed() ), this, SLOT( connectError() ) );
            return true;
        }
    }
    return false;
}


void
StreamProvider::transmitData( const QString &data )
{
    DEBUG_BLOCK

    debug() << "Received MetaData: " << data << endl;

    // because we assumed latin1 earlier this codec conversion works
    QTextCodec *codec = ( AmarokConfig::recodeShoutcastMetadata() && AmarokConfig::recodeEncoding() )
            ? QTextCodec::codecForIndex( AmarokConfig::recodeEncoding() )
            : QTextCodec::codecForName( "ISO8859-1" ); //Latin1 returns 0

    if ( !codec ) {
        error() << "QTextCodec* codec == NULL!" << endl;
        return;
    }

    const MetaBundle bundle( codec->toUnicode( extractStr( data, "StreamTitle" ).latin1() ),
                             m_streamUrl,
                             m_bitRate,
                             codec->toUnicode( m_streamGenre.latin1() ),
                             codec->toUnicode( m_streamName.latin1() ),
                             m_url );

    emit metaData( bundle );
}


void
StreamProvider::restartNoIcy()
{
    debug() <<  "Stream does not support Shoutcast Icy-MetaData. Restarting in non-Metadata mode.\n";

    m_sockRemote.close();
    m_icyMode = false;

    // Open stream again,  but this time without metadata, please
    connectToHost();
}


QString
StreamProvider::extractStr( const QString &str, const QString &key ) const
{
    int index = str.find( key, 0, true );

    if ( index == -1 )
        return QString::null;

    else {

        // String looks like this:
        // StreamTitle='foobar';StreamUrl='http://shn.mthN.net';

        index = str.find( "'", index ) + 1;
        int indexEnd = str.find( "';", index );
        return str.mid( index, indexEnd - index );
    }
}


#include "streamprovider.moc"

