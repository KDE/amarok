/***************************************************************************
 * copyright            : (C) 2006 Chris Muehlhaeuser <chris@chris.de>     *
 *                      : (C) 2006 Seb Ruiz <me@sebruiz.net>               *
 *                      : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "amarok.h" //APP_VERSION
#include "amarokconfig.h" //AS username and passwd
#include "debug.h"
#include "lastfmproxy.h"
#include "enginecontroller.h"

#include <qdom.h>
#include <qfile.h>
#include <qhttp.h>
#include <qregexp.h>
#include <qsocket.h>
#include <qtimer.h>

#include <kio/job.h> //KIO::get
#include <kio/jobclasses.h> //KIO::Job
#include <kio/netaccess.h> //synchronousRun
#include <kmdcodec.h> //md5sum
#include <kprotocolmanager.h>
#include <kurl.h>

#include <time.h>
#include <unistd.h>

LastFm::Controller::Controller()
    : QObject( EngineController::instance(), "lastfmController" ),
    m_playing( false ),
    m_service( 0 ),
    m_server( 0 )
{ }

LastFm::Controller::~Controller()
{ } //m_service and m_server are both qobject children

LastFm::Controller*
LastFm::Controller::instance()
{
    static Controller* control = new Controller();
    return control;
}

KURL
LastFm::Controller::getNewProxy(QString genreUrl)
{
    DEBUG_BLOCK

    m_genreUrl = genreUrl;

    if ( m_playing )
       return KURL(m_server->getProxyUrl());
    m_service = new WebService(this);
    m_service->handshake(  AmarokConfig::scrobblerUsername(),  AmarokConfig::scrobblerPassword() );
    connect( m_service, SIGNAL ( handshakeResult( int ) ), this, SLOT( handshakeFinished() ) );
    m_server = m_service->getServer();
    KURL streamUrl = m_server->getProxyUrl();
    m_playing=true;
    return streamUrl;
}

void
LastFm::Controller::handshakeFinished()
{
    DEBUG_BLOCK

    disconnect( m_service, SIGNAL ( handshakeResult( int ) ), this, SLOT( handshakeFinished() ) );
    connect( m_service, SIGNAL( stationChanged( QString, QString ) ), this, SLOT( initialGenreSet() ) );
    m_service->changeStation( m_genreUrl );
}

void
LastFm::Controller::initialGenreSet()
{
    DEBUG_BLOCK
   //we only want to do this function once for each new m_server
   disconnect( m_service, SIGNAL( stationChanged( QString, QString ) ), this, SLOT( initialGenreSet() ) ); 
   m_server->loadStream( m_service->streamUrl() );//we only want the first time
}

void
LastFm::Controller::playbackStopped()
{ 
    m_playing = false;
    delete m_service; m_service = 0;
    delete m_server; m_server = 0;
}

LastFm::WebService::WebService( QObject* parent )
    : QObject( parent, "lastfmParent" )
    , m_server( 0 )
{
    debug() << "Initialising Web Service" << endl;
}

void
LastFm::WebService::handshake( const QString& username, const QString& password )
{
    DEBUG_BLOCK

    m_username = username;
    m_password = password;

    QHttp *http = new QHttp( "ws.audioscrobbler.com", 80, this );
    connect( http, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ),
             this,   SLOT( handshakeHeaderReceived( const QHttpResponseHeader& ) ) );

    connect( http, SIGNAL( requestFinished( int, bool ) ),
             this,   SLOT( handshakeFinished( int, bool ) ) );

    QString path =
            QString( "/radio/handshake.php?version=%1&platform=%2&username=%3&passwordmd5=%4&debug=%5" )
            .arg( APP_VERSION )             //Muesli-approved: Amarok version, and Amarok-as-platform
            .arg( QString("Amarok") )
            .arg( QString( QUrl( username ).encodedPathAndQuery() ) )
            .arg( KMD5( m_password.utf8() ).hexDigest() )
            .arg( "0" );

    http->get( path );
    m_lastHttp = http;

    m_server = new Server( this );
}


void
LastFm::WebService::handshakeHeaderReceived( const QHttpResponseHeader &resp )
{
    if ( resp.statusCode() == 503 )
    {
        debug() << "Handshake error" << endl;
    }
}


void
LastFm::WebService::handshakeFinished( int /*id*/, bool error )
{
    DEBUG_BLOCK

    if ( error )
    {
        emit handshakeResult( -1 );
        return;
    }

    QString result( m_lastHttp->readAll() );

    debug() << "result: " << result << endl;

    m_session = parameter( "session", result );
    m_baseHost = parameter( "base_url", result );
    m_basePath = parameter( "base_path", result );
    m_subscriber = parameter( "subscriber", result ) == "1";
    m_streamUrl = QUrl( parameter( "stream_url", result ) );
//     bool banned = parameter( "banned", result ) == "1";

    if ( m_session.lower() == "failed" )
    {
        emit handshakeResult( 0 );
        return;
    }

    emit streamingUrl( m_streamUrl );
    emit handshakeResult( m_session.length() == 32 ? 1 : -1 );
}

