/***************************************************************************
 * copyright            : (C) 2006 Chris Muehlhaeuser <chris@chris.de>     *
 *                      : (C) 2006 Seb Ruiz <me@sebruiz.net>               *
 *                      : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 *                      : (C) 2006 Mark Kretschmann <markey@web.de>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "LastFmProxy"

#include "amarok.h"         //APP_VERSION
#include "amarokconfig.h"   //last.fm username and passwd
#include "debug.h"
#include "enginecontroller.h"
#include "lastfmproxy.h"
#include "metabundle.h"

#include <qdom.h>
#include <qhttp.h>
#include <qregexp.h>

#include <kmdcodec.h>       //md5sum
#include <kprocio.h>
#include <kprotocolmanager.h>
#include <kurl.h>

#include <time.h>
#include <unistd.h>

using namespace LastFm;


////////////////////////////////////////////////////////////////////////////////
// CLASS Controller
////////////////////////////////////////////////////////////////////////////////

Controller *Controller::s_instance = 0;

Controller::Controller()
    : QObject( EngineController::instance(), "lastfmController" )
    , m_service( 0 )
{}


Controller*
Controller::instance()
{
    if( !s_instance ) s_instance = new Controller();
    return s_instance;
}


KURL
Controller::getNewProxy( QString genreUrl )
{
    DEBUG_BLOCK

    m_genreUrl = genreUrl;

    if ( m_service ) playbackStopped();

    m_service = new WebService( this );

    if( m_service->handshake( AmarokConfig::scrobblerUsername(), AmarokConfig::scrobblerPassword() ) ) {
        m_service->changeStation( m_genreUrl );

        return KURL( m_service->proxyUrl() );
    }

    // Some kind of failure happened, so crap out
    playbackStopped();
    return KURL();
}


void
Controller::playbackStopped() //SLOT
{
    DEBUG_BLOCK

    delete m_service;
    m_service = 0;
}


////////////////////////////////////////////////////////////////////////////////
// CLASS WebService
////////////////////////////////////////////////////////////////////////////////

WebService::WebService( QObject* parent )
    : QObject( parent, "lastfmParent" )
    , m_lastHttp( 0 )
    , m_server( 0 )
{
    debug() << "Initialising Web Service" << endl;
}


WebService::~WebService()
{
    delete m_server;
}


void
WebService::readProxy() //SLOT
{
    DEBUG_BLOCK

    QString line;
    int res;

    while( true ) {
        res = m_server->readln( line );
        if( res == -1 ) break;

        if( line == "AMAROK_PROXY: SYNC frame" )
            requestMetaData();
    }
}


bool
WebService::handshake( const QString& username, const QString& password )
{
    DEBUG_BLOCK

    m_username = username;
    m_password = password;

    QHttp *http = new QHttp( "ws.audioscrobbler.com", 80, this );

    const QString path =
            QString( "/radio/handshake.php?version=%1&platform=%2&username=%3&passwordmd5=%4&debug=%5" )
            .arg( APP_VERSION )             //Muesli-approved: Amarok version, and Amarok-as-platform
            .arg( QString("Amarok") )
            .arg( QString( QUrl( username ).encodedPathAndQuery() ) )
            .arg( KMD5( m_password.utf8() ).hexDigest() )
            .arg( "0" );

    http->get( path );
    m_lastHttp = http;

    do
        kapp->processEvents();
    while( http->state() != QHttp::Unconnected );

    if ( http->error() != QHttp::NoError )
        return false;

    const QString result( m_lastHttp->readAll() );

    debug() << "result: " << result << endl;

    m_session = parameter( "session", result );
    m_baseHost = parameter( "base_url", result );
    m_basePath = parameter( "base_path", result );
    m_subscriber = parameter( "subscriber", result ) == "1";
    m_streamUrl = QUrl( parameter( "stream_url", result ) );
//     bool banned = parameter( "banned", result ) == "1";

    if ( m_session.lower() == "failed" )
        return false;

    // Find free port
    MyServerSocket* socket = new MyServerSocket();
    const int port = socket->port();
    debug() << "Proxy server using port: " << port << endl;
    delete socket;

    m_proxyUrl = QString( "http://localhost:%1/theBeard.mp3" ).arg( port );

    m_server = new KProcIO();
    m_server->setComm( KProcess::Communication( KProcess::Stdin|KProcess::Stderr ) );
    *m_server << "amarok_proxy.rb";
    *m_server << QString::number( port );
    *m_server << m_streamUrl.toString();
    m_server->start( KProcIO::NotifyOnExit, true );

    QString line;
    while( true ) {
        m_server->readln( line );
        if( line == "AMAROK_PROXY: startup" ) break;
        kapp->processEvents();
    }

    connect( m_server, SIGNAL( readReady( KProcIO* ) ), this, SLOT( readProxy() ) );

    return true;
}


void
WebService::changeStation( QString url )
{
    DEBUG_BLOCK
    debug() << "Changing station:" << url << endl;

    QHttp *http = new QHttp( m_baseHost, 80, this );

    if ( url.startsWith( "lastfm://" ) )
        url.remove( 0, 9 ); // get rid of it!

    http->get( QString( m_basePath + "/adjust.php?session=%1&url=lastfm://%2&debug=%3" )
                  .arg( m_session )
                  .arg( url.contains( "%" ) ? url : QUrl( url ).toString(true, false) )
                  .arg( "0" ) );
    m_lastHttp = http;

    do
        kapp->processEvents();
    while( http->state() != QHttp::Unconnected );

    if ( http->error() != QHttp::NoError )
        return;

    const QString result( http->readAll() );
    const int errCode = parameter( "error", result ).toInt();

    if ( errCode <= 0 )
    {
        const QString url = parameter( "url", result );
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
WebService::requestMetaData() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( metaDataFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/np.php?session=%1&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
WebService::metaDataFinished( int /*id*/, bool error ) //SLOT
{
    if( error ) return;

    const QString result( m_lastHttp->readAll() );

    MetaBundle bundle;
    bundle.setArtist( parameter( "artist", result ) );
    bundle.setAlbum( parameter( "album", result ) );
    bundle.setTitle( parameter( "track", result ) );
    bundle.setUrl( KURL (Controller::instance()->getGenreUrl() ) );
//     bundle.setCover( parameter( "albumcover_medium", result ) );
//     bundle.setArtistUrl( parameter( "artist_url", result ) );
//     bundle.setAlbumUrl( parameter( "album_url", result ) );
//     bundle.setTrackUrl( parameter( "track_url", result ) );
    bundle.setLength( parameter( "trackduration", result ).toInt() );
//     bool discovery = parameter( "discovery", result ) != "-1";

    int errCode = parameter( "error", result ).toInt();
    if ( errCode > 0 )
        return;

    emit metaDataResult( bundle );
}


