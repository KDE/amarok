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

#include <KLocale>
#include <threadweaver/ThreadWeaver.h>

using namespace Meta;

AmpacheServiceCollection::AmpacheServiceCollection( ServiceBase * service, const QString &server, const QString &sessionId )
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
AmpacheServiceCollection::possiblyContainsTrack(const KUrl & url) const
{
    return url.url().contains( m_server );
}

void
AmpacheServiceCollection::slotAuthenticationNeeded()
{
    emit authenticationNeeded();
}

Meta::TrackPtr
AmpacheServiceCollection::trackForUrl( const KUrl & url )
{
    MetaProxy::Track* ptrack = new MetaProxy::Track( url.url(), true );
    MetaProxy::TrackPtr trackptr(ptrack);
    AmpacheTrackForUrlWorker * worker =  new AmpacheTrackForUrlWorker(url, trackptr, m_server, m_sessionId, service() );
//     connect( worker, SIGNAL( finishedLookup ( const Meta::TrackPtr& ) ), this,
//              SLOT( slotLookupComplete( const Meta::TrackPtr& ) ) );
    connect( worker, SIGNAL( authenticationNeeded() ), this,
             SLOT( slotAuthenticationNeeded() ) );
    ThreadWeaver::Weaver::instance()->enqueue( worker );

    return Meta::TrackPtr::staticCast( trackptr );
}
void AmpacheServiceCollection::slotLookupComplete( const Meta::TrackPtr& )
{
}

void AmpacheTrackForUrlWorker::parseTrack( const QString &xml )
{
//     DEBUG_BLOCK

//      debug() << "Received track response: " << xml;

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( xml );
    QDomElement root = doc.firstChildElement("root");
    QDomElement song = root.firstChildElement("song");

    m_urlTrackId = song.attribute( "id", "0").toInt();

    QDomElement element = song.firstChildElement("title");

    QString title = element.text();
    if ( title.isEmpty() ) title = "Unknown";

    element = song.firstChildElement("url");

    m_urlTrack = new AmpacheTrack( title, m_service );
    TrackPtr trackPtr( m_urlTrack );

    //debug() << "Adding track: " <<  title;
    m_urlTrack->setUidUrl( element.text() );
    m_urlTrack->setId( m_urlTrackId );

    element = song.firstChildElement("time");
    m_urlTrack->setLength( element.text().toInt() * 1000 );

    element = song.firstChildElement("track");
    m_urlTrack->setTrackNumber( element.text().toInt() );

    QDomElement albumElement = song.firstChildElement("album");
    //m_urlAlbumId = albumElement.attribute( "id", "0").toInt();

    AmpacheAlbum * album = new AmpacheAlbum( albumElement.text() );

    QDomElement artElement = song.firstChildElement("art");
    album->setCoverUrl( artElement.text() );

    album->addTrack( trackPtr );
    m_urlTrack->setAlbumPtr( AlbumPtr( album ) );

    QDomElement artistElement = song.firstChildElement("artist");
    ServiceArtist * artist = new ServiceArtist( artistElement.text() );

    ArtistPtr artistPtr( artist );
    m_urlTrack->setArtist( artistPtr );
    album->setAlbumArtist( artistPtr );
}

AmpacheTrackForUrlWorker::AmpacheTrackForUrlWorker(const KUrl &url, MetaProxy::TrackPtr track, const QString &server, const QString &sessionId, ServiceBase* service) : Amarok::TrackForUrlWorker(url), mProxy(track), m_server(server), m_sessionId(sessionId), m_service(service)
{
}

AmpacheTrackForUrlWorker::~AmpacheTrackForUrlWorker()
{}

void
AmpacheTrackForUrlWorker::run()
{
    //     DEBUG_BLOCK;

    m_urlTrack = 0;
    m_urlAlbum = 0;
    m_urlArtist = 0;

    m_urlTrackId = 0;
    m_urlAlbumId = 0;
    m_urlArtistId = 0;

    //send url_to_song to Ampache

    QString requestUrl = QString( "%1/server/xml.server.php?action=url_to_song&auth=%2&url=%3")
    . arg(  m_server,  m_sessionId,  QUrl::toPercentEncoding( mUrl.url() ) );
    //     debug() << "request url: " << requestUrl;

    m_storedTransferJob = KIO::storedGet(  KUrl( requestUrl ), KIO::NoReload, KIO::HideProgressInfo );
    if ( !m_storedTransferJob->exec() )
    {
        if( m_storedTransferJob->error() == 401 ) {
            debug() << "Trying to re-authenticate Ampache..";
            emit authenticationNeeded();

            m_storedTransferJob = KIO::storedGet(  KUrl( requestUrl ), KIO::NoReload, KIO::HideProgressInfo ); //Second try..
//             if( !m_storedTransferJob->exec() )
//                 return TrackPtr();
        }
//         else
//             return TrackPtr();
    }

    parseTrack( m_storedTransferJob->data() );
    m_storedTransferJob->deleteLater();

    mTrack = TrackPtr( m_urlTrack );
    mProxy->updateTrack( mTrack );
}
