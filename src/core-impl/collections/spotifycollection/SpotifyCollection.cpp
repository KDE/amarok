/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng<odayfans@gmail.com>                                     *
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

#define DEBUG_PREFIX "SpotifyCollection"

#include "SpotifyCollection.h"

#include "SpotifyQueryMaker.h"
#include "SpotifySettingsDialog.h"
#include "support/Controller.h"
#include "support/TrackProxy.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/collections/Collection.h"
#include "core/support/Debug.h"

#include <KIcon>
#include <KStandardDirs>

#include <QSysInfo>
#include <QTimer>

namespace Collections
{

    AMAROK_EXPORT_COLLECTION( SpotifyCollectionFactory, spotifycollection )

    SpotifyCollectionFactory::SpotifyCollectionFactory( QObject* parent, const QVariantList &args )
        : CollectionFactory( parent, args )
        , m_controller( 0 )
        , m_collectionIsManaged( false )
    {
        m_info = KPluginInfo( "amarok_collection-spotifycollection.desktop", "services" );
        m_info.setConfig( Amarok::config( SpotifyConfig::configSectionName() ) );
        DEBUG_BLOCK
    }

    SpotifyCollectionFactory::~SpotifyCollectionFactory()
    {
        DEBUG_BLOCK
        delete m_collection.data();

        if( m_controller )
            m_controller->deleteLater();
    }

    void
    SpotifyCollectionFactory::init()
    {
        DEBUG_BLOCK

        // Load credentials
        m_config.load();

        m_controller = The::SpotifyController( SpotifyCollection::resolverPath() );
#ifdef Q_OS_LINUX
        m_controller->environment().insert("LD_LIBRARY_PATH",
                                           SpotifyCollection::resolverDownloadPath());
#endif

        Q_ASSERT( m_controller != 0 );

        connect( m_controller, SIGNAL( spotifyReady() ),
                 this, SLOT( slotSpotifyReady() ) );

        connect( m_controller, SIGNAL( spotifyError( Spotify::Controller::ErrorState ) ),
                 this, SLOT( slotSpotifyError( Spotify::Controller::ErrorState ) ) );

        // Initialize SpotifyCollection
        m_collection = new SpotifyCollection( m_controller );
        connect( m_collection.data(), SIGNAL(remove()), this, SLOT(collectionRemoved()) );
        m_collectionIsManaged = true;

        // Register collection
        emit newCollection( m_collection.data() );

        // Start the controller and try to load Spotify resolver
        m_controller->start();
        slotCheckStatus();

        m_initialized = true;
    }

    void
    SpotifyCollectionFactory::slotCheckStatus()
    {
        //TODO: Add controller checkStatus function
    }


    void
    SpotifyCollectionFactory::slotSpotifyReady()
    {
        DEBUG_BLOCK

        m_controller->setFilePath( SpotifyCollection::resolverPath() );
        if( !m_controller->loggedIn()
            && !m_config.username().isEmpty()
            && !m_config.password().isEmpty() )
        {
            m_controller->login( m_config.username(), m_config.password(), m_config.highQuality() );
        }

    }

    void
    SpotifyCollectionFactory::slotSpotifyError( const Spotify::Controller::ErrorState error )
    {
        // DEBUG_BLOCK

        if( error == Spotify::Controller::ResolverNotFound )
        {
            if( m_collection && !m_collectionIsManaged )
                CollectionManager::instance()->removeTrackProvider( m_collection.data() );
        }
    }

    void
    SpotifyCollectionFactory::collectionRemoved()
    {
        DEBUG_BLOCK

        m_collectionIsManaged = false;
    }

    const QString
    SpotifyCollection::supportedPlatformName()
    {
    #ifdef Q_OS_WIN32
        return "win32";
    #else
    #ifdef Q_OS_LINUX
        return QString("linux%1").arg(QSysInfo::WordSize);
    #else
        return QString();
    #endif
    #endif
    }

    //TODO: replace with link on files.kde.org or redirect from amarok.kde.org
    const QString SpotifyCollection::s_resolverDownloadUrl =
            "http://hades.name/static/amarok/";

    const QString
    SpotifyCollection::defaultResolverName()
    {
        return QString("spotify_resolver_%1").arg(supportedPlatformName());
    }

    const QString
    SpotifyCollection::resolverDownloadPath()
    {
        return KStandardDirs::locateLocal( "data", "amarok" );
    }

