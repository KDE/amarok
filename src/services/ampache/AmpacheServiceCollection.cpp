/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AmpacheServiceCollection.h"

#include "AmpacheServiceQueryMaker.h"
#include "NetworkAccessManagerProxy.h"

#include <KLocale>
#include <threadweaver/ThreadWeaver.h>

#include <QDomDocument>
#include <QNetworkReply>

using namespace Collections;

AmpacheServiceCollection::AmpacheServiceCollection( ServiceBase *service,
                                                    const QString &server,
                                                    const QString &sessionId )
    : ServiceCollection( service, "AmpacheCollection", "AmpacheCollection" )
    , m_server( server )
    , m_sessionId( sessionId )
{
    m_trackForUrlWorker = 0;
}

AmpacheServiceCollection::~AmpacheServiceCollection()
{
}

QueryMaker *
AmpacheServiceCollection::queryMaker()
{
    return new AmpacheServiceQueryMaker( this, m_server, m_sessionId );
}

QString
AmpacheServiceCollection::collectionId() const
{
    return "Ampache: " + m_server;
}

QString
AmpacheServiceCollection::prettyName() const
{
    return i18n( "Ampache Server %1", m_server );
}

bool
AmpacheServiceCollection::possiblyContainsTrack( const QUrl &url ) const
{
    return url.url().contains( m_server );
}

void
AmpacheServiceCollection::slotAuthenticationNeeded()
{
    emit authenticationNeeded();
}

Meta::TrackPtr
AmpacheServiceCollection::trackForUrl( const QUrl &url )
{
    MetaProxy::TrackPtr trackptr( new MetaProxy::Track( url.url(), MetaProxy::Track::ManualLookup ) );
    AmpacheTrackForUrlWorker *worker = new AmpacheTrackForUrlWorker( url, trackptr,
        m_server, m_sessionId, service() );
    connect( worker, SIGNAL(authenticationNeeded()), SLOT(slotAuthenticationNeeded()) );
    ThreadWeaver::Weaver::instance()->enqueue( worker );

    return Meta::TrackPtr::staticCast( trackptr );
}

void AmpacheServiceCollection::slotLookupComplete( const Meta::TrackPtr& )
{
}

void AmpacheTrackForUrlWorker::parseTrack( const QString &xml )
{
    //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( xml );
    QDomElement root = doc.firstChildElement( "root" );
    QDomElement song = root.firstChildElement( "song" );

    m_urlTrackId = song.attribute( "id", "0" ).toInt();

    QDomElement element = song.firstChildElement( "title" );

    QString title = element.text();
    if ( title.isEmpty() ) title = "Unknown";

    element = song.firstChildElement( "url" );

    m_urlTrack = new Meta::AmpacheTrack( title, m_service );
    Meta::TrackPtr trackPtr( m_urlTrack );

    m_urlTrack->setUidUrl( element.text() );
    m_urlTrack->setId( m_urlTrackId );

    element = song.firstChildElement( "time" );
    m_urlTrack->setLength( element.text().toInt() * 1000 );

    element = song.firstChildElement( "track" );
    m_urlTrack->setTrackNumber( element.text().toInt() );

    QDomElement albumElement = song.firstChildElement( "album" );
    m_urlAlbumId = albumElement.attribute( "id", "0" ).toInt();

    Meta::AmpacheAlbum *album = new Meta::AmpacheAlbum( albumElement.text() );

    QDomElement artElement = song.firstChildElement( "art" );
    album->setCoverUrl( artElement.text() );

    album->addTrack( trackPtr );
    m_urlTrack->setAlbumPtr( Meta::AlbumPtr( album ) );

    QDomElement artistElement = song.firstChildElement( "artist" );
    Meta::ServiceArtist *artist = new Meta::ServiceArtist( artistElement.text() );

    Meta::ArtistPtr artistPtr( artist );
    m_urlTrack->setArtist( artistPtr );
    album->setAlbumArtist( artistPtr );
}

AmpacheTrackForUrlWorker::AmpacheTrackForUrlWorker( const QUrl &url,
                                                    MetaProxy::TrackPtr track,
                                                    const QString &server,
                                                    const QString &sessionId,
                                                    ServiceBase *service )
    : Amarok::TrackForUrlWorker( url )
    , m_proxy( track )
    , m_server( server )
    , m_sessionId( sessionId )
    , m_service( service )
{
}

AmpacheTrackForUrlWorker::~AmpacheTrackForUrlWorker()
{}

void
AmpacheTrackForUrlWorker::run()
{
    m_urlTrack = 0;
    m_urlAlbum = 0;
    m_urlArtist = 0;

    m_urlTrackId = 0;
    m_urlAlbumId = 0;
    m_urlArtistId = 0;

    //send url_to_song to Ampache

    QString requestUrl =
            QString( "%1/server/xml.server.php?action=url_to_song&auth=%2&url=%3" )
                    .arg( m_server, m_sessionId, QUrl::toPercentEncoding( m_url.url() ) );

    QNetworkRequest req( requestUrl );
    QNetworkReply *reply = The::networkAccessManager()->get( req );

    if( reply->waitForReadyRead(-1) )
    {
        if( reply->error() == QNetworkReply::ContentAccessDenied )
        {
            debug() << "Trying to re-authenticate Ampache..";
            emit authenticationNeeded();
        }
    }
    parseTrack( reply->readAll() );
    m_track = Meta::TrackPtr( m_urlTrack );
    m_proxy->updateTrack( m_track );
    reply->deleteLater();
}