void
LastFm::WebService::changeStation( QString url )
{
    DEBUG_BLOCK
    debug() << "Changing station:" << url << endl;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( changeStationFinished( int, bool ) ) );

    if ( url.startsWith( "lastfm://" ) )
        url.remove( 0, 9 ); // get rid of it!

    http->get( QString( m_basePath + "/adjust.php?session=%1&url=lastfm://%2&debug=%3" )
                  .arg( m_session )
                  .arg( url.contains( "%" ) ? url : QUrl( url ).toString(true, false) )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFm::WebService::changeStationFinished( int /*id*/, bool error )
{
    DEBUG_BLOCK

    if( error ) return;

    QString result( m_lastHttp->readAll() );
    int errCode = parameter( "error", result ).toInt();
    if ( errCode <= 0 )
    {
        QString url = parameter( "url", result );
        if ( url.startsWith( "lastfm://" ) )
        {
            QString stationName = parameter( "stationname", result );
            if ( stationName.isEmpty() )
                stationName = url;
            emit stationChanged( url, stationName );
        }
        else
            emit stationChanged( url, QString::null );
    }
}


void
LastFm::WebService::requestMetaData()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( metaDataFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/np.php?session=%1&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFm::WebService::metaDataFinished( int /*id*/, bool error )
{
    if( error )
        return;

    QString result( m_lastHttp->readAll() );

    MetaBundle song;
    song.setArtist( parameter( "artist", result ) );
    song.setAlbum( parameter( "album", result ) );
    song.setTitle( parameter( "track", result ) );
//     song.setCover( parameter( "albumcover_medium", result ) );
//     song.setArtistUrl( parameter( "artist_url", result ) );
//     song.setAlbumUrl( parameter( "album_url", result ) );
//     song.setTrackUrl( parameter( "track_url", result ) );
    song.setLength( parameter( "trackduration", result ).toInt() );
//     bool discovery = parameter( "discovery", result ) != "-1";

    int errCode = parameter( "error", result ).toInt();
    if ( errCode > 0 )
        return;

    emit metaDataResult( song );
}


void
LastFm::WebService::enableScrobbling( bool enabled )
{
    if ( enabled )
        debug() << "Enabling Scrobbling!" << endl;
    else
        debug() << "Disabling Scrobbling!" << endl;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( enableScrobblingFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=%2&debug=%3" )
                  .arg( m_session )
                  .arg( enabled ? QString( "rtp" ) : QString( "nortp" ) )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFm::WebService::enableScrobblingFinished( int /*id*/, bool error )
{
    if ( error )
        return;

    emit enableScrobblingDone();
}


void
LastFm::WebService::love()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=love&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );

    m_lastHttp = http;
}


void
LastFm::WebService::skip()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( skipFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=skip&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
LastFm::WebService::ban()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=ban&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFm::WebService::loveFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit loveDone();
}


void
LastFm::WebService::skipFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit skipDone();
}


void
LastFm::WebService::banFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit banDone();
    emit skipDone();
}


