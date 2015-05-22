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

#define DEBUG_PREFIX "CurrentEngine"

#include "CurrentEngine.h"

#include "EngineController.h"
#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Amarok.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverCache.h"

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
    , m_coverCacheKey( 0 )
    , m_lastQueryMaker( 0 )
{
    Q_UNUSED( args )

    m_sources << QLatin1String("current") << QLatin1String("albums");
    m_requested[ QLatin1String("current") ] = false;
    m_requested[ QLatin1String("albums")  ] = false;
    EngineController* engine = The::engineController();

    connect( engine, SIGNAL(trackPlaying(Meta::TrackPtr)),
             this, SLOT(trackPlaying(Meta::TrackPtr)) );
    connect( engine, SIGNAL(stopped(qint64,qint64)),
             this, SLOT(stopped()) );

    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(metadataChanged(Meta::TrackPtr)) );
    connect( engine, SIGNAL(albumMetadataChanged(Meta::AlbumPtr)),
             this, SLOT(metadataChanged(Meta::AlbumPtr)) );
}

CurrentEngine::~CurrentEngine()
{
}

QStringList
CurrentEngine::sources() const
{
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool
CurrentEngine::sourceRequestEvent( const QString& name )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    m_requested[ name ] = true;
    if( !track )
        stopped();

    if( name == QLatin1String("current") )
        update( track );
    else if( name == QLatin1String("albums") )
        track ? update(track->album()) : setData(name, Plasma::DataEngine::Data());
    else
        return false;

    return true;
}

void
CurrentEngine::metadataChanged( Meta::AlbumPtr album )
{
    // disregard changes for other albums (BR: 306735)
    if( !m_currentTrack || m_currentTrack->album() != album )
        return;

    QImage cover = album->image( m_coverWidth );
    qint64 coverCacheKey = cover.cacheKey();
    if( m_coverCacheKey != coverCacheKey )
    {
        m_coverCacheKey = coverCacheKey;
        setData( "current", "albumart", cover );
    }
}

void
CurrentEngine::metadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    if( m_trackInfo != trackInfo )
    {
        m_trackInfo = trackInfo;
        setData( "current", "current", trackInfo );
        if( track && m_requested.value( QLatin1String("albums") ) )
            update( track->album() );
    }
}

void
CurrentEngine::trackPlaying( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    m_lastQueryMaker = 0;
    if( m_requested.value( QLatin1String("current") ) )
        update( track );
    if( track && m_requested.value( QLatin1String("albums") ) )
        update( track->album() );
}

void
CurrentEngine::stopped()
{
    if( m_requested.value( QLatin1String("current") ) )
    {
        removeAllData( "current" );
        setData( "current", "notrack", i18n( "No track playing") );
        m_currentTrack.clear();
    }

    if( m_requested.value( QLatin1String("albums") ) )
    {
        removeAllData( "albums" );
        m_albumData.clear();

        // Collect data for the recently added albums
        setData( "albums", "headerText", QVariant( i18n( "Recently Added Albums" ) ) );
        m_albums.clear();

        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        qm->setAutoDelete( true );
        qm->setQueryType( Collections::QueryMaker::Album );
        qm->excludeFilter( Meta::valAlbum, QString(), true, true );
        qm->orderBy( Meta::valCreateDate, true );
        qm->limitMaxResultSize( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) );

        connect( qm, SIGNAL(newResultReady(Meta::AlbumList)),
                 SLOT(resultReady(Meta::AlbumList)), Qt::QueuedConnection );
        connect( qm, SIGNAL(queryDone()), SLOT(setupAlbumsData()) );

        m_lastQueryMaker = qm;
        qm->run();
    }
}

void
CurrentEngine::update( Meta::TrackPtr track )
{
    if( !m_requested.value( QLatin1String("current") ) ||
        track == m_currentTrack )
        return;

    m_currentTrack = track;
    removeAllData( QLatin1String("current") );

    if( !track )
        return;

    Plasma::DataEngine::Data data;
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    data["current"] = trackInfo;
    Meta::AlbumPtr album = track->album();
    data["albumart"] = QVariant( album ? The::coverCache()->getCover( album, m_coverWidth) : QPixmap() );

    Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        const QString source = sic->sourceName();
        debug() <<" We have source " <<source;
        if( !source.isEmpty() )
            data["source_emblem"] = sic->scalableEmblem();

        delete sic;
    }
    else
        data["source_emblem"] = QVariant( QPixmap() );

    debug() << "updating track" << track->name();
    setData( "current", data );
}

void
CurrentEngine::update( Meta::AlbumPtr album )
{
    if( !m_requested.value( QLatin1String("albums") ) )
        return;

    m_lastQueryMaker = 0;
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( !album )
        return;

    Meta::ArtistPtr artist = track->artist();

    // Prefer track artist to album artist BUG: 266682
    if( !artist )
        artist = album->albumArtist();
    
    if( artist && !artist->name().isEmpty() )
    {
        m_albums.clear();
        m_albumData.clear();
        m_albumData[ QLatin1String("currentTrack") ] = qVariantFromValue( track );
        m_albumData[ QLatin1String("headerText") ] = QVariant( i18n( "Albums by %1", artist->name() ) );

        // -- search the collection for albums with the same artist
        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        qm->setAutoDelete( true );
        qm->addFilter( Meta::valArtist, artist->name(), true, true );
        qm->setAlbumQueryMode( Collections::QueryMaker::AllAlbums );
        qm->setQueryType( Collections::QueryMaker::Album );

        connect( qm, SIGNAL(newResultReady(Meta::AlbumList)),
                 SLOT(resultReady(Meta::AlbumList)), Qt::QueuedConnection );
        connect( qm, SIGNAL(queryDone()), SLOT(setupAlbumsData()) );

        m_lastQueryMaker = qm;
        qm->run();
    }
    else
    {
        removeAllData( QLatin1String("albums") );
        setData( QLatin1String("albums"), QLatin1String("headerText"),
                 i18nc( "Header text for current album applet", "Albums" ) );
    }
}

void
CurrentEngine::setupAlbumsData()
{
    if( sender() == m_lastQueryMaker )
    {
        m_albumData[ QLatin1String("albums") ] = QVariant::fromValue( m_albums );
        setData( QLatin1String("albums"), m_albumData );
    }
}

void
CurrentEngine::resultReady( const Meta::AlbumList &albums )
{
    if( sender() == m_lastQueryMaker )
        m_albums << albums;
}

