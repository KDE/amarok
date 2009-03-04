/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Tuner.h"
#include "lastfm/core/CoreDomElement.h"
#include "lastfm/core/CoreLocale.h"
#include "lastfm/core/CoreSettings.h"
#include "lastfm/ws/WsAccessManager.h"
#include <QCoreApplication>
#include <QtNetwork>

namespace lastfm {
namespace legacy {


Tuner::Tuner( const RadioStation& station, const QString& password_md5 )
     : m_nam( new WsAccessManager( this ) ),
       m_retry_counter( 0 ),
       m_stationUrl( station.url() )
{   
#ifdef WIN32
    static const char *PLATFORM = "win32";
#elif defined Q_WS_X11
    static const char *PLATFORM = "linux";
#elif defined Q_WS_MAC
    static const char *PLATFORM = "mac";
#else
    static const char *PLATFORM = "unknown";
#endif

    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/radio/handshake.php" );
    url.addQueryItem( "version", QCoreApplication::applicationVersion() );
    url.addQueryItem( "platform", PLATFORM );
    url.addQueryItem( "username", Ws::Username );
    url.addQueryItem( "passwordmd5", password_md5 );
    url.addQueryItem( "language", CoreSettings().locale().code() );

    QNetworkRequest request( url );
    QNetworkReply* reply = m_nam->get( request );
    connect( reply, SIGNAL(finished()), SLOT(onHandshakeReturn()) );
}

    
static QByteArray replyParameter( const QByteArray& data, const QByteArray& key )
{
    foreach (QByteArray key_value_pair, data.split( '\n' ))
    {
        QList<QByteArray> pair = key_value_pair.split( '=' );
        if (pair.value( 0 ) == key)
            return pair.value( 1 );
    }

    return "";
}
    

void
Tuner::onHandshakeReturn()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    reply->deleteLater();
    
    QByteArray data = reply->readAll();
    qDebug() << data;

    m_session = replyParameter( data, "session" );
    
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/radio/adjust.php" );
    url.addEncodedQueryItem( "session", m_session );
    url.addQueryItem( "url", m_stationUrl );
    url.addQueryItem( "lang", CoreSettings().locale().code() );

    qDebug() << url;
    
    QNetworkRequest request( url );
    reply = m_nam->get( request );
    connect( reply, SIGNAL(finished()), SLOT(onAdjustReturn()) );
}


void
Tuner::onAdjustReturn()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QByteArray data = reply->readAll();
    qDebug() << data;
    
    m_stationName = QString::fromUtf8( replyParameter( data, "stationname" ) );
    emit stationName( m_stationName );
    
    fetchFiveMoreTracks();

    reply->deleteLater();
}


void
Tuner::fetchFiveMoreTracks()
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/radio/xspf.php" );
    url.addQueryItem( "sk", m_session );
    url.addQueryItem( "desktop", "1.5.3" );
    
    QNetworkRequest request( url );
    QNetworkReply* reply = m_nam->get( request );
    connect( reply, SIGNAL(finished()), SLOT(onGetPlaylistReturn()) );
}


bool
Tuner::tryAgain()
{
	if (++m_retry_counter > 5)
		return false;
	fetchFiveMoreTracks();
	return true;
}


class Xspf
{
    QList<Track> m_tracks;
    QString m_title;
    
public:
    Xspf( const QDomElement& playlist_node )
    {
        try
        {
            CoreDomElement e( playlist_node );
            
            m_title = e["title"].text();
            
            //FIXME should we use UnicornUtils::urlDecode()?
            //The title is url encoded, has + instead of space characters 
            //and has a + at the begining. So it needs cleaning up:
            m_title.replace( '+', ' ' );
            m_title = QUrl::fromPercentEncoding( m_title.toAscii());
            m_title = m_title.trimmed();
            
            foreach (CoreDomElement e, e[ "trackList" ].children( "track" ))
            {
                MutableTrack t;
                try
                {
                    //TODO we don't want to throw an exception for any of these really,
                    //TODO only if we couldn't get any, but even then so what
                    t.setUrl( e[ "location" ].text() ); //location first due to exception throwing
                    t.setExtra( "trackauth", e[ "lastfm:trackauth" ].text() );
                    t.setTitle( e[ "title" ].text() );
                    t.setArtist( e[ "creator" ].text() );
                    t.setAlbum( e[ "album" ].text() );
                    t.setDuration( e[ "duration" ].text().toInt() / 1000 );
                    t.setSource( Track::LastFmRadio );
                }
                catch (CoreDomElement::Exception& exception)
                {
                    qWarning() << exception << e;
                }
                
                m_tracks += t; // outside try-catch since location is enough basically
            }
        }
        catch (CoreDomElement::Exception& e)
        {
            qWarning() << e;        
        }
    }
    
    QList<Track> tracks() const { return m_tracks; }
};
    
    
void
Tuner::onGetPlaylistReturn()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    reply->deleteLater();

    QByteArray data = reply->readAll();
    qDebug() << data;

    QDomDocument xml;
    xml.setContent( data );

    Xspf xspf( xml.documentElement() );

    if (xspf.tracks().isEmpty())
    {
        // sometimes the recs service craps out and gives us a blank playlist

        if (!tryAgain())
        {
            // an empty playlist is a bug, if there is no content
            // NotEnoughContent should have been returned with the WsReply
            emit error( Ws::MalformedResponse );
        }
    }
    else {
        m_retry_counter = 0;
        emit tracks( xspf.tracks() );
    }
}

} //namespace legacy
} //namespace lastfm