void
LastFm::WebService::friends( QString username )
{
    if ( username.isEmpty() )
        username = m_username;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( friendsFinished( bool ) ) );

    http->get( QString( "/1.0/user/%1/friends.xml" )
                  .arg( QString( QUrl( username ).encodedPathAndQuery() ) ) );
    m_lastHttp = http;
}


void
LastFm::WebService::friendsFinished( int /*id*/, bool error )
{
    if( error ) return;

    QDomDocument document;
    document.setContent( m_lastHttp->readAll() );

    if ( document.elementsByTagName( "friends" ).length() == 0 )
    {
        emit friendsResult( QString( "" ), QStringList() );
        return;
    }

    QStringList friends;
    QString user = document.elementsByTagName( "friends" ).item( 0 ).attributes().namedItem( "user" ).nodeValue();
    QDomNodeList values = document.elementsByTagName( "user" );
    for ( uint i = 0; i < values.count(); i++ )
    {
        friends << values.item( i ).attributes().namedItem( "username" ).nodeValue();
    }

    emit friendsResult( user, friends );
}


void
LastFm::WebService::neighbours( QString username )
{
    if ( username.isEmpty() )
        username = m_username;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( neighboursFinished( bool ) ) );

    http->get( QString( "/1.0/user/%1/neighbours.xml" )
                  .arg( QString( QUrl( username ).encodedPathAndQuery() ) ) );
    m_lastHttp = http;
}


void
LastFm::WebService::neighboursFinished( int /*id*/, bool error )
{
    if( error )  return;

    QDomDocument document;
    document.setContent( m_lastHttp->readAll() );

    if ( document.elementsByTagName( "neighbours" ).length() == 0 )
    {
        emit friendsResult( QString( "" ), QStringList() );
        return;
    }

    QStringList neighbours;
    QString user = document.elementsByTagName( "neighbours" ).item( 0 ).attributes().namedItem( "user" ).nodeValue();
    QDomNodeList values = document.elementsByTagName( "user" );
    for ( uint i = 0; i < values.count(); i++ )
    {
        neighbours << values.item( i ).attributes().namedItem( "username" ).nodeValue();
    }

    emit neighboursResult( user, neighbours );
}


void
LastFm::WebService::userTags( QString username )
{
    if ( username.isEmpty() )
        username = m_username;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( userTagsFinished( bool ) ) );

    http->get( QString( "/1.0/user/%1/tags.xml?debug=%2" )
                  .arg( QString( QUrl( username ).encodedPathAndQuery() ) ) );
    m_lastHttp = http;
}


void
LastFm::WebService::userTagsFinished( int /*id*/, bool error )
{
    if( error ) return;

    QDomDocument document;
    document.setContent( m_lastHttp->readAll() );

    if ( document.elementsByTagName( "toptags" ).length() == 0 )
    {
        emit userTagsResult( QString(), QStringList() );
        return;
    }

    QStringList tags;
    QDomNodeList values = document.elementsByTagName( "tag" );
    QString user = document.elementsByTagName( "toptags" ).item( 0 ).attributes().namedItem( "user" ).nodeValue();
    for ( uint i = 0; i < values.count(); i++ )
    {
        QDomNode item = values.item( i ).namedItem( "name" );
        tags << item.toElement().text();
    }
    emit userTagsResult( user, tags );
}


void
LastFm::WebService::recentTracks( QString username )
{
    if ( username.isEmpty() )
        username = m_username;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( recentTracksFinished( bool ) ) );

    http->get( QString( "/1.0/user/%1/recenttracks.xml" )
                  .arg( QString( QUrl( username ).encodedPathAndQuery() ) ) );
    m_lastHttp = http;
}


