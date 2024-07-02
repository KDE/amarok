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

#include <KLocalizedString>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>

#include <QDomDocument>
#include <QNetworkReply>
#include <QUrlQuery>

using namespace Collections;

AmpacheServiceCollection::AmpacheServiceCollection( ServiceBase *service,
                                                    const QUrl &server,
                                                    const QString &sessionId )
    : ServiceCollection( service, QStringLiteral("AmpacheCollection"), QStringLiteral("AmpacheCollection") )
    , m_server( server )
    , m_sessionId( sessionId )
{
    m_trackForUrlWorker = nullptr;
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
    return QStringLiteral("Ampache: ") + m_server.url();
}

QString
AmpacheServiceCollection::prettyName() const
{
    return i18n( "Ampache Server %1", m_server.url() );
}

bool
AmpacheServiceCollection::possiblyContainsTrack( const QUrl &url ) const
{
    return m_server.isParentOf( url );
}

void
AmpacheServiceCollection::slotAuthenticationNeeded()
{
    Q_EMIT authenticationNeeded();
}

Meta::TrackPtr
AmpacheServiceCollection::trackForUrl( const QUrl &url )
{
    MetaProxy::TrackPtr trackptr( new MetaProxy::Track( url, MetaProxy::Track::ManualLookup ) );
    AmpacheTrackForUrlWorker *worker = new AmpacheTrackForUrlWorker( url, trackptr,
        m_server, m_sessionId, service() );
    connect( worker, &AmpacheTrackForUrlWorker::authenticationNeeded,
             this, &AmpacheServiceCollection::slotAuthenticationNeeded );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(worker) );

    return Meta::TrackPtr::staticCast( trackptr );
}

void AmpacheServiceCollection::slotLookupComplete( const Meta::TrackPtr& )
{
}

void AmpacheTrackForUrlWorker::parseTrack( const QString &xml )
{
    //so lets figure out what we got here:
    QDomDocument doc( QStringLiteral("reply") );
    doc.setContent( xml );
    QDomElement root = doc.firstChildElement( QStringLiteral("root") );
    QDomElement song = root.firstChildElement( QStringLiteral("song") );

    m_urlTrackId = song.attribute( QStringLiteral("id"), QStringLiteral("0") ).toInt();

    QDomElement element = song.firstChildElement( QStringLiteral("title") );

    QString title = element.text();
    if ( title.isEmpty() ) title = QStringLiteral("Unknown");

    element = song.firstChildElement( QStringLiteral("url") );

    m_urlTrack = new Meta::AmpacheTrack( title, m_service );
    Meta::TrackPtr trackPtr( m_urlTrack );

    m_urlTrack->setUidUrl( element.text() );
    m_urlTrack->setId( m_urlTrackId );

    element = song.firstChildElement( QStringLiteral("time") );
    m_urlTrack->setLength( element.text().toInt() * 1000 );

    element = song.firstChildElement( QStringLiteral("track") );
    m_urlTrack->setTrackNumber( element.text().toInt() );

    QDomElement albumElement = song.firstChildElement( QStringLiteral("album") );
    m_urlAlbumId = albumElement.attribute( QStringLiteral("id"), QStringLiteral("0") ).toInt();

    Meta::AmpacheAlbum *album = new Meta::AmpacheAlbum( albumElement.text() );

    QDomElement artElement = song.firstChildElement( QStringLiteral("art") );
    album->setCoverUrl( artElement.text() );

    album->addTrack( trackPtr );
    m_urlTrack->setAlbumPtr( Meta::AlbumPtr( album ) );

    QDomElement artistElement = song.firstChildElement( QStringLiteral("artist") );
    Meta::ServiceArtist *artist = new Meta::ServiceArtist( artistElement.text() );

    Meta::ArtistPtr artistPtr( artist );
    m_urlTrack->setArtist( artistPtr );
    album->setAlbumArtist( artistPtr );
}

AmpacheTrackForUrlWorker::AmpacheTrackForUrlWorker( const QUrl &url,
                                                    const MetaProxy::TrackPtr &track,
                                                    const QUrl &server,
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
AmpacheTrackForUrlWorker::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED( self )
    Q_UNUSED( thread )

    m_urlTrack = nullptr;
    m_urlAlbum = nullptr;
    m_urlArtist = nullptr;

    m_urlTrackId = 0;
    m_urlAlbumId = 0;
    m_urlArtistId = 0;

    //send url_to_song to Ampache

    QUrl requestUrl = m_server;
    requestUrl.setPath( m_server.path() + QStringLiteral("/server/xml.server.php") );
    QUrlQuery query;
    query.addQueryItem( QStringLiteral("action"), QStringLiteral("url_to_song") );
    query.addQueryItem( QStringLiteral("auth"), m_sessionId );
    query.addQueryItem( QStringLiteral("url"), m_url.toEncoded() );
    requestUrl.setQuery( query );

    QNetworkRequest req( requestUrl );
    QNetworkReply *reply = The::networkAccessManager()->get( req );

    if( reply->waitForReadyRead(-1) )
    {
        if( reply->error() == QNetworkReply::ContentAccessDenied )
        {
            debug() << "Trying to re-authenticate Ampache..";
            Q_EMIT authenticationNeeded();
        }
    }
    parseTrack( reply->readAll() );
    m_track = Meta::TrackPtr( m_urlTrack );
    m_proxy->updateTrack( m_track );
    reply->deleteLater();
}
