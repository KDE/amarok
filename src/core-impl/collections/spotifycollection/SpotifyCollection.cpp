/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#include "core/collections/Collection.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "SpotifyQueryMaker.h"
#include "support/Controller.h"
#include "support/TrackProxy.h"

#include <KIcon>

#include <QObject>
#include <QString>
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
        DEBUG_BLOCK
    }

    SpotifyCollectionFactory::~SpotifyCollectionFactory()
    {
        DEBUG_BLOCK
        delete m_collection.data();
        m_controller->deleteLater();
    }

    void
    SpotifyCollectionFactory::init()
    {
        DEBUG_BLOCK

        m_controller = new Spotify::Controller( "/home/ofan/Documents/Repos/tomahawk-resolvers/spotify/build/spotify_tomahawkresolver" );
        Q_ASSERT( m_controller != 0 );
        connect( m_controller, SIGNAL( spotifyReady() ),
                 this, SLOT( spotifyReady() ) );

        m_controller->reload();
        m_controller->start();

        connect( m_controller, SIGNAL( spotifyError( Spotify::Controller::ErrorState ) ),
                 this, SLOT( slotSpotifyError( Spotify::Controller::ErrorState ) ) );
        checkStatus();

        m_collection = new SpotifyCollection( m_controller );
        connect( m_collection.data(), SIGNAL(remove()), this, SLOT(collectionRemoved()) );
        CollectionManager::instance()->addTrackProvider( m_collection.data() );

        m_initialized = true;
    }

    void
    SpotifyCollectionFactory::checkStatus()
    {
        //TODO: Add controller checkStatus function
    }


    void
    SpotifyCollectionFactory::spotifyReady()
    {
        DEBUG_BLOCK

        if( !m_collection )
        {
            Q_ASSERT( m_controller != 0 );
            m_collection = new SpotifyCollection( m_controller );
            connect( m_collection.data(), SIGNAL(remove()), this, SLOT(collectionRemoved()) );
        }

        if( !m_collectionIsManaged )
        {
            m_collectionIsManaged = true;
            emit newCollection( m_collection.data() );
        }


        QVariantMap map;
        map["_msgtype"] = "getCredentials";
        m_controller->sendMessage(map);
    }

    void
    SpotifyCollectionFactory::slotSpotifyError( const Spotify::Controller::ErrorState error )
    {
        // DEBUG_BLOCK

        if( error == Spotify::Controller::ErrorState( 1 ) )
        {
            if( m_collection && !m_collectionIsManaged )
                CollectionManager::instance()->removeTrackProvider( m_collection.data() );

            QTimer::singleShot( 10 * 60 * 1000, this, SLOT( checkStatus() ) );
        }
    }

    void
    SpotifyCollectionFactory::collectionRemoved()
    {
        DEBUG_BLOCK

        m_collectionIsManaged = false;
        QTimer::singleShot( 10000, this, SLOT( checkStatus() ) );
    }

    SpotifyCollection::SpotifyCollection( Spotify::Controller *controller )
        : m_collectionId( i18n( "Spotify Collection" ) )
        , m_memoryCollection( new MemoryCollection )
        , m_controller( controller )
    {
        DEBUG_BLOCK

    }

    SpotifyCollection::~SpotifyCollection()
    {
        DEBUG_BLOCK
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
            m_memoryCollection->releaseLock();
            MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( url ) );
            proxyTrack->setArtist( url.queryItem( "artist" ) );
            proxyTrack->setAlbum( url.queryItem( "album" ) );
            proxyTrack->setName( url.queryItem( "title" ) );
            Spotify::TrackProxy *proxy = new Spotify::TrackProxy( url, proxyTrack, this );

            connect( proxy, SIGNAL( spotifyError( const Spotify::Controller::ErrorState ) ),
                     this, SLOT( slotSpotifyError( const Spotify::Controller::ErrorState ) ) );

            return Meta::TrackPtr::staticCast( proxyTrack );
        }
    }

    bool
    SpotifyCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        //TODO: Make this work once capabilities are set.
        Q_UNUSED( type );
        return false;
    }

    Capabilities::Capability*
    SpotifyCollection::createCapabilityInterface( Capabilities::Capability::Type type )
    {
        //TODO: Make this work once capabilities are set.
        Q_UNUSED( type );
        return 0;
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

            Meta::SpotifyComposerPtr composerPtr;
            if( m_memoryCollection->composerMap().contains( track->composer()->name() ) )
            {
                Meta::ComposerPtr composer = m_memoryCollection->composerMap().value( track->composer()->name() );
                composerPtr = Meta::SpotifyComposerPtr::staticCast( composer );
            }
            else
            {
                composerPtr = track->spotifyComposer();
                Meta::ComposerPtr composer = Meta::ComposerPtr::staticCast( composerPtr );
                m_memoryCollection->addComposer( composer );
            }
            composerPtr->addTrack( track );
            track->setComposer( composerPtr );

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

            foreach( Meta::SpotifyLabelPtr label, track->spotifyLabels() )
            {
                m_memoryCollection->addLabelToTrack( Meta::LabelPtr::staticCast( label ),
                                                     Meta::TrackPtr::staticCast( track ) );
            }

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
} // namespace Collections
