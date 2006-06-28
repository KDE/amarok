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
#include "collectiondb.h"
#include "debug.h"
#include "enginecontroller.h"
#include "lastfmproxy.h"

#include <qdom.h>
#include <qhttp.h>
#include <qlabel.h>
#include <qregexp.h>

#include <klineedit.h>
#include <kmdcodec.h>       //md5sum
#include <kmessagebox.h>
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
    DEBUG_BLOCK
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

    if( AmarokConfig::scrobblerUsername().isEmpty() || AmarokConfig::scrobblerPassword().isEmpty() )
    {
        LoginDialog dialog( 0 );
        dialog.setCaption( "last.fm" );
        dialog.exec();
    }

    QString user = AmarokConfig::scrobblerUsername();
    QString pass = AmarokConfig::scrobblerPassword();

    if( !user.isEmpty() && !pass.isEmpty() &&
        m_service->handshake( user, pass ) )
    {
        m_service->changeStation( m_genreUrl );
        if( !AmarokConfig::submitPlayedSongs() )
            m_service->enableScrobbling( false );

        return KURL( m_service->proxyUrl() );
    }

    // Some kind of failure happened, so crap out
    playbackStopped();
    return KURL();
}


void
Controller::playbackStopped() //SLOT
{
    delete m_service;
    m_service = 0;
}


////////////////////////////////////////////////////////////////////////////////
// CLASS WebService
////////////////////////////////////////////////////////////////////////////////

WebService::WebService( QObject* parent )
    : QObject( parent, "lastfmParent" )
    , m_server( 0 )
{
    debug() << "Initialising Web Service" << endl;
}


WebService::~WebService()
{
    DEBUG_BLOCK

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

        if( line == "AMAROK_PROXY: SYNC" )
            requestMetaData();
    }
}


bool
WebService::handshake( const QString& username, const QString& password )
{
    DEBUG_BLOCK

    m_username = username;
    m_password = password;

    QHttp http( "ws.audioscrobbler.com", 80 );

    const QString path =
            QString( "/radio/handshake.php?version=%1&platform=%2&username=%3&passwordmd5=%4&debug=%5" )
            .arg( APP_VERSION )             //Muesli-approved: Amarok version, and Amarok-as-platform
            .arg( QString("Amarok") )
            .arg( QString( QUrl( username ).encodedPathAndQuery() ) )
            .arg( KMD5( m_password.utf8() ).hexDigest() )
            .arg( "0" );

    http.get( path );

    do
        kapp->processEvents();
    while( http.state() != QHttp::Unconnected );

    if ( http.error() != QHttp::NoError )
        return false;

    const QString result( http.readAll() );

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
        kapp->processEvents();
        m_server->readln( line );
        if( line == "AMAROK_PROXY: startup" ) break;
    }

    connect( m_server, SIGNAL( readReady( KProcIO* ) ), this, SLOT( readProxy() ) );
    connect( m_server, SIGNAL( processExited( KProcess* ) ), this, SLOT( deleteLater() ) );

    return true;
}


