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

#include "debug.h"
#include <kmdcodec.h>
#include <kprotocolmanager.h>
#include "metabundle.h"
#include <qtimer.h>
#include "streamprovider.h"

// TODO:
// Look at LibVorbisfile for ogg comment decoding.


using namespace amaroK;

static const uint MIN_PROXYPORT = 6700;
static const uint MAX_PROXYPORT = 7777;
static const int BUFSIZE = 16384;


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
    DEBUG_FUNC_INFO

    // Don't try to get metdata for ogg streams (different protocol)
    m_icyMode = url.path().endsWith( ".ogg" ) ? false : true;
    // If no port is specified, use default shoutcast port
    if ( !m_url.port() ) m_url.setPort( 80 );

    connect( &m_sockRemote, SIGNAL( error( int ) ), this, SLOT( connectError() ) );
    connect( &m_sockRemote, SIGNAL( connected() ), this, SLOT( sendRequest() ) );
    connect( &m_sockRemote, SIGNAL( readyRead() ), this, SLOT( readRemote() ) );

    if ( streamingMode == "Socket" ) {
        uint i;
        StreamProxy* server;
        for ( i = MIN_PROXYPORT; i <= MAX_PROXYPORT; i++ ) {
            server = new StreamProxy( i, this );
            kdDebug() << k_funcinfo << "Trying to bind to port: " << i << endl;
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
    DEBUG_BEGIN

    { //initialisations
        m_connectSuccess = false;
        m_headerFinished = false;
        m_headerStr = "";
    }

    { //connect to server
        QTimer::singleShot( KProtocolManager::connectTimeout() * 1000, this, SLOT( connectError() ) );
        m_sockRemote.connectToHost( m_url.host(), m_url.port() );
    }

    DEBUG_END
}


void
StreamProvider::sendRequest() //SLOT
{
    DEBUG_BEGIN

    QCString username = m_url.user().utf8();
    QCString password = m_url.pass().utf8();
    QString authString = KCodecs::base64Encode( username + ":" + password );
    bool auth = !( username.isEmpty() && password.isEmpty() );

    QString request = QString( "GET %1\r\n"
                               "Host: %2\r\n"
                               "User-Agent: amaroK/1.2\r\n"
                               "%3"
                               "%4"
                               "\r\n" )
                               .arg( m_url.path( -1 ).isEmpty() ? "/" : m_url.path( -1 ) + m_url.query() )
                               .arg( m_url.host() )
                               .arg( m_icyMode ? "Icy-MetaData:1\r\n" : "" )
                               .arg( auth ? "Authorization: Basic " + authString + "\r\n" : "" );

    kdDebug() << "StreamProvider sending request:\n" << request << endl;
    m_sockRemote.writeBlock( request.latin1(), request.length() );

    DEBUG_END
}


void
StreamProvider::readRemote() //SLOT
{
    m_connectSuccess = true;
    Q_LONG index = 0;
    Q_LONG bytesWrite = 0;
    Q_LONG bytesRead = m_sockRemote.readBlock( m_pBuf, BUFSIZE );
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
            uint length = ( m_metaLen > bytesRead - index ) ? ( bytesRead - index ) : ( m_metaLen );
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

            if ( bytesWrite == -1 ) { error(); return; }

            index += bytesWrite;
            m_byteCount += bytesWrite;
        }
    }
}


void
StreamProvider::connectError() //SLOT
{
    if ( !m_connectSuccess ) {
        kdError() << "StreamProvider error: Unable to connect to this stream server. Can't play the stream!\n";

        emit sigError();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

bool
StreamProvider::processHeader( Q_LONG &index, Q_LONG bytesRead )
{
    DEBUG_FUNC_INFO

    while ( index < bytesRead ) {
        m_headerStr.append( m_pBuf[ index++ ] );
        if ( m_headerStr.endsWith( "\r\n\r\n" ) ) {
            kdDebug() << k_funcinfo << "Got shoutcast header: '" << m_headerStr << "'" << endl;
            // Handle redirection
            QString loc( "Location: " );
            int index = m_headerStr.find( loc );
            if ( index >= 0 ) {
                int start = index + loc.length();
                int end = m_headerStr.find( "\n", index );
                m_url = m_headerStr.mid( start, end - start - 1 );
                kdDebug() << "Stream redirected to: " << m_url << endl;
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
                error();
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
    kdDebug() << "[StreamProvider] Received new metadata: " << data << endl;

    // Prevent spamming by ignoring repeated identical data (some servers repeat it every 10 seconds)
    if ( !data.isNull() && data == m_lastMetadata ) return;
    m_lastMetadata = data;

    MetaBundle bundle( extractStr( data, "StreamTitle" ),
                       m_streamUrl,
                       m_bitRate,
                       m_streamGenre,
                       m_streamName,
                       m_url );

    emit metaData( bundle );
}


void
StreamProvider::error()
{
    kdDebug() <<  "StreamProvider error: Stream does not support shoutcast metadata. "
                  "Restarting in non-metadata mode.\n";

    m_sockRemote.close();
    m_icyMode = false;

    // Open stream again,  but this time without metadata, please
    connectToHost();
}


QString
StreamProvider::extractStr( const QString &str, const QString &key ) const
{
    int index = str.find( key, 0, true );

    if ( index == -1 ) {
        return QString::null;

    } else {

        // String looks like this:
        // StreamTitle='foobar';StreamUrl='http://shn.mthN.net';

        index = str.find( "'", index ) + 1;
        int indexEnd = str.find( "';", index );
        return str.mid( index, indexEnd - index );

    }
}


#include "streamprovider.moc"