void
LastFm::WebService::recentTracksFinished( int /*id*/, bool error )
{
    if( error ) return;

    QValueList< QPair<QString, QString> > songs;
    QDomDocument document;
    document.setContent( m_lastHttp->readAll() );

    if ( document.elementsByTagName( "recenttracks" ).length() == 0 )
    {
        emit recentTracksResult( QString(), songs );
        return;
    }

    QDomNodeList values = document.elementsByTagName( "track" );
    QString user = document.elementsByTagName( "recenttracks" ).item( 0 ).attributes().namedItem( "user" ).nodeValue();
    for ( uint i = 0; i < values.count(); i++ )
    {
        QPair<QString, QString> song;
        song.first = values.item( i ).namedItem( "artist" ).toElement().text();
        song.second = values.item( i ).namedItem( "name" ).toElement().text();

        songs << song;
    }
    emit recentTracksResult( user, songs );
}


void
LastFm::WebService::recommend( int type, QString username, QString artist, QString token )
{
    QString modeToken = "";
    switch ( type )
    {
        case 0:
            modeToken = QString( "artist_name=%1" ).arg( QString( QUrl( artist ).encodedPathAndQuery() ) );
            break;

        case 1:
            modeToken = QString( "album_artist=%1&album_name=%2" )
                           .arg( QString( QUrl( artist ).encodedPathAndQuery() ) )
                           .arg( QString( QUrl( token ).encodedPathAndQuery() ) );
            break;

        case 2:
            modeToken = QString( "track_artist=%1&track_name=%2" )
                           .arg( QString( QUrl( artist ).encodedPathAndQuery() ) )
                           .arg( QString( QUrl( token ).encodedPathAndQuery() ) );
            break;
    }

    QHttp *http = new QHttp( "wsdev.audioscrobbler.com", 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( recommendFinished( bool ) ) );

    uint currentTime = QDateTime::currentDateTime( Qt::UTC ).toTime_t();
    QString challenge = QString::number( currentTime );

    QCString md5pass = KMD5( KMD5( m_password.utf8() ).hexDigest() + currentTime ).hexDigest();

    QString token = QString( "user=%1&auth=%2&nonce=%3recipient=%4" )
                       .arg( QString( QUrl( currentUsername() ).encodedPathAndQuery() ) )
                       .arg( QString( QUrl( md5pass ).encodedPathAndQuery() ) )
                       .arg( QString( QUrl( challenge ).encodedPathAndQuery() ) )
                       .arg( QString( QUrl( username ).encodedPathAndQuery() ) );

    QHttpRequestHeader header( "POST", "/1.0/rw/recommend.php?" + token.utf8() );
    header.setValue( "Host", "wsdev.audioscrobbler.com" );
    header.setContentType( "application/x-www-form-urlencoded" );
    http->request( header, modeToken.utf8() );

    m_lastHttp = http;
}


void
LastFm::WebService::recommendFinished( int /*id*/, bool /*error*/ )
{
    debug() << "Recommendation:" << m_lastHttp->readAll() << endl;
}


QString
LastFm::WebService::parameter( QString keyName, QString data )
{
    QStringList list = QStringList::split( '\n', data );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( '=', list[i] );
        if ( values[0] == keyName )
        {
            values.remove( values.at(0) );
            return QString().fromUtf8( values.join( "=" ).ascii() );
        }
    }

    return QString( "" );
}


QStringList
LastFm::WebService::parameterArray( QString keyName, QString data )
{
    QStringList result;
    QStringList list = QStringList::split( '\n', data );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( '=', list[i] );
        if ( values[0].startsWith( keyName ) )
        {
            values.remove( values.at(0) );
            result.append( QString().fromUtf8( values.join( "=" ).ascii() ) );
        }
    }

    return result;
}


