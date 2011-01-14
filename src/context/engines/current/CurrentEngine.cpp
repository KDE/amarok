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

#include "core/support/Amarok.h"
#include "ContextView.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "covermanager/CoverCache.h"
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
{
    DEBUG_BLOCK
    Q_UNUSED( args )

    m_sources << QLatin1String("current") << QLatin1String("albums");
    m_requested[ QLatin1String("current") ] = false;
    m_requested[ QLatin1String("albums")  ] = false;
    EngineController* engine = The::engineController();

    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
             this, SLOT( trackPlaying( Meta::TrackPtr ) ) );
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
        track ? update(track->artist()) : setData(name, Plasma::DataEngine::Data());
    else
        return false;

    return true;
}

void
CurrentEngine::metadataChanged( Meta::AlbumPtr album )
{
    setData( "current", "albumart", album->image( m_coverWidth ) );
}

void
CurrentEngine::metadataChanged( Meta::TrackPtr track )
{
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    setData( "current", "current", trackInfo );
    if( track && m_requested.value( QLatin1String("albums") ) )
        update( track->artist() );
}

void
CurrentEngine::trackPlaying( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if( m_recentAlbumsQm )
        m_recentAlbumsQm.data()->abortQuery();
    if( m_requested.value( QLatin1String("current") ) )
        update( track );
    if( track && m_requested.value( QLatin1String("albums") ) )
        update( track->artist() );
}

void
CurrentEngine::stopped()
{
    if( m_requested.value( QLatin1String("current") ) )
    {
        removeAllData( "current" );
        setData( "current", "notrack", i18n( "No track playing") );
    }

    if( m_requested.value( QLatin1String("albums") ) )
    {
        removeAllData( "albums" );

        // Collect data for the recently added albums
        setData( "albums", "headerText", QVariant( i18n( "Recently added albums" ) ) );
        m_albums.clear();

        m_recentAlbumsQm = CollectionManager::instance()->queryMaker();
        Collections::QueryMaker *qm = m_recentAlbumsQm.data();
        if( qm )
        {
            qm->setAutoDelete( true );
            qm->setQueryType( Collections::QueryMaker::Album );
            qm->excludeFilter( Meta::valAlbum, QString(), true, true );
            qm->orderBy( Meta::valCreateDate, true );
            qm->limitMaxResultSize( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) );

            connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                    SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
            connect( qm, SIGNAL( queryDone() ), SLOT( setupAlbumsData() ) );

            qm->run();
        }
    }
}

void
CurrentEngine::update( Meta::TrackPtr track )
{
    if( !track )
        return;
    if( !m_requested.value( QLatin1String("current") ) )
        return;

    removeAllData( "current" );
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
CurrentEngine::update( Meta::ArtistPtr artist )
{
    if( !m_requested.value( QLatin1String("albums") ) )
        return;

    if( !artist )
        return;

    Meta::AlbumList albums = artist->albums();
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( (albums == m_albums) && (track == m_currentTrack) )
    {
        debug() << "albums list unchanged, not updating";
        return;
    }

    m_albums.clear();
    removeAllData( QLatin1String("albums") );
    setData( "albums", "headerText", QVariant( i18n( "Albums by %1", artist->name() ) ) );
    setData( "albums", "currentTrack", qVariantFromValue(track) );
    m_currentTrack = track;

    if( albums.isEmpty() )
    {
        //try searching the collection as we might be dealing with a non local track
        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        qm->setAutoDelete( true );
        qm->setQueryType( Collections::QueryMaker::Album );
        qm->addMatch( artist );

        connect( qm, SIGNAL(newResultReady(QString, Meta::AlbumList)),
                 SLOT(resultReady(QString, Meta::AlbumList)), Qt::QueuedConnection );
        connect( qm, SIGNAL(queryDone()), SLOT(setupAlbumsData()) );
        qm->run();
    }
    else
    {
        m_albums << albums;
        setupAlbumsData();
    }
}

void
CurrentEngine::setupAlbumsData()
{
    debug() << "setting up" << m_albums.count() << "albums";
    setData( "albums", "albums", QVariant::fromValue( m_albums ) );
}

void
CurrentEngine::resultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    // DEBUG_BLOCK
    Q_UNUSED( collectionId )

    m_albums.clear();
    m_albums << albums;
}

#include "CurrentEngine.moc"
