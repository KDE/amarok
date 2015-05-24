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

#include "UpcomingEventsEngine.h"

#include "context/ContextView.h"
#include "context/applets/upcomingevents/LastFmEventXmlParser.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"

#include <KDateTime>

#include <QXmlStreamReader>

AMAROK_EXPORT_DATAENGINE( upcomingEvents, UpcomingEventsEngine )

using namespace Context;

UpcomingEventsEngine::UpcomingEventsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
{
    m_timeSpan = Amarok::config("UpcomingEvents Applet").readEntry( "timeSpan", "AllEvents" );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(updateDataForArtist()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(updateDataForArtist()) );
}

UpcomingEventsEngine::~UpcomingEventsEngine()
{
}

bool
UpcomingEventsEngine::sourceRequestEvent( const QString &source )
{
    if( source == "artistevents" )
    {
        updateDataForArtist();
        return false; // data is not ready yet, but will be soon
    }
    else if( source == "venueevents" )
    {
        m_venueIds.clear();
        QStringList venues = Amarok::config("UpcomingEvents Applet").readEntry( "favVenues", QStringList() );
        foreach( const QString &venue, venues )
        {
            QStringList frag = venue.split( QChar(';') );
            m_venueIds << frag.at( 0 ).toInt();
        }
        updateDataForVenues();
        return true;
    }
    else if( source == "venueevents:update" )
    {
        removeAllData( source );
        sourceRequestEvent( "venueevents" );
    }
    else if( source == "timespan:update" )
    {
        // user has changed the timespan.
        m_timeSpan = Amarok::config("UpcomingEvents Applet").readEntry( "timeSpan", "AllEvents" );
        sourceRequestEvent( "venueevents:update" );
        updateDataForArtist();
        return true;
    }
    return false;
}

void
UpcomingEventsEngine::updateDataForVenues()
{
    if( !m_venueIds.isEmpty() )
    {
        int id = m_venueIds.takeFirst();
        QUrl url;
        url.setScheme( "http" );
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        url.addQueryItem( "method", "venue.getEvents" );
        url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        url.addQueryItem( "venue", QString::number( id ) );
        The::networkAccessManager()->getData( url, this,
             SLOT(venueEventsFetched(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        QTimer::singleShot( 50, this, SLOT(updateDataForVenues()) );
    }
}

void
UpcomingEventsEngine::updateDataForArtist()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;

    Meta::ArtistPtr artist = track->artist();
    if( !artist || artist == m_currentArtist || artist->name().isEmpty() )
        return;

    m_currentArtist = artist;

    // Prepares the url for LastFm request
    m_urls.clear();
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", m_currentArtist->name() );
    m_urls << url;
    The::networkAccessManager()->getData( url, this,
         SLOT(artistEventsFetched(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsEngine::artistEventsFetched( const QUrl &url, QByteArray data,
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
        LastFmEvent::List artistEvents = filterEvents( eventsParser.events() );
        engineData[ "artist" ] = m_currentArtist->name();
        engineData[ "events" ] = qVariantFromValue( artistEvents );
    }
    setData( "artistevents", engineData );
}

void
UpcomingEventsEngine::venueEventsFetched( const QUrl &url, QByteArray data,
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
        LastFmEvent::List venueEvents = filterEvents( eventsParser.events() );
        if( !venueEvents.isEmpty() )
        {
            engineData[ "venue"  ] = qVariantFromValue( venueEvents.first()->venue() );
            engineData[ "events" ] = qVariantFromValue( venueEvents );
        }
    }
    setData( "venueevents", engineData );
}

LastFmEvent::List
UpcomingEventsEngine::filterEvents( const LastFmEvent::List &events ) const
{
    KDateTime currentTime( KDateTime::currentLocalDateTime() );

    if( m_timeSpan == "ThisWeek")
        currentTime = currentTime.addDays( 7 );
    else if( m_timeSpan == "ThisMonth" )
        currentTime = currentTime.addMonths( 1 );
    else if( m_timeSpan == "ThisYear" )
        currentTime = currentTime.addYears( 1 );
    else
        return events; // no filtering is done

    LastFmEvent::List newEvents;
    foreach( const LastFmEventPtr &event, events )
    {
        if( event->date() < currentTime )
            newEvents << event;
    }
    return newEvents;
}

