/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
 * Copyright (c) 2010 Hormiere Guillaume <hormiere.guillaume@gmail.com>                 *
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

#define DEBUG_PREFIX "UpcomingEventsEngine"

// Includes
#include "UpcomingEventsEngine.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ContextView.h"
#include "EngineController.h"
#include "LastFmEventXmlParser.h"

// LastFm
#include <lastfm/XmlQuery>
#include <lastfm/ws.h>

// KDE
#include <KDateTime>

// Qt
#include <QXmlStreamReader>

K_EXPORT_AMAROK_DATAENGINE( upcomingEvents, UpcomingEventsEngine )

using namespace Context;

UpcomingEventsEngine::UpcomingEventsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , Engine::EngineObserver( The::engineController() )
        , m_timeSpan( "AllEvents" )
{
}

UpcomingEventsEngine::~UpcomingEventsEngine()
{
}

bool
UpcomingEventsEngine::sourceRequestEvent( const QString &source )
{
    if( source == "artistevents" )
    {
        engineNewTrackPlaying();
        return false; // data is not ready yet, but will be soon
    }
    else if( source == "venueevents" )
    {
        QList<int> venueIds;
        QStringList venues = Amarok::config("UpcomingEvents Applet").readEntry( "favVenues", QStringList() );
        foreach( const QString &venue, venues )
        {
            QStringList frag = venue.split( QChar(';') );
            venueIds << frag.at( 0 ).toInt();
        }
        updateDataForVenues( venueIds );
        return true;
    }
    else if( source == "venueevents:update" )
    {
        removeAllData( source );
        sourceRequestEvent( "venueevents" );
    }
    else if( source.startsWith( "timespan" ) )
    {
        // user has changed the timespan.
        QStringList tokens = source.split( ':' );
        if( tokens.size() > 1 )
            m_timeSpan = tokens.at( 1 );
        updateDataForArtist();
        return true;
    }
    return false;
}

void
UpcomingEventsEngine::metadataChanged( Meta::TrackPtr track )
{
    if( m_currentTrack->artist() != track->artist() )
        updateDataForArtist();
}

void
UpcomingEventsEngine::engineNewTrackPlaying()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !m_currentTrack )
    {
        subscribeTo( track );
        m_currentTrack = track;
        updateDataForArtist();
    }
    else if( m_currentTrack != track )
    {
        Meta::ArtistPtr oldArtist = m_currentTrack->artist();
        unsubscribeFrom( m_currentTrack );
        subscribeTo( track );
        m_currentTrack = track;
        if( oldArtist != track->artist() )
            updateDataForArtist();
    }
}

void
UpcomingEventsEngine::updateDataForVenues( QList<int> ids )
{
    while( !ids.isEmpty() )
    {
        int id = ids.takeFirst();
        KUrl url;
        url.setScheme( "http" );
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        url.addQueryItem( "method", "venue.getEvents" );
        url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        url.addQueryItem( "venue", QString::number( id ) );
        The::networkAccessManager()->getData( url, this,
             SLOT(venueEventsFetched(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
}

void
UpcomingEventsEngine::updateDataForArtist()
{
    if( !m_currentTrack )
        return;

    const QString &artistName = m_currentTrack->artist()->name();
    if( artistName.isEmpty() )
        return;

    // Prepares the url for LastFm request
    m_urls.clear();
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );
    m_urls << url;
    The::networkAccessManager()->getData( url, this,
         SLOT(artistEventsFetched(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsEngine::artistEventsFetched( const KUrl &url, QByteArray data,
                                           NetworkAccessManagerProxy::Error e )
{
    if( !m_urls.contains( url ) )
        return;

    m_urls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Error received getting upcoming artist events" << e.description;
        return;
    }

    QXmlStreamReader xml( data );
    LastFmEventXmlParser eventsParser( xml );
    removeAllData( "artistevents" );
    Plasma::DataEngine::Data engineData;
    if( eventsParser.read() )
    {
        LastFmEvent::List artistEvents = eventsParser.events();
        engineData[ "artist" ] = m_currentTrack->artist()->name();
        engineData[ "events" ] = qVariantFromValue( artistEvents );
    }
    setData( "artistevents", engineData );
}

void
UpcomingEventsEngine::venueEventsFetched( const KUrl &url, QByteArray data,
                                          NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Error received getting upcoming venue events" << e.description;
        return;
    }

    QXmlStreamReader xml( data );
    LastFmEventXmlParser eventsParser( xml );
    Plasma::DataEngine::Data engineData;
    if( eventsParser.read() )
    {
        LastFmEvent::List venueEvents = eventsParser.events();
        if( !venueEvents.isEmpty() )
        {
            engineData[ "venue"  ] = qVariantFromValue( venueEvents.first()->venue() );
            engineData[ "events" ] = qVariantFromValue( venueEvents );
        }
    }
    setData( "venueevents", engineData );
}

#include "UpcomingEventsEngine.moc"
