/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

    // User has requested a reload
    if( tokens.contains( "reload" ) && tokens.size() > 1 )
    {
        if ( tokens.at( 1 ) == QString( "reload" ) )
        {
            reloadUpcomingEvents();
            return true;
        }
    }

    // User has clicked on a link, let's fetch the page
    if( tokens.contains( "get" ) && tokens.size() > 1 )
    {
        if ( ( tokens.at( 1 ) == QString( "get" ) ) && ( tokens.size() > 3 ) )
        {
            // TODO adapt this to the Last.FM webservices
            //m_wikiCurrentUrl = tokens.at( 2 ) + QString( ":" ) + tokens.at( 3 );
            // removeSource( "wikipedia" );
            // setData( "wikipedia", "busy", "busy" );
            //m_upcomingEventsJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
            //connect( m_upcomingEventsJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
            return true;
        }
    }

    // user has selected the timespan.
    if ( tokens.contains( "timeSpan" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "timeSpan" ) )  && ( tokens.size() > 2 ) )
            m_timeSpan = tokens.at( 2 );

    // user has selected the timespan.
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
    if( state == Current && m_requested )
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
    if( selection() == "artist" ) 
    {
        if( currentTrack->artist() )
        {
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
                artistName = currentTrack->artist()->name();
            else
                artistName = currentTrack->artist()->prettyName();
        }
    }

}

void
UpcomingEventsEngine::reloadUpcomingEvents()
{

}

#include "UpcomingEventsEngine.moc"

