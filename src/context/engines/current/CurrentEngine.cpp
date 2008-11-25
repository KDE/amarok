/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CurrentEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"
#include "meta/SourceInfoCapability.h"

#include <QVariant>

using namespace Context;

CurrentEngine::CurrentEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_coverWidth( 0 )
    , m_requested( true )
    , m_currentArtist( 0 )
{
    DEBUG_BLOCK
    Q_UNUSED( args )
    m_sources = QStringList();
    m_sources << "current" << "albums";
    m_timer = new QTimer(this);
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( stoppedState() ) );
    update();
}

CurrentEngine::~CurrentEngine()
{
    DEBUG_BLOCK
}

QStringList CurrentEngine::sources() const
{
    DEBUG_BLOCK
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool CurrentEngine::sourceRequested( const QString& name )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
/*    m_sources << name;    // we are already enabled if we are alive*/
    removeAllData( name );
    setData( name, QVariant());
    update();
    m_requested = true;
    return true;
}

void CurrentEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    
    if( state == Current )
    {
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
    removeAllData( "current" );
    setData( "current", "notrack", i18n( "No track playing") );
    removeAllData( "albums" );
    m_currentArtist = 0;

    // Collect data for the recently added albums
    setData( "albums", "headerText", QVariant( i18n( "Recently added albums" ) ) );
    
    Collection *coll = CollectionManager::instance()->primaryCollection();
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


    // Get the latest tracks played

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

void CurrentEngine::metadataChanged( Meta::AlbumPtr album )
{
    DEBUG_BLOCK
    setData( "current", "albumart", album->image( coverWidth() ) );
}

void
CurrentEngine::metadataChanged( Meta::TrackPtr track )
{
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    setData( "current", "current", trackInfo );
}

void CurrentEngine::update()
{
    DEBUG_BLOCK

    if ( m_currentTrack )
    {
        unsubscribeFrom( m_currentTrack );
        if ( m_currentTrack->album() )
        {
            unsubscribeFrom( m_currentTrack->album() );
        }
    }

    m_currentTrack = The::engineController()->currentTrack();
    
    if( !m_currentTrack )
    {
        return;
    }
    
    subscribeTo( m_currentTrack );

    setData( "current", "notrack" , QString() );

    QVariantMap trackInfo = Meta::Field::mapFromTrack( m_currentTrack );

    int width = coverWidth();
    if( m_currentTrack->album() )
        subscribeTo( m_currentTrack->album() );
    
    removeAllData( "current" );
        
    if( m_currentTrack->album() )
    {
        //add a source info emblem ( if available ) to the cover
        QPixmap art = m_currentTrack->album()->image( width );
        setData( "current", "albumart",  QVariant( art ) );
     }
    else
        setData( "current", "albumart", QVariant( QPixmap() ) );

    setData( "current", "current", trackInfo );

    Meta::SourceInfoCapability *sic = m_currentTrack->as<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if( !source.isEmpty() )
            setData( "current", "source_emblem",  QVariant( sic->emblem() ) );

        delete sic;
    }
    else
        setData( "current", "source_emblem",  QVariant( QPixmap() ) );

    //generate data for album applet
    Meta::ArtistPtr artist = m_currentTrack->artist();
    //if it's the same artist we don't have to update albums data
    if( m_currentArtist != artist )
    {
        m_currentArtist = artist;
        removeAllData( "albums" );
        Meta::AlbumList albums = artist->albums();
        setData( "albums", "headerText", QVariant( i18n( "Albums by %1", artist->name() ) ) );

        if( albums.count() == 0 )
        {
            //try searching the collection as we might be dealing with a non local track
            Collection *coll = CollectionManager::instance()->primaryCollection();
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
    v.setValue( m_latestTracks );
    setData( "current", "tracks", v );
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
    m_latestTracks.clear();
    m_latestTracks << tracks;
}


#include "CurrentEngine.moc"