    const QString SpotifyCollection::resolverDownloadUrl()
    {
        return s_resolverDownloadUrl + defaultResolverName() + ".zip";
    }

    const QString SpotifyCollection::resolverPath()
    {
        return resolverDownloadPath() + "/" + defaultResolverName();
    }

    SpotifyCollection::SpotifyCollection( Spotify::Controller *controller )
        : m_collectionId( i18n( "Spotify Collection" ) )
        , m_memoryCollection( new MemoryCollection )
        , m_controller( controller )
        , m_configureAction( 0 )
    {
        DEBUG_BLOCK

        // Create configuration action
        m_configureAction = new QAction( KIcon( "configure" ), i18n( "&Configure Spotify collection" ), this );
        m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( m_configureAction, SIGNAL( triggered() ), SLOT( slotConfigure() ) );
        connect( m_controller, SIGNAL( userChanged() ), this, SLOT( slotUserChanged() ) );
    }

    SpotifyCollection::~SpotifyCollection()
    {
        DEBUG_BLOCK
    }

    Spotify::Controller*
    SpotifyCollection::controller()
    {
        return m_controller;
    }

    QueryMaker*
    SpotifyCollection::queryMaker()
    {
        DEBUG_BLOCK

        Collections::SpotifyQueryMaker* qm = new Collections::SpotifyQueryMaker( this );
        connect( qm, SIGNAL( spotifyError(Spotify::Controller::ErrorState)),
                 this, SLOT(slotSpotifyError(Spotify::Controller::ErrorState)));
        return qm;
    }

    Playlists::UserPlaylistProvider*
    SpotifyCollection::userPlaylistProvider()
    {
        DEBUG_BLOCK

        return 0;
    }

    QString
    SpotifyCollection::uidUrlProtocol() const
    {
        return QString( "spotify" );
    }

    QString
    SpotifyCollection::collectionId() const
    {
        return m_collectionId;
    }

    QString
    SpotifyCollection::prettyName() const
    {
        return collectionId();
    }

    KIcon
    SpotifyCollection::icon() const
    {
        return KIcon( "network-server" );
    }

    bool
    SpotifyCollection::isWritable() const
    {

        return false;
    }

    bool
    SpotifyCollection::isOrganizable() const
    {
        DEBUG_BLOCK

        return false;
    }

    bool
    SpotifyCollection::possiblyContainsTrack( const KUrl &url ) const
    {
        DEBUG_BLOCK

        if( url.protocol() == uidUrlProtocol() &&
            url.hasQueryItem( QString( "artist" ) ) &&
            url.hasQueryItem( QString( "album" ) ) &&
            url.hasQueryItem( QString( "title" ) ) )
            return true;
        else
            return false;
    }