QStringList
LastFm::WebService::parameterKeys( QString keyName, QString data )
{
    QStringList result;
    QStringList list = QStringList::split( '\n', data );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( '=', list[i] );
        if ( values[0].startsWith( keyName ) )
        {
            values = QStringList::split( '[', values[0] );
            values = QStringList::split( ']', values[1] );
            result.append( values[0] );
        }
    }

    return result;
}


LastFm::Server::Server( QObject* parent )
    : QObject(parent, "lastfmProxyServer")
{
    DEBUG_BLOCK
    m_sockProxy - new QSocket( this, "socketLastFmProxy" );

    StreamProxy* server = new StreamProxy( this );
    m_proxyPort = server->port();
    connect( server, SIGNAL( connected( int ) ), this, SLOT( accept( int ) ) );
}

LastFm::Server::~Server()
{
    delete m_buffer;
}

void
LastFm::Server::loadStream( QUrl remote )
{
    DEBUG_BLOCK

    m_remoteUrl = remote;
    debug() << m_remoteUrl.host() << ':' << m_remoteUrl.port() << m_remoteUrl.encodedPathAndQuery() << endl;
    debug() << m_remoteUrl.toString( true, false ) << endl;
    m_http = new QHttp ( m_remoteUrl.host(), m_remoteUrl.port(), this, "lastfmClient" );
    m_buffer = new QByteArray();
    connect( m_http, SIGNAL( readyRead( const QHttpResponseHeader& ) ), this, SLOT( dataAvailable( const QHttpResponseHeader& ) ) );
    connect( m_http, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ), this, SLOT( responseHeaderReceived( const QHttpResponseHeader& ) ) );

    m_http->get( m_remoteUrl.encodedPathAndQuery() );
}

KURL
LastFm::Server::getProxyUrl()
{
    return KURL( QString("http://localhost:%1/theBeard.mp3").arg( m_proxyPort ) );
}

void
LastFm::Server::accept( int socket)
{
    DEBUG_BLOCK
    m_sockProxy->setSocket( socket );
    /*char emptyBuf[2048];
    memset( emptyBuf, 0, sizeof( emptyBuf ) );
    m_sockProxy.writeBlock( emptyBuf, sizeof( emptyBuf ) );*/
    QTextStream proxyStream( m_sockProxy );
    proxyStream << "HTTP/1.0 200 Ok\r\n"
                   "Content-Type: audio/x-mp3; charset=\"utf-8\"\r\n"
                   "\r\n";
    m_sockProxy.waitForMore( KProtocolManager::proxyConnectTimeout() * 1000 );
}

void
LastFm::Server::responseHeaderReceived( const QHttpResponseHeader &resp  )
{
    DEBUG_BLOCK

    debug() << resp.statusCode() << endl;
}

void
LastFm::Server::dataAvailable( const QHttpResponseHeader & /*resp*/ )
{
    DEBUG_BLOCK
    Q_LONG index = 0;
    Q_LONG bytesWrite = 0;   
    
    char inBuf[8192];
    memset( inBuf, 0, sizeof( inBuf ) );
    const Q_LONG bytesRead = m_http->readBlock( inBuf, sizeof( inBuf ) );
    
    while ( index < bytesRead ) {
        bytesWrite = bytesRead - index;
        debug() << inBuf << " bytesWrite: " << bytesWrite << " bytesRead: " << bytesRead << " index " << index << endl;
        if( qstrcmp(inBuf, "SYNC") == 0 )
            return;
        bytesWrite = m_sockProxy->writeBlock( inBuf + index, bytesWrite );
        
        if ( bytesWrite == -1 )
            return;
        index += bytesWrite;
    }
 /*  for ( int i = 0; i < len; i++ )
    {
        QByteArray::Iterator end = m_buffer.end();
        end++;
        (*end) = inBuf[ i ] ;
    } */

}

void
LastFm::StreamProxy::newConnection( int socket )
{ 
    DEBUG_BLOCK 

    emit connected( socket ); 
}

#include "lastfmproxy.moc"