void
WebService::changeStation( QString url )
{
    debug() << "Changing station:" << url << endl;

    QHttp http( m_baseHost, 80 );

    url.replace( "lastfm://", "" ); // get rid of it!

    http.get( QString( m_basePath + "/adjust.php?session=%1&url=lastfm://%2&debug=%3" )
             .arg( m_session )
             .arg( url )
             .arg( "0" ) );

    do
        kapp->processEvents();
    while( http.state() != QHttp::Unconnected );

    if ( http.error() != QHttp::NoError )
        return;

    const QString result( http.readAll() );
    const int errCode = parameter( "error", result ).toInt();

    if ( errCode <= 0 )
    {
        const QString url = parameter( "url", result );
        if ( url.startsWith( "lastfm://" ) )
        {
            m_station = parameter( "stationname", result );
            if ( m_station.isEmpty() )
                m_station = url;
            emit stationChanged( url, m_station );
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
}


void
WebService::metaDataFinished( int /*id*/, bool error ) //SLOT
{
    DEBUG_BLOCK

    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    const QString result( http->readAll() );
    debug() << result << endl;

    int errCode = parameter( "error", result ).toInt();
    if ( errCode > 0 )
        return;

    m_metaBundle.setArtist( parameter( "artist", result ) );
    m_metaBundle.setAlbum ( parameter( "album", result )  );
    m_metaBundle.setTitle ( parameter( "track", result )  );
    m_metaBundle.setUrl   ( KURL( Controller::instance()->getGenreUrl() ) );
    m_metaBundle.setLength( parameter( "trackduration", result ).toInt()  );

    Bundle lastFmStuff;
    QString imageUrl = parameter( "albumcover_medium", result );

    if( imageUrl == "http://static.last.fm/coverart/" ||
        imageUrl == "http://static.last.fm/depth/catalogue/no_album_large.gif" )
        imageUrl = QString::null;

    lastFmStuff.setImageUrl ( "file://" + CollectionDB::instance()->notAvailCover( true ) );
    lastFmStuff.setArtistUrl( parameter( "artist_url", result ) );
    lastFmStuff.setAlbumUrl ( parameter( "album_url", result ) );
    lastFmStuff.setTitleUrl ( parameter( "track_url", result ) );
//     bool discovery = parameter( "discovery", result ) != "-1";

    m_metaBundle.setLastFmBundle( lastFmStuff );

    if( imageUrl.isEmpty() ) {
        emit metaDataResult( m_metaBundle );
        return;
    }

    const KURL u( imageUrl );
    QHttp* h = new QHttp( u.host(), 80 );
    connect( h, SIGNAL( requestFinished( int, bool ) ), this, SLOT( fetchImageFinished( int, bool ) ) );

    h->get( u.path() );
}


void
WebService::fetchImageFinished( int /*id*/, bool error ) //SLOT
{
    DEBUG_BLOCK

    QHttp* http = (QHttp*) sender();
    http->deleteLater();

    if( !error ) {
        const QString path = amaroK::saveLocation() + "lastfm_image.png";
        const int size = AmarokConfig::coverPreviewSize();

        QImage img( http->readAll() );
        img.smoothScale( size, size ).save( path, "PNG" );

        m_metaBundle.lastFmBundle()->setImageUrl( "file://" + CollectionDB::makeShadowedImage( path, false ) );
    }
    emit metaDataResult( m_metaBundle );
}


void
WebService::enableScrobbling( bool enabled ) //SLOT
{
    if ( enabled )
        debug() << "Enabling Scrobbling!" << endl;
    else
        debug() << "Disabling Scrobbling!" << endl;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( enableScrobblingFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=%2&debug=%3" )
                  .arg( m_session )
                  .arg( enabled ? QString( "rtp" ) : QString( "nortp" ) )
                  .arg( "0" ) );
}


void
WebService::enableScrobblingFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if ( error ) return;

    emit enableScrobblingDone();
}


void
WebService::love() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( loveFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=love&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
WebService::skip() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( skipFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=skip&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
WebService::ban() //SLOT
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( banFinished( int, bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=ban&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
WebService::loveFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    emit loveDone();
}


void
WebService::skipFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    EngineController::engine()->flushBuffer();
    emit skipDone();
}


void
WebService::banFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    EngineController::engine()->flushBuffer();
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
}


void
WebService::friendsFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    QDomDocument document;
    document.setContent( http->readAll() );

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
}


void
WebService::neighboursFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error )  return;

    QDomDocument document;
    document.setContent( http->readAll() );

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
}


void
WebService::userTagsFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    QDomDocument document;
    document.setContent( http->readAll() );

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
}


void
WebService::recentTracksFinished( int /*id*/, bool error ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();
    if( error ) return;

    QValueList< QPair<QString, QString> > songs;
    QDomDocument document;
    document.setContent( http->readAll() );

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
}


void
WebService::recommendFinished( int /*id*/, bool /*error*/ ) //SLOT
{
    QHttp* http = (QHttp*) sender();
    http->deleteLater();

    debug() << "Recommendation:" << http->readAll() << endl;
}


QString
WebService::parameter( QString keyName, QString data ) const
{
    QStringList list = QStringList::split( '\n', data );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( '=', list[i] );
        if ( keyName == "albumcover_small" )
            debug() << values[0] << '-' << values[1] << endl;
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

////////////////////////////////////////////////////////////////////////////////
// CLASS LastFm::Bundle
////////////////////////////////////////////////////////////////////////////////

Bundle::Bundle( const Bundle& lhs )
    : m_imageUrl( lhs.m_imageUrl )
    , m_albumUrl( lhs.m_albumUrl )
    , m_artistUrl( lhs.m_artistUrl )
    , m_titleUrl( lhs.m_titleUrl )
{}

////////////////////////////////////////////////////////////////////////////////
// CLASS LastFm::LoginDialog
////////////////////////////////////////////////////////////////////////////////
LoginDialog::LoginDialog( QWidget *parent )
    : KDialogBase( parent, "LastFmLogin", true, QString::null, Ok|Cancel)
{
    makeGridMainWidget( 1, Qt::Horizontal );
    new QLabel( i18n( "To use last.fm with Amarok, you need a last.fm profile." ), mainWidget() );

    makeGridMainWidget( 2, Qt::Horizontal );
    QLabel *nameLabel = new QLabel( i18n("&Username:"), mainWidget() );
    m_userLineEdit = new KLineEdit( mainWidget() );
    nameLabel->setBuddy( m_userLineEdit );

    QLabel *passLabel = new QLabel( i18n("&Password:"), mainWidget() );
    m_passLineEdit = new KLineEdit( mainWidget() );
    passLabel->setBuddy( m_passLineEdit );

    m_userLineEdit->setFocus();
}

void LoginDialog::slotOK()
{
    AmarokConfig::setScrobblerUsername( m_userLineEdit->text() );
    AmarokConfig::setScrobblerPassword( m_passLineEdit->text() );

    KDialogBase::slotOk();
}

#include "lastfmproxy.moc"