    Meta::TrackPtr
    SpotifyCollection::trackForUrl( const KUrl &url )
    {
        DEBUG_BLOCK

        debug() << "Get track for url: " << url.url();
        m_memoryCollection->acquireReadLock();

        if( m_memoryCollection->trackMap().contains( url.url() ) )
        {
            Meta::TrackPtr track = m_memoryCollection->trackMap().value( url.url() );
            m_memoryCollection->releaseLock();
            return track;
        }
        else
        {
            MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( url ) );
            proxyTrack->setArtist( url.queryItem( "artist" ) );
            proxyTrack->setAlbum( url.queryItem( "album" ) );
            proxyTrack->setTitle( url.queryItem( "title" ) );
            Spotify::TrackProxy *proxy = new Spotify::TrackProxy( url, proxyTrack, this );

            connect( proxy, SIGNAL( spotifyError( const Spotify::Controller::ErrorState ) ),
                     this, SLOT( slotSpotifyError( const Spotify::Controller::ErrorState ) ) );

            Meta::TrackPtr result = Meta::TrackPtr::staticCast( proxyTrack );

            m_memoryCollection->addTrack( result );
            m_memoryCollection->releaseLock();

            return result;
        }
    }

    bool
    SpotifyCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        switch( type )
        {
            case Capabilities::Capability::Actions :
                return true;
            default:
                return false;
        }
    }

    Capabilities::Capability*
    SpotifyCollection::createCapabilityInterface( Capabilities::Capability::Type type )
    {
        switch( type )
        {
            case Capabilities::Capability::Actions :
            {
                QList< QAction * > actions;
                actions << m_configureAction;
                return new Capabilities::ActionsCapability( actions );
            }
            default:
                return 0;
        }
    }

    void
    SpotifyCollection::addNewTrack( Meta::SpotifyTrackPtr track )
    {
        DEBUG_BLOCK

        m_memoryCollection->acquireReadLock();

        if( !m_memoryCollection->trackMap().contains( track->uidUrl() ) )
        {
            m_memoryCollection->releaseLock();
            m_memoryCollection->acquireWriteLock();

            Meta::SpotifyArtistPtr artistPtr;
            if( m_memoryCollection->artistMap().contains( track->artist()->name() ) )
            {
                Meta::ArtistPtr artist = m_memoryCollection->artistMap().value( track->artist()->name() );
                artistPtr = Meta::SpotifyArtistPtr::staticCast( artist );
            }
            else
            {
                artistPtr = track->spotifyArtist();
                Meta::ArtistPtr artist = Meta::ArtistPtr::staticCast( artistPtr );
                m_memoryCollection->addArtist( artist );
            }
            artistPtr->addTrack( track );
            track->setArtist( artistPtr );

            Meta::SpotifyAlbumPtr albumPtr;
            if( m_memoryCollection->albumMap().contains( track->album()->name(),
                                                         artistPtr->name() ) )
            {
                Meta::AlbumPtr album = m_memoryCollection->albumMap().value( track->album()->name(),
                                                                             artistPtr->name() );
                albumPtr = Meta::SpotifyAlbumPtr::staticCast( album );
            }
            else
            {
                albumPtr = track->spotifyAlbum();
                albumPtr->setAlbumArtist( artistPtr );
                artistPtr->addAlbum( albumPtr );
                Meta::AlbumPtr album = Meta::AlbumPtr::staticCast( albumPtr );
                m_memoryCollection->addAlbum( album );
            }
            albumPtr->addTrack( track );
            track->setAlbum( albumPtr );

            Meta::SpotifyGenrePtr genrePtr;
            if( m_memoryCollection->genreMap().contains( track->genre()->name() ) )
            {
                Meta::GenrePtr genre = m_memoryCollection->genreMap().value( track->genre()->name() );
                genrePtr = Meta::SpotifyGenrePtr::staticCast( genre );
            }
            else
            {
                genrePtr = track->spotifyGenre();
                Meta::GenrePtr genre = Meta::GenrePtr::staticCast( genrePtr );
                m_memoryCollection->addGenre( genre );
            }
            genrePtr->addTrack( track );
            track->setGenre( genrePtr );

            Meta::SpotifyYearPtr yearPtr;
            if( m_memoryCollection->yearMap().contains( track->year()->year() ) )
            {
                Meta::YearPtr year = m_memoryCollection->yearMap().value( track->year()->year() );
                yearPtr = Meta::SpotifyYearPtr::staticCast( year );
            }
            else
            {
                yearPtr = track->spotifyYear();
                Meta::YearPtr year = Meta::YearPtr::staticCast( yearPtr );
                m_memoryCollection->addYear( year );
            }
            yearPtr->addTrack( track );
            track->setYear( yearPtr );

            m_memoryCollection->addTrack( Meta::TrackPtr::staticCast( track ) );

            m_memoryCollection->releaseLock();
            emit updated();
        }
        else
            m_memoryCollection->releaseLock();
    }

    QSharedPointer< MemoryCollection >
    SpotifyCollection::memoryCollection()
    {
        return m_memoryCollection;
    }

    void
    SpotifyCollection::slotSpotifyError( const Spotify::Controller::ErrorState error )
    {
        if( error == Spotify::Controller::ErrorState( 1 ) )
            emit remove();
    }

    void
    SpotifyCollection::slotConfigure()
    {
        SpotifySettingsDialog settingDialog;
        settingDialog.setModal( true );
        // This will return immediately
        settingDialog.exec();
    }

    void
    SpotifyCollection::slotUserChanged()
    {
        m_memoryCollection->acquireWriteLock();
        // Clear collection when user changed
        m_memoryCollection->setAlbumMap( AlbumMap() );
        m_memoryCollection->setArtistMap( ArtistMap() );
        m_memoryCollection->setGenreMap( GenreMap() );
        m_memoryCollection->setTrackMap( TrackMap() );
        m_memoryCollection->setYearMap( YearMap() );
        m_memoryCollection->releaseLock();
        emit updated();
    }
} // namespace Collections

#include "SpotifyCollection.moc"
