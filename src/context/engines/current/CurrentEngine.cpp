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
#include "core/collections/QueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/SourceInfoCapability.h"

#include <KConfigDialog>

#include <QVariant>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/MediaController>
#include <Phonon/MediaSource> //Needed for the slot

using namespace Context;

CurrentEngine::CurrentEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , m_coverWidth( 0 )
    , m_currentArtist( 0 )
{
    DEBUG_BLOCK
    Q_UNUSED( args )
    m_sources << "current" << "albums";
    m_requested[ "current" ] = true;
    m_requested[ "albums" ] = false;

    EngineController* engine = The::engineController();

    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
             this, SLOT( trackChanged( Meta::TrackPtr ) ) );
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped() ) );

    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( metadataChanged( Meta::TrackPtr ) ) );
    connect( engine, SIGNAL( albumMetadataChanged( Meta::AlbumPtr ) ),
             this, SLOT( metadataChanged( Meta::AlbumPtr ) ) );
}

CurrentEngine::~CurrentEngine()
{
}

void
CurrentEngine::init()
{
    Plasma::DataEngine::init();
    update( The::engineController()->currentTrack() );
}

QStringList
CurrentEngine::sources() const
{
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool
CurrentEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    removeAllData( name ); // unneeded?
    m_requested[ name ] = true;
    update( The::engineController()->currentTrack() );
    return true;
}

void
CurrentEngine::metadataChanged( Meta::AlbumPtr album )
{
    const int width = 156; // ARGH, hardcoded?
    setData( "current", "albumart", album->image( width ) );
}

void
CurrentEngine::metadataChanged( Meta::TrackPtr track )
{
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    setData( "current", "current", trackInfo );
    if( m_requested[ "albums" ] )
        update( track );
}

void
CurrentEngine::trackChanged( Meta::TrackPtr track )
{
    update( track );
}

void
CurrentEngine::stopped()
{
    DEBUG_BLOCK

    removeAllData( "current" );
    setData( "current", "notrack", i18n( "No track playing") );
    removeAllData( "albums" );
    m_currentArtist = 0;

    if( m_requested[ "albums" ] )
    {
        // Collect data for the recently added albums
        setData( "albums", "headerText", QVariant( i18n( "Recently added albums" ) ) );

        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        if( qm )
        {
            qm->setAutoDelete( true );
            qm->setQueryType( Collections::QueryMaker::Album );
            qm->excludeFilter( Meta::valAlbum, QString(), true, true );
            qm->orderBy( Meta::valCreateDate, true );
            qm->limitMaxResultSize( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) );
            m_albums.clear();

            connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                    SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
            connect( qm, SIGNAL( queryDone() ), SLOT( setupAlbumsData() ) );

            qm->run();
        }
    }

    // Get the latest tracks played:

    if( m_requested[ "current" ] )
    {
        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        if( !qm )
            return;

        qm->setAutoDelete( true );
        qm->setQueryType( Collections::QueryMaker::Track );
        qm->excludeFilter( Meta::valTitle, QString(), true, true );
        qm->orderBy( Meta::valLastPlayed, true );
        qm->excludeFilter( Meta::valLastPlayed, "2147483647" );
        qm->limitMaxResultSize( 5 );

        m_latestTracks.clear();

        connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
                SLOT( resultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( setupTracksData() ) );

        qm->run();
    }

    // Get the favorite tracks:
    /* commenting out for now, we disabled the tabbar so this is just taking up CPU cycles
    if( m_qmFavTracks )
        m_qmFavTracks->reset();
    else
        m_qmFavTracks = coll->queryMaker();
    m_qmFavTracks->setQueryType( Collections::QueryMaker::Track );
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
CurrentEngine::update( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    if( !track )
    {
        stopped();
        return;
    }

    if( m_requested[ "current" ] )
    {

        QVariantMap trackInfo = Meta::Field::mapFromTrack( track );

        const int width = m_coverWidth;
        removeAllData( "current" );

        Meta::AlbumPtr album = track->album();
        if( album )
        {
            QPixmap art = album->image( width );
            setData( "current", "albumart",  QVariant( art ) );
        }
        else
            setData( "current", "albumart", QVariant( QPixmap() ) );

        setData( "current", "current", trackInfo );

        Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
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
        Meta::ArtistPtr artist = track->artist();

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
                Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
                qm->setAutoDelete( true );
                qm->setQueryType( Collections::QueryMaker::Album );
                qm->addMatch( artist );

                m_albums.clear();

                connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                        SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
                connect( qm, SIGNAL( queryDone() ), SLOT( setupAlbumsData() ) );
                qm->run();

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
    QVariant v;
    v.setValue( m_latestTracks );
    setData( "current", "lastTracks", v );
    /*
    else if( sender() == m_qmFavTracks )
    {
        v.setValue( m_favoriteTracks );
        setData( "current", "favoriteTracks", v );
    }
    */
}

void
CurrentEngine::resultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    // DEBUG_BLOCK
    Q_UNUSED( collectionId )

    m_albums.clear();
    m_albums << albums;
}

void
CurrentEngine::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    // DEBUG_BLOCK
    Q_UNUSED( collectionId )
    m_latestTracks.clear();
    m_latestTracks << tracks;
    /*
    else if( sender() == m_qmFavTracks )
    {
        m_favoriteTracks.clear();
        m_favoriteTracks << tracks;
    }
    */
}


#include "CurrentEngine.moc"