void
WebService::enableScrobbling( bool enabled ) //SLOT
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
WebService::enableScrobblingFinished( int /*id*/, bool error ) //SLOT
{
    if ( error ) return;

    emit enableScrobblingDone();
}


void
WebService::love() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=love&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );

    m_lastHttp = http;
}


void
WebService::skip() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( skipFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=skip&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
WebService::ban() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=ban&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
WebService::loveFinished( int /*id*/, bool error ) //SLOT
{
    if( error ) return;
    emit loveDone();
}


void
WebService::skipFinished( int /*id*/, bool error ) //SLOT
{
    if( error ) return;
    emit skipDone();
}


void
WebService::banFinished( int /*id*/, bool error ) //SLOT
{
    if( error ) return;
    emit banDone();
    emit skipDone();
}


void
WebService::friends( QString username )
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
WebService::friendsFinished( int /*id*/, bool error ) //SLOT
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
WebService::neighbours( QString username )
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
WebService::neighboursFinished( int /*id*/, bool error ) //SLOT
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
WebService::userTags( QString username )
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
WebService::userTagsFinished( int /*id*/, bool error ) //SLOT
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
WebService::recentTracks( QString username )
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
WebService::recentTracksFinished( int /*id*/, bool error ) //SLOT
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
WebService::recommend( int type, QString username, QString artist, QString token )
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
WebService::recommendFinished( int /*id*/, bool /*error*/ ) //SLOT
{
    debug() << "Recommendation:" << m_lastHttp->readAll() << endl;
}


QString
WebService::parameter( QString keyName, QString data ) const
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
WebService::parameterArray( QString keyName, QString data ) const
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
WebService::parameterKeys( QString keyName, QString data ) const
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


#include "lastfmproxy.moc"
