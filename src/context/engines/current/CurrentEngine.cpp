/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#include "CurrentEngine.h"

#include "core/support/Amarok.h"
#include "ContextView.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/collections/Collection.h"
#include "collection/CollectionManager.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/SourceInfoCapability.h"

#include <QVariant>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/MediaController>
#include <Phonon/MediaSource> //Needed for the slot

using namespace Context;

CurrentEngine::CurrentEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , Engine::EngineObserver( The::engineController() )
    , m_coverWidth( 0 )
    , m_state( Phonon::StoppedState )
	, m_qm( 0 )
	, m_qmTracks( 0 )
	, m_qmFavTracks( 0 )
    , m_currentArtist( 0 )
{
    DEBUG_BLOCK
    Q_UNUSED( args )

    m_sources << "current" << "albums";
    m_requested[ "current" ] = false;
    m_requested[ "albums" ] = false;
    
    m_timer = new QTimer(this);
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( stoppedState() ) );

    update();
}

CurrentEngine::~CurrentEngine()
{
    DEBUG_BLOCK
    if( m_qm )
        m_qm->abortQuery();
    delete m_qm;
    if( m_qmTracks )
        m_qmTracks->abortQuery();
    delete m_qmTracks;
    if( m_qmFavTracks )
        m_qmFavTracks->abortQuery();
    delete m_qmFavTracks;
}

QStringList
CurrentEngine::sources() const
{
    DEBUG_BLOCK

    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool
CurrentEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    removeAllData( name );
    setData( name, QVariant() );
    m_requested[ name ] = true;
    if( The::engineController()->currentTrack() )
    {
        if( m_qm )
            m_qm->abortQuery();
        if( m_qmTracks )
            m_qmTracks->abortQuery();
        if( m_qmFavTracks )
            m_qmFavTracks->abortQuery();
        update();

    }
    else
        m_timer->start();

    return true;
}

void
CurrentEngine::engineStateChanged(Phonon::State newState, Phonon::State )
{
    m_state = newState ;
}

void
CurrentEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    
    if( state == Current )
    {
        m_timer->stop();
        
        update();
    }
    else if( state == Home )
    {
        if( m_currentTrack )
        {
            unsubscribeFrom( m_currentTrack );
            if( m_currentTrack->album() )
                unsubscribeFrom( m_currentTrack->album() );
        }        
        m_timer->start( 1000 );
    }
}

