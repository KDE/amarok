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
#include "Amarok.h"
#include "Debug.h"
#include "ContextView.h"
#include "EngineController.h"

// LastFm
#include <lastfm/XmlQuery>
#include <lastfm/ws.h>

// Qt
#include <QXmlStreamReader>


using namespace Context;

/**
 * \brief Constructor
 *
 * Creates a new instance of UpcomingEventsEngine
 */
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

/**
 * \brief Destructor
 *
 * Destroys an UpcomingEventsEngine instance
 */
UpcomingEventsEngine::~UpcomingEventsEngine()
{
    DEBUG_BLOCK
}

/**
 * Returns the sources
 */
QStringList
UpcomingEventsEngine::sources() const
{
    return m_sources;
}

/**
 * When a source that does not currently exist is requested by the
 * consumer, this method is called to give the DataEngine the
 * opportunity to create one.
 *
 * The name of the data source (e.g. the source parameter passed into
 * setData) must be the same as the name passed to sourceRequestEvent
 * otherwise the requesting visualization may not receive notice of a
 * data update.
 *
 * If the source can not be populated with data immediately (e.g. due to
 * an asynchronous data acquisition method such as an HTTP request)
 * the source must still be created, even if it is empty. This can
 * be accomplished in these cases with the follow line:
 *
 *      setData(name, DataEngine::Data());
 *
 * \param source : the name of the source that has been requested
 * \return true if a DataContainer was set up, false otherwise
 */
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

/**
 * Overriden from Context::Observer
 */
void
UpcomingEventsEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

/**
 * This method is called when the metadata of a track has changed.
 * The called class may not cache the pointer
 */
void
UpcomingEventsEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    update();
}

/**
 * Sends the data to the observers (e.g UpcomingEventsApplet)
 */
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

/**
 * Returns all the upcoming events
 */
QList< LastFmEvent >
UpcomingEventsEngine::upcomingEvents()
{
    return m_upcomingEvents;
}

/**
 * Fetches the upcoming events for an artist thanks to the LastFm WebService
 *
 * \param artist_name the name of the artist
 * \return a list of events
 */
void
UpcomingEventsEngine::upcomingEventsRequest(const QString& artist_name)
{
    // Prepares the url for LastFm request
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getEvents" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );

    // The results are parsed only when the job is finished
    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( upcomingEventsResultFetched( KJob* ) ) );
}

/**
 * Runs the KJob to parse the XML file
 */
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
    QXmlStreamReader xmlReader;
    xmlReader.addData( m_xml );
    doc.setContent( m_xml );
    upcomingEventsParseResult( xmlReader );
}

/**
 * Parses the upcoming events request result
 */
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

    QVariant variant ( QMetaType::type( "LastFmEvent::LastFmEventList" ), &m_upcomingEvents );
    removeData("upcomingEvents", "LastFmEvent");
    setData ( "upcomingEvents", "LastFmEvent", variant );
}

/**
 * Sets the selection
 *
 * \param selection : the current selection
 */
void
UpcomingEventsEngine::setSelection( const QString& selection )
{
    m_currentSelection = selection;
}

/**
 * Returns the current selection
 */
QString
UpcomingEventsEngine::selection()
{
     return m_currentSelection;
}

#include "UpcomingEventsEngine.moc"
