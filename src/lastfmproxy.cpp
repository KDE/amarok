/***************************************************************************
 * copyright            : (C) 2006 Chris Muehlhaeuser <chris@chris.de>     *
 * copyright            : (C) 2006 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "lastfmproxy.h"
#include "debug.h"

#include <qdom.h>
#include <qfile.h>
#include <qhttp.h>
#include <qsocket.h>
#include <qtimer.h>

#include <kmdcodec.h> //md5sum

#include <time.h>

LastFmProxy::LastFmProxy()
{
    debug() << "Initialising Web Service" << endl;
}


void
LastFmProxy::handshake( const QString& username, const QString& password )
{
    m_username = username;
    m_password = password;

    QHttp *http = new QHttp( "ws.audioscrobbler.com", 80, this );
    connect( http, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ),
             this,   SLOT( handshakeHeaderReceived( const QHttpResponseHeader& ) ) );

    connect( http, SIGNAL( requestFinished( int, bool ) ),
             this,   SLOT( handshakeFinished( int, bool ) ) );

    QString path =
            QString( "/radio/handshake.php?version=%1&platform=%2&username=%3&passwordmd5=%4&debug=%5" )
            .arg( "1.2.0" )
            .arg( QString("linux") ) //platform
            .arg( QString( QUrl( username ).encodedPathAndQuery() ) )
            .arg( KMD5( m_password.utf8() ).hexDigest() )
            .arg( "0" );

    http->get( path );
    m_lastHttp = http;
}


void
LastFmProxy::handshakeHeaderReceived( const QHttpResponseHeader &resp )
{
    if ( resp.statusCode() == 503 )
    {
        debug() << "Handshake error" << endl;
    }
}


void
LastFmProxy::handshakeFinished( int /*id*/, bool error )
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
    m_streamUrl.setPath( parameter( "stream_url", result ) );
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
LastFmProxy::changeStation( QString url )
{
    debug() << "Changing station:" << url;

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( changeStationFinished( bool ) ) );

    if ( url.startsWith( "lastfm://" ) )
        url.remove( 0, 9 ); // get rid of it!

    http->get( QString( m_basePath + "/adjust.php?session=%1&url=lastfm://%2&debug=%3" )
                  .arg( m_session )
                  .arg( url.contains( "%" ) ? url : QString( QUrl( url ).encodedPathAndQuery() ) )
                  .arg( "0" ) );

    m_lastHttp = http;
}


void
LastFmProxy::changeStationFinished( int /*id*/, bool error )
{
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

            if ( !url.startsWith( "play" ) )
            {
                emit skipDone();
                emit stationChanged( url, stationName );
            }
            else
                emit songQueued();
        }
    }
}


void
LastFmProxy::requestMetaData()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( metaDataFinished( bool ) ) );

    http->get( QString( m_basePath + "/np.php?session=%1&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFmProxy::metaDataFinished( int /*id*/, bool error )
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
LastFmProxy::enableScrobbling( bool enabled )
{
    if ( enabled )
        debug() << "Enabling Scrobbling!";
    else
        debug() << "Disabling Scrobbling!";

    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( enableScrobblingFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=%2&debug=%3" )
                  .arg( m_session )
                  .arg( enabled ? QString( "rtp" ) : QString( "nortp" ) )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFmProxy::enableScrobblingFinished( int /*id*/, bool error )
{
    if ( error )
        return;

    emit enableScrobblingDone();
}


void
LastFmProxy::love()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=love&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );

    m_lastHttp = http;
}


void
LastFmProxy::skip()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( skipFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=skip&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
}


void
LastFmProxy::ban()
{
    QHttp *http = new QHttp( m_baseHost, 80, this );
    connect( http, SIGNAL( requestFinished( bool ) ), this, SLOT( loveFinished( bool ) ) );

    http->get( QString( m_basePath + "/control.php?session=%1&command=ban&debug=%2" )
                  .arg( m_session )
                  .arg( "0" ) );
    m_lastHttp = http;
}


void
LastFmProxy::loveFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit loveDone();
}


void
LastFmProxy::skipFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit skipDone();
}


void
LastFmProxy::banFinished( int /*id*/, bool error )
{
    if( error ) return;
    emit banDone();
    emit skipDone();
}


void
LastFmProxy::friends( QString username )
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
LastFmProxy::friendsFinished( int /*id*/, bool error )
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
LastFmProxy::neighbours( QString username )
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
LastFmProxy::neighboursFinished( int /*id*/, bool error )
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
LastFmProxy::userTags( QString username )
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
LastFmProxy::userTagsFinished( int /*id*/, bool error )
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
LastFmProxy::recentTracks( QString username )
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
LastFmProxy::recentTracksFinished( int /*id*/, bool error )
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
LastFmProxy::recommend( int type, QString username, QString artist, QString token )
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
LastFmProxy::recommendFinished( int /*id*/, bool /*error*/ )
{
    debug() << "Recommendation:" << m_lastHttp->readAll();
}


QString
LastFmProxy::parameter( QString keyName, QString data )
{
    QStringList list = QStringList::split( data, "\n" );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( list[i], "=" );
        if ( values[0] == keyName )
        {
            values.remove( values.at(0) );
            return QString().fromUtf8( values.join( "=" ).ascii() );
        }
    }

    return QString( "" );
}


QStringList
LastFmProxy::parameterArray( QString keyName, QString data )
{
    QStringList result;
    QStringList list = QStringList::split( data, "\n" );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( list[i], "=" );
        if ( values[0].startsWith( keyName ) )
        {
            values.remove( values.at(0) );
            result.append( QString().fromUtf8( values.join( "=" ).ascii() ) );
        }
    }

    return result;
}


QStringList
LastFmProxy::parameterKeys( QString keyName, QString data )
{
    QStringList result;
    QStringList list = QStringList::split( data, "\n" );

    for ( uint i = 0; i < list.size(); i++ )
    {
        QStringList values = QStringList::split( list[i], "=" );
        if ( values[0].startsWith( keyName ) )
        {
            values = QStringList::split( values[0], "[" );
            values = QStringList::split( values[1], "]" );
            result.append( values[0] );
        }
    }

    return result;
}


#include "lastfmproxy.moc"