void
CurrentEngine::stoppedState()
{
    DEBUG_BLOCK

    m_timer->stop();    

    //TODO
    // if we are in buffering state or loading state, do not show the recently album etc ...
    if ( m_state == Phonon::BufferingState || m_state== Phonon::LoadingState )
        return;
    
    removeAllData( "current" );
    setData( "current", "notrack", i18n( "No track playing") );
    removeAllData( "albums" );
    m_currentArtist = 0;


    if( m_requested[ "albums" ] )
    {
        // Collect data for the recently added albums
        setData( "albums", "headerText", QVariant( i18n( "Recently added albums" ) ) );

        Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
        if( coll )
        {
            if( m_qm )
                m_qm->reset();
            else
                m_qm = coll->queryMaker();
            m_qm->setQueryType( QueryMaker::Album );
            m_qm->excludeFilter( Meta::valAlbum, QString(), true, true );
            m_qm->orderBy( Meta::valCreateDate, true );
            m_qm->limitMaxResultSize( 5 );
            m_albums.clear();

            connect( m_qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                    SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
            connect( m_qm, SIGNAL( queryDone() ), SLOT( setupAlbumsData() ) );

            m_qm->run();
        }
    }

    // Get the latest tracks played:

    if( m_requested[ "current" ] )
    {
        Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
        if( !coll )
            return;

        if( m_qmTracks )
            m_qmTracks->reset();
        else
            m_qmTracks = coll->queryMaker();
        m_qmTracks->setQueryType( QueryMaker::Track );
        m_qmTracks->excludeFilter( Meta::valTitle, QString(), true, true );
        m_qmTracks->orderBy( Meta::valLastPlayed, true );
        m_qmTracks->limitMaxResultSize( 5 );

        m_latestTracks.clear();

        connect( m_qmTracks, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
                SLOT( resultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
        connect( m_qmTracks, SIGNAL( queryDone() ), SLOT( setupTracksData() ) );

        m_qmTracks->run();
    }

    // Get the favorite tracks:
    /* commenting out for now, we disabled the tabbar so this is just taking up CPU cycles
    if( m_qmFavTracks )
        m_qmFavTracks->reset();
    else
        m_qmFavTracks = coll->queryMaker();
    m_qmFavTracks->setQueryType( QueryMaker::Track );
    m_qmFavTracks->excludeFilter( Meta::valTitle, QString(), true, true );
    m_qmFavTracks->orderBy( Meta::valScore, true );
    m_qmFavTracks->limitMaxResultSize( 5 );

    m_qmFavTracks->run();

    connect( m_qmFavTracks, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
            SLOT( resultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
    connect( m_qmFavTracks, SIGNAL( queryDone() ), SLOT( setupTracksData() ) );
    */
    
}

void
CurrentEngine::metadataChanged( Meta::AlbumPtr album )
{
    DEBUG_BLOCK
    const int width = 156;
    setData( "current", "albumart", album->image( width ) );
}

void
CurrentEngine::metadataChanged( Meta::TrackPtr track )
{
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    setData( "current", "current", trackInfo );
    if( m_requested[ "albums" ] )
        update();
}

void
CurrentEngine::update()
{
    DEBUG_BLOCK

    if ( m_currentTrack )
    {
        unsubscribeFrom( m_currentTrack );
        if ( m_currentTrack->album() )
            unsubscribeFrom( m_currentTrack->album() );
    }

    m_currentTrack = The::engineController()->currentTrack();
    
    if( !m_currentTrack )
        return;
    
    subscribeTo( m_currentTrack );

    if( m_requested[ "current" ] )
    {

        QVariantMap trackInfo = Meta::Field::mapFromTrack( m_currentTrack );

        //const int width = coverWidth(); // this is always == 0, someone needs to setCoverWidth()
        const int width = 156; // workaround to make the art less grainy. 156 is the width of the nocover image
                            // there is no way to resize the currenttrack applet at this time, so this size
                            // will always look good.
        if( m_currentTrack->album() )
            subscribeTo( m_currentTrack->album() );

        removeAllData( "current" );

        if( m_currentTrack->album() )
        {
            QPixmap art = m_currentTrack->album()->image( width );
            setData( "current", "albumart",  QVariant( art ) );
        }
        else
            setData( "current", "albumart", QVariant( QPixmap() ) );

        setData( "current", "current", trackInfo );

        Capabilities::SourceInfoCapability *sic = m_currentTrack->create<Capabilities::SourceInfoCapability>();
        if( sic )
        {
            //is the source defined
            const QString source = sic->sourceName();
            debug() <<" We have source " <<source;
            if( !source.isEmpty() )
                setData( "current", "source_emblem", sic->scalableEmblem() );

            delete sic;
        }
        else
                setData( "current", "source_emblem",  QVariant( QPixmap() ) );
    }

    if( m_requested[ "albums" ] )
    {
        //generate data for album applet
        Meta::ArtistPtr artist = m_currentTrack->artist();

        //We need to update the albums data even if the artist is the same, since the current track is
        //most likely different, and thus the rendering of the albums applet should change
        if( artist )
        {
            m_currentArtist = artist;
            removeAllData( "albums" );
            Meta::AlbumList albums = artist->albums();
            setData( "albums", "headerText", QVariant( i18n( "Albums by %1", artist->name() ) ) );

            if( albums.count() == 0 )
            {
                //try searching the collection as we might be dealing with a non local track
                Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
                m_qm = coll->queryMaker();
                m_qm->setQueryType( QueryMaker::Album );
                m_qm->addMatch( artist );

                m_albums.clear();

                connect( m_qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                        SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
                connect( m_qm, SIGNAL( queryDone() ), SLOT( setupAlbumsData() ) );
                m_qm->run();

            }
            else
            {
                m_albums.clear();
                m_albums << albums;
                setupAlbumsData();
            }
        }
    }
}

void
CurrentEngine::setupAlbumsData()
{
    QVariant v;
    v.setValue( m_albums );
    setData( "albums", "albums", v );
}

void
CurrentEngine::setupTracksData()
{
    DEBUG_BLOCK

    QVariant v;
    if( sender() == m_qmTracks )
    {
        v.setValue( m_latestTracks );
        setData( "current", "lastTracks", v );
    }
    else if( sender() == m_qmFavTracks )
    {
        v.setValue( m_favoriteTracks );
        setData( "current", "favoriteTracks", v );
    }
}

void
CurrentEngine::resultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )

    m_albums.clear();
    m_albums << albums;
}

void
CurrentEngine::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    if( sender() == m_qmTracks )
    {
        m_latestTracks.clear();
        m_latestTracks << tracks;
    }
    else if( sender() == m_qmFavTracks )
    {
        m_favoriteTracks.clear();
        m_favoriteTracks << tracks;
    }
}


#include "CurrentEngine.moc"
