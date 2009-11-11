/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "SimilarArtistsEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"

using namespace Context;

SimilarArtistsEngine::SimilarArtistsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_upcomingEventsJob( 0 )
    , m_currentSelection( "artist" )
    , m_requested( true )
    , m_sources( "current" )
    , m_triedRefinedSearch( 0 )
{
    update();
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
    DEBUG_BLOCK
}

QStringList SimilarArtistsEngine::sources() const
{
    return m_sources;
}

bool SimilarArtistsEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // user has changed the maximum artists returned.
    if ( tokens.contains( "maxArtists" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "maxArtists" ) )  && ( tokens.size() > 2 ) )
            m_maxArtists = tokens.at( 2 ).toInt();
    
    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant());
    update();

    return true;
}

void SimilarArtistsEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
        update();
}

void SimilarArtistsEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK
    
    update();
}

void SimilarArtistsEngine::update()
{
    DEBUG_BLOCK

    // We've got a new track, great, let's fetch some info from SimilarArtists !
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
SimilarArtistsEngine::reloadSimilarArtists()
{

}

#include "SimilarArtistsEngine.moc"

