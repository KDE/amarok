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
        , ContextObserver( ContextView::self() )
        , m_timeSpan( "AllEvents" )
        , m_currentSelection( "artist" )
        , m_requested( true )
        , m_sources( "current" )
{
    update();
}

UpcomingEventsEngine::~UpcomingEventsEngine()
{
    DEBUG_BLOCK
}

QStringList
UpcomingEventsEngine::sources() const
{
    return m_sources;
}

bool
UpcomingEventsEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    // someone is asking for data
    m_requested = true;

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

void
UpcomingEventsEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void
UpcomingEventsEngine::metadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    if( m_currentTrack->artist() != track->artist() )
        update();
}

void
UpcomingEventsEngine::update()
{
    DEBUG_BLOCK

    static QString s_lastArtistName;
    m_artistName = "";

    // Gets the current track
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
                m_artistName = currentTrack->artist()->name();
            else
                m_artistName = currentTrack->artist()->prettyName();
        }

        // Sends the artist name if exists, "Unkown artist" if not
        if ( m_artistName.compare( "" ) == 0 )
            setData( "upcomingEvents", "artist", "Unknown artist" );
        else
            setData( "upcomingEvents", "artist", m_artistName );
    }

    // Resizes the cover to better visibility
    QPixmap cover = m_currentTrack->album()->image( 156 );

    // Stored the last artist name to not send the same data twice
    if( m_artistName != s_lastArtistName )
    {
        upcomingEventsRequest( m_artistName );
        s_lastArtistName = m_artistName;
    }
}

QList< LastFmEvent >
UpcomingEventsEngine::upcomingEvents()
{
    return m_upcomingEvents;
}

void
UpcomingEventsEngine::upcomingEventsRequest(const QString& artist_name)
{
    // Prepares the url for LastFm request
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );
    m_url = url;

    QNetworkRequest req( url );
    The::networkAccessManager()->get( req );
    The::networkAccessManager()->getData( url, this,
         SLOT(upcomingEventsResultFetched(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsEngine::upcomingEventsResultFetched( const KUrl &url, QByteArray data,
                                                   NetworkAccessManagerProxy::Error e )
{
    if( m_url != url )
        return;

    m_url.clear();
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
                    if( artist !=  m_artistName)
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

    QVariant variant ( QMetaType::type( "LastFmEvent::List" ), &m_upcomingEvents );
    removeData("upcomingEvents", "LastFmEvent");
    setData ( "upcomingEvents", "LastFmEvent", variant );
}

void
UpcomingEventsEngine::setSelection( const QString& selection )
{
    m_currentSelection = selection;
}

QString
UpcomingEventsEngine::selection()
{
     return m_currentSelection;
}

#include "UpcomingEventsEngine.moc"
