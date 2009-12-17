/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#include "UpcomingEventsEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include <lastfm/XmlQuery>
#include <lastfm/ws.h>

using namespace Context;

UpcomingEventsEngine::UpcomingEventsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_upcomingEventsJob( 0 )
        , m_currentSelection( "artist" )
        , m_requested( true )
        , m_sources( "current" )
        , m_timeSpan( "AllEvents" )
        , m_triedRefinedSearch( 0 )
{
    update();
}

UpcomingEventsEngine::~UpcomingEventsEngine()
{
    DEBUG_BLOCK
}

QStringList UpcomingEventsEngine::sources() const
{
    return m_sources;
}

bool UpcomingEventsEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // user has changed the timespan.
    if ( tokens.contains( "timeSpan" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "timeSpan" ) )  && ( tokens.size() > 2 ) )
            m_timeSpan = tokens.at( 2 );

    // user has enabled or disabled showing addresses as links
    if ( tokens.contains( "enabledLinks" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "enabledLinks" ) )  && ( tokens.size() > 2 ) )
            m_enabledLinks = (tokens.at( 2 ) == QString(Qt::Checked));

    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant());
    update();

    return true;
}

void UpcomingEventsEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void UpcomingEventsEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    update();
}

void UpcomingEventsEngine::update()
{
    DEBUG_BLOCK

    // We've got a new track, great, let's fetch some info from UpcomingEvents !
    m_triedRefinedSearch = 0;
    QString artistName;


    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;

    DataEngine::Data data;
    // default, or applet told us to fetch artist
    if ( selection() == "artist" )
    {
        if ( currentTrack->artist() )
        {
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                    ( currentTrack->playableUrl().protocol() == "daap" ) ||
                    !The::engineController()->isStream() )
                artistName = currentTrack->artist()->name();
            else
                artistName = currentTrack->artist()->prettyName();
        }
        if (artistName.compare( "") == 0)
            setData( "upcomingEvents", "artist", "Unknown artist" );
        else
            setData( "upcomingEvents", "artist", artistName );
    }
    QPixmap cover = m_currentTrack->album()->image( 156 );
    
    setData( "upcomingEvents", "cover",  QVariant( cover ) );
    /*setData( "upcomingEvents", "eventName", "Concert de Noël" );
    setData( "upcomingEvents", "eventDate", "2009-12-25" );
    setData( "upcomingEvents", "eventUrl", "<a href='http://www.google.com'>"
                                           "<font size='2' family='Arial, Helvetica, sans-serif' color='blue'><b>http://www.google.fr</b></font>"
                                           "</a>" );*/
   upcomingEventsRequest( artistName );
   QVariant variant ( QMetaType::type( "LastFmEvent::LastFmEventList" ), &m_upcomingEvents );
   
   setData ( "upcomingEvents", "LastFmEvent", variant );
}

void UpcomingEventsEngine::reloadUpcomingEvents()
{

}


QList< LastFmEvent >
UpcomingEventsEngine::getUpcomingEvents()
{
    return m_upcomingEvents;
}



void
UpcomingEventsEngine::upcomingEventsRequest(const QString& artist_name)
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );

    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(upcomingEventsResultFetched( KJob* )) );
    //job->start();
}

void
UpcomingEventsEngine::upcomingEventsResultFetched (KJob* job) // SLOT
{
    m_upcomingEvents.clear();
    
    if( !job->error() )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    else
        return;
    
    QDomDocument doc;
    doc.setContent( m_xml );
    upcomingEventsParseResult(doc);
}


void
UpcomingEventsEngine::upcomingEventsParseResult( QDomDocument doc )
{
    
    const QDomNode events = doc.documentElement().namedItem( "events" );

    QDomNode n = events.firstChild();
    while( !n.isNull() )
    {
        // Event name
        QDomNode titleNode = n.namedItem( "title" );
        QDomElement e = titleNode.toElement();
        QString title;
        if( !e.isNull() )
        {            
            title = e.text();
        }

        // Event participants
        QDomNode artistsNode = n.namedItem( "artists" );
        QDomNode currentArtistNode = artistsNode.firstChild();
        QStringList artists;
        while( !currentArtistNode.isNull() )
        {
            QDomElement currentArtistElement = currentArtistNode.toElement();
            if( currentArtistElement.tagName() == "artist" )
            {
                if( !currentArtistElement.isNull() )
                {
                    artists.append( currentArtistElement.text() );
                }
            }
            currentArtistNode = currentArtistNode.nextSibling();
        }

        // Event date
        QDomNode startDateNode = n.namedItem( "startDate" );
        QDomElement startDateElement = startDateNode.toElement();
        QDateTime startDate;
        if( !startDateElement.isNull() )
        {
            QString startDateString = startDateElement.text();            
            startDate.setDate( QDate::fromString( startDateString.section( "", 0, 16 ), "ddd, dd MMM yyyy" ) );
            startDate.setTime( QTime::fromString( startDateString.section( "", 18 ), "HH:mm:ss" ) );
        }

        // Event url
        QDomNode urlNode = n.namedItem( "url" );
        QDomElement urlElement = urlNode.toElement();
        KUrl url;
        if( !urlElement.isNull() )
        {
            url = KUrl( urlElement.text() );
        }

        // Small image url
        QDomNode imageUrlNode = n.namedItem( "image" );
        QDomElement imageUrlElement = imageUrlNode.toElement();
        KUrl imageUrl;
        if( !imageUrlElement.isNull() )
        {
            while ( imageUrlElement.attributeNode( "size" ).value() != "large" )
            {
                imageUrlElement = imageUrlElement.nextSiblingElement();
            }
            imageUrl = KUrl( imageUrlElement.text() );
        }
        m_upcomingEvents.append( LastFmEvent( artists, title, startDate, imageUrl, url ) );
        
        n = n.nextSibling();
    }
    update();
}

#include "UpcomingEventsEngine.moc"

