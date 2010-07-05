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
        updateDataForArtist();
        return false; // data is not ready yet, but will be soon
    }
    else if( source == "venue" )
    {
        // TODO: handle venue source requests
    }
    else if( source.startsWith( "timespan" ) )
    {
        // user has changed the timespan.
        QStringList tokens = source.split( ':' );
        if( tokens.size() > 1 )
            m_timeSpan = tokens.at( 1 );
        removeAllData( source );
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
        if( m_currentTrack && m_currentTrack->artist() != track->artist() )
            updateDataForArtist();
        unsubscribeFrom( m_currentTrack );
        subscribeTo( track );
        m_currentTrack = track;
    }
}

void
UpcomingEventsEngine::updateDataForArtist()
{
    DEBUG_BLOCK
    if( !m_currentTrack )
        return;

    const QString &artistName = m_currentTrack->artist()->name();
    if( artistName.isEmpty() )
        return;

    // Prepares the url for LastFm request
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );
    The::networkAccessManager()->getData( url, this,
         SLOT(upcomingEventsResultFetched(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsEngine::upcomingEventsResultFetched( const KUrl &/*url*/, QByteArray data,
                                                   NetworkAccessManagerProxy::Error e )
{
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Error received getting upcoming events" << e.description;
        return;
    }

    LastFmEvent::List artistEvents;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "event" )
        {
            QHash<QString, QString> artists;
            LastFmEvent event;
            while( !xml.atEnd() )
            {
                xml.readNext();
                const QStringRef &n = xml.name();
                if( xml.isEndElement() && n == "event" )
                    break;

                if( xml.isStartElement() )
                {
                    const QXmlStreamAttributes &a = xml.attributes();
                    if( n == "title" )
                        event.setName(  xml.readElementText() );
                    else if( n == "artists" )
                        artists = readEventArtists( xml );
                    else if( n == "venue" )
                        event.setVenue( readVenue( xml ) );
                    else if( n == "startDate" )
                        event.setDate( KDateTime::fromString( xml.readElementText(), "%a, %d %b %Y %H:%M:%S" ) );
                    else if( n == "image" && a.hasAttribute("size") )
                        event.setImageUrl( LastFmEvent::stringToImageSize(a.value("size").toString()), KUrl( xml.readElementText() ) );
                    else if( n == "url" )
                        event.setUrl( KUrl( xml.readElementText() ) );
                    else if( n == "attendance" )
                        event.setAttendance( xml.readElementText().toInt() );
                    else if( n == "cancelled" )
                        event.setCancelled( bool( xml.readElementText().toInt() ) );
                    else if( n == "tags" )
                        event.setTags( readEventTags( xml ) );
                    else
                        xml.skipCurrentElement();
                }
            }
            event.setHeadliner( artists.value("headliner") );
            event.setParticipants( artists.values("artist") );
            artistEvents.append( event );
        }
    }
    Plasma::DataEngine::Data EngineData;
    EngineData[ "artist" ] =  m_currentTrack->artist()->name();
    EngineData[ "LastFmEvent" ] = qVariantFromValue( artistEvents );
    setData( "artistevents", EngineData );
}

QStringList
UpcomingEventsEngine::readEventTags( QXmlStreamReader &xml )
{
    QStringList tags;
    while( !xml.atEnd() )
    {
        xml.readNext();
        if( xml.isEndElement() && xml.name() == "tags" )
            break;

        if( xml.isStartElement() )
        {
            if( xml.name() == "tag" )
                tags << xml.readElementText();
            else
                xml.skipCurrentElement();
        }
    }
    return tags;
}

QHash<QString, QString>
UpcomingEventsEngine::readEventArtists( QXmlStreamReader &xml )
{
    QHash<QString, QString> artists;
    while( !xml.atEnd() )
    {
        xml.readNext();
        if( xml.isEndElement() && xml.name() == "artists" )
            break;

        if( xml.isStartElement() )
        {
            if( xml.name() == "artist" )
            {
                QString artist = xml.readElementText();
                if( artist != m_currentTrack->artist()->name() )
                    artists.insertMulti( "artist", artist );
            }
            else if( xml.name() == "headliner" )
                artists.insert( "headliner", xml.readElementText() );
            else
                xml.skipCurrentElement();
        }
    }
    return artists;
}

LastFmVenuePtr
UpcomingEventsEngine::readVenue( QXmlStreamReader &xml )
{
    LastFmVenuePtr venue( new LastFmVenue );
    while( !xml.atEnd() )
    {
        xml.readNext();
        const QStringRef &n = xml.name();
        if( xml.isEndElement() && n == "venue" )
            break;

        if( xml.isStartElement() )
        {
            const QXmlStreamAttributes &a = xml.attributes();
            if( n == "id" )
                venue->id = xml.readElementText().toInt();
            else if( n == "name" )
                venue->name = xml.readElementText();
            else if( n == "website" )
                venue->website = xml.readElementText();
            else if( n == "url" )
                venue->url = xml.readElementText();
            else if( n == "phonenumber" )
                venue->phoneNumber = xml.readElementText();
            else if( n == "image" && a.hasAttribute("size") )
                venue->imageUrls[ LastFmEvent::stringToImageSize(a.value("size").toString()) ] = KUrl( xml.readElementText() );
            else if( n == "location" )
            {
                LastFmLocationPtr location( new LastFmLocation );
                while( !xml.atEnd() )
                {
                    xml.readNext();
                    const QStringRef &n = xml.name();
                    if( xml.isEndElement() && n == "location" )
                        break;

                    if( xml.isStartElement() )
                    {
                        if( n == "city" )
                            location->city = xml.readElementText();
                        else if( n == "country" )
                            location->country = xml.readElementText();
                        else if( n == "street" )
                            location->street = xml.readElementText();
                        else if( n == "postalcode" )
                            location->postalCode = xml.readElementText();
                        else if( n == "geo:point" )
                        {
                            while( !xml.atEnd() )
                            {
                                xml.readNext();
                                if( xml.isEndElement() && xml.name() == "geo:point" )
                                    break;

                                if( xml.isStartElement() )
                                {
                                    if( xml.name() == "geo:lat" )
                                        location->lattitude = xml.readElementText();
                                    else if( xml.name() == "geo:long" )
                                        location->longitude = xml.readElementText();
                                    else
                                        xml.skipCurrentElement();
                                }
                            }
                        }
                        else
                            xml.skipCurrentElement();
                    }
                }
                venue->location = location;
            }
            else
                xml.skipCurrentElement();
        }
    }
    return venue;
}

#include "UpcomingEventsEngine.moc"
