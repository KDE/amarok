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

    if( m_currentTrack != track )
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

QList< LastFmEvent >
UpcomingEventsEngine::upcomingEvents()
{
    return m_upcomingEvents;
}

void
UpcomingEventsEngine::upcomingEventsResultFetched( const KUrl &url, QByteArray data,
                                                   NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    m_upcomingEvents.clear();

    if( e.code != QNetworkReply::NoError )
        return;

    if( !data.isNull() )
    {
        m_xml = QString::fromUtf8( data, data.size() );
    }
    else
    {
        return;
    }

    QDomDocument doc;
    QXmlStreamReader xmlReader;
    xmlReader.addData( m_xml );
    doc.setContent( m_xml );
    upcomingEventsParseResult( xmlReader );
}

void
UpcomingEventsEngine::upcomingEventsParseResult( QXmlStreamReader& xmlReader )
{
    DEBUG_BLOCK
    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        QLocale::setDefault(QLocale::English);
        QStringList artists;
        QString title = "";
        QString location = "";
        QDateTime startDate;
        KUrl url;
        KUrl imageUrl;

        if(xmlReader.name() == "event")
        {
            //Read title
            while(!xmlReader.atEnd() && !xmlReader.hasError() && xmlReader.name() != "title") xmlReader.readNext();
            if(!xmlReader.atEnd() && !xmlReader.hasError()) title = xmlReader.readElementText();

            //Read artists
            while( !xmlReader.atEnd() && !xmlReader.hasError() && xmlReader.name() != "artist" ) xmlReader.readNext();
            if( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                while( xmlReader.name() == "artist" )
                {
                    QString artist = xmlReader.readElementText();
                    QString currentArtist = m_currentTrack->artist()->name();
                    if( artist != currentArtist )
                        artists.append( artist );
                    while( !xmlReader.isStartElement() ) xmlReader.readNext();
                }
            }

            //Read Location
            while(!xmlReader.atEnd() && !xmlReader.hasError() && xmlReader.name() != "city") xmlReader.readNext();
            if(!xmlReader.atEnd() && !xmlReader.hasError())
            {
                location.append(xmlReader.readElementText());
                location.append(", ");
            }

            while(!xmlReader.atEnd() && !xmlReader.hasError() && xmlReader.name() != "country") xmlReader.readNext();
            if(!xmlReader.atEnd() && !xmlReader.hasError())
            {
                location.append(xmlReader.readElementText());
            }

            //Read URL
            while(!xmlReader.atEnd() && !xmlReader.hasError() && xmlReader.name() != "url") xmlReader.readNext();
            if(!xmlReader.atEnd() && !xmlReader.hasError())
            {
                url = KUrl(xmlReader.readElementText());
            }

            //Read Image
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "image")
            {
                xmlReader.readNext ();
            }

            //we search the large image, in the panel of the image proposed by lastFM
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.attributes().value("size")!="large")
            {
                xmlReader.readNext ();
            }

            if (!xmlReader.atEnd() && !xmlReader.hasError() )
            {
                imageUrl = KUrl( xmlReader.readElementText() );
            }

            //Read startDate
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "startDate")
            {
                xmlReader.readNext ();
            }
            if( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                QString startDateString = xmlReader.readElementText();
                startDate = QLocale().toDateTime( startDateString, "ddd, dd MMM yyyy HH:mm:ss" );
            }

            // LastFm event creation
            if( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                LastFmEvent event;
                event.setArtists( artists );
                event.setName( title );
                event.setLocation( location );
                event.setDate( startDate );
                event.setUrl( url );
                event.setSmallImageUrl( imageUrl );
                m_upcomingEvents.append( event );
            }
        }
        xmlReader.readNext();
    }

    Plasma::DataEngine::Data data;
    data[ "artist" ] =  m_currentTrack->artist()->name();
    data[ "LastFmEvent" ] = qVariantFromValue( m_upcomingEvents );
    setData( "artistevents", data );
}

#include "UpcomingEventsEngine.moc"
