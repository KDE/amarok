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

#define DEBUG_PREFIX "PlaydarCollection"

#include "PlaydarCollection.h"

#include "core/collections/Collection.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "PlaydarQueryMaker.h"
#include "support/Controller.h"
#include "support/ProxyResolver.h"
#include "core/support/Debug.h"

#include <KIcon>

#include <QObject>
#include <QString>
#include <QTimer>

namespace Collections
{

    AMAROK_EXPORT_COLLECTION( PlaydarCollectionFactory, playdarcollection )

    PlaydarCollectionFactory::PlaydarCollectionFactory( QObject* parent, const QVariantList &args )
        : CollectionFactory( parent, args )
        , m_controller( 0 )
        , m_collectionIsManaged( false )
    {
        m_info = KPluginInfo( "amarok_collection-playdarcollection.desktop", "services" );
        DEBUG_BLOCK
    }

    PlaydarCollectionFactory::~PlaydarCollectionFactory()
    {
        DEBUG_BLOCK
        delete m_collection.data();
        delete m_controller;
    }

    void
    PlaydarCollectionFactory::init()
    {
        DEBUG_BLOCK
        m_controller = new Playdar::Controller( this );
        connect( m_controller, SIGNAL(playdarReady()),
                 this, SLOT(playdarReady()) );
        connect( m_controller, SIGNAL(playdarError(Playdar::Controller::ErrorState)),
                 this, SLOT(slotPlaydarError(Playdar::Controller::ErrorState)) );
        checkStatus();

        m_collection = new PlaydarCollection;
        connect( m_collection.data(), SIGNAL(remove()), this, SLOT(collectionRemoved()) );
        CollectionManager::instance()->addTrackProvider( m_collection.data() );

        m_initialized = true;
    }

    void
    PlaydarCollectionFactory::checkStatus()
    {
        m_controller->status();
    }


    void
    PlaydarCollectionFactory::playdarReady()
    {
        DEBUG_BLOCK

        if( !m_collection )
        {
            m_collection = new PlaydarCollection();
            connect( m_collection.data(), SIGNAL(remove()), this, SLOT(collectionRemoved()) );
        }

        if( !m_collectionIsManaged )
        {
            m_collectionIsManaged = true;
            emit newCollection( m_collection.data() );
        }
    }

    void
    PlaydarCollectionFactory::slotPlaydarError( Playdar::Controller::ErrorState error )
    {
        // DEBUG_BLOCK

        if( error == Playdar::Controller::ErrorState( 1 ) )
        {
            if( m_collection && !m_collectionIsManaged )
                CollectionManager::instance()->removeTrackProvider( m_collection.data() );

            QTimer::singleShot( 10 * 60 * 1000, this, SLOT(checkStatus()) );
        }
    }

    void
    PlaydarCollectionFactory::collectionRemoved()
    {
        DEBUG_BLOCK

        m_collectionIsManaged = false;
        QTimer::singleShot( 10000, this, SLOT(checkStatus()) );
    }

    PlaydarCollection::PlaydarCollection()
        : m_collectionId( i18n( "Playdar Collection" ) )
        , m_memoryCollection( new MemoryCollection )
    {
        DEBUG_BLOCK

    }

    PlaydarCollection::~PlaydarCollection()
    {
        DEBUG_BLOCK

    }

    QueryMaker*
    PlaydarCollection::queryMaker()
    {
        DEBUG_BLOCK

        PlaydarQueryMaker *freshQueryMaker = new PlaydarQueryMaker( this );
        connect( freshQueryMaker, SIGNAL(playdarError(Playdar::Controller::ErrorState)),
                 this, SLOT(slotPlaydarError(Playdar::Controller::ErrorState)) );
        return freshQueryMaker;
    }

    Playlists::UserPlaylistProvider*
    PlaydarCollection::userPlaylistProvider()
    {
        DEBUG_BLOCK

        return 0;
    }

    QString
    PlaydarCollection::uidUrlProtocol() const
    {
        return QString( "playdar" );
    }

    QString
    PlaydarCollection::collectionId() const
    {
        return m_collectionId;
    }

    QString
    PlaydarCollection::prettyName() const
    {
        return collectionId();
    }

    KIcon
    PlaydarCollection::icon() const
    {
        return KIcon( "network-server" );
    }

    bool
    PlaydarCollection::isWritable() const
    {
        DEBUG_BLOCK

        return false;
    }

    bool
    PlaydarCollection::isOrganizable() const
    {
        DEBUG_BLOCK

        return false;
    }

    bool
    PlaydarCollection::possiblyContainsTrack( const KUrl &url ) const
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
    PlaydarCollection::trackForUrl( const KUrl &url )
    {
        DEBUG_BLOCK

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
            proxyTrack->setTitle( url.queryItem( "title" ) );
            Playdar::ProxyResolver *proxyResolver = new Playdar::ProxyResolver( this, url, proxyTrack );

            connect( proxyResolver, SIGNAL(playdarError(Playdar::Controller::ErrorState)),
                     this, SLOT(slotPlaydarError(Playdar::Controller::ErrorState)) );

            return Meta::TrackPtr::staticCast( proxyTrack );
        }
    }

    bool
    PlaydarCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        //TODO: Make this work once capabilities are set.
        Q_UNUSED( type );
        return false;
    }

    Capabilities::Capability*
    PlaydarCollection::createCapabilityInterface( Capabilities::Capability::Type type )
    {
        //TODO: Make this work once capabilities are set.
        Q_UNUSED( type );
        return 0;
    }

    void
    PlaydarCollection::addNewTrack( Meta::PlaydarTrackPtr track )
    {
        DEBUG_BLOCK

        m_memoryCollection->acquireReadLock();

        if( !m_memoryCollection->trackMap().contains( track->uidUrl() ) )
        {
            m_memoryCollection->releaseLock();
            m_memoryCollection->acquireWriteLock();

            Meta::PlaydarArtistPtr artistPtr;
            if( m_memoryCollection->artistMap().contains( track->artist()->name() ) )
            {
                Meta::ArtistPtr artist = m_memoryCollection->artistMap().value( track->artist()->name() );
                artistPtr = Meta::PlaydarArtistPtr::staticCast( artist );
            }
            else
            {
                artistPtr = track->playdarArtist();
                Meta::ArtistPtr artist = Meta::ArtistPtr::staticCast( artistPtr );
                m_memoryCollection->addArtist( artist );
            }
            artistPtr->addTrack( track );
            track->setArtist( artistPtr );

            Meta::PlaydarAlbumPtr albumPtr;
            if( m_memoryCollection->albumMap().contains( track->album()->name(),
                                                         artistPtr->name() ) )
            {
                Meta::AlbumPtr album = m_memoryCollection->albumMap().value( track->album()->name(),
                                                                             artistPtr->name() );
                albumPtr = Meta::PlaydarAlbumPtr::staticCast( album );
            }
            else
            {
                albumPtr = track->playdarAlbum();
                albumPtr->setAlbumArtist( artistPtr );
                artistPtr->addAlbum( albumPtr );
                Meta::AlbumPtr album = Meta::AlbumPtr::staticCast( albumPtr );
                m_memoryCollection->addAlbum( album );
            }
            albumPtr->addTrack( track );
            track->setAlbum( albumPtr );

            Meta::PlaydarGenrePtr genrePtr;
            if( m_memoryCollection->genreMap().contains( track->genre()->name() ) )
            {
                Meta::GenrePtr genre = m_memoryCollection->genreMap().value( track->genre()->name() );
                genrePtr = Meta::PlaydarGenrePtr::staticCast( genre );
            }
            else
            {
                genrePtr = track->playdarGenre();
                Meta::GenrePtr genre = Meta::GenrePtr::staticCast( genrePtr );
                m_memoryCollection->addGenre( genre );
            }
            genrePtr->addTrack( track );
            track->setGenre( genrePtr );

            Meta::PlaydarComposerPtr composerPtr;
            if( m_memoryCollection->composerMap().contains( track->composer()->name() ) )
            {
                Meta::ComposerPtr composer = m_memoryCollection->composerMap().value( track->composer()->name() );
                composerPtr = Meta::PlaydarComposerPtr::staticCast( composer );
            }
            else
            {
                composerPtr = track->playdarComposer();
                Meta::ComposerPtr composer = Meta::ComposerPtr::staticCast( composerPtr );
                m_memoryCollection->addComposer( composer );
            }
            composerPtr->addTrack( track );
            track->setComposer( composerPtr );

            Meta::PlaydarYearPtr yearPtr;
            if( m_memoryCollection->yearMap().contains( track->year()->year() ) )
            {
                Meta::YearPtr year = m_memoryCollection->yearMap().value( track->year()->year() );
                yearPtr = Meta::PlaydarYearPtr::staticCast( year );
            }
            else
            {
                yearPtr = track->playdarYear();
                Meta::YearPtr year = Meta::YearPtr::staticCast( yearPtr );
                m_memoryCollection->addYear( year );
            }
            yearPtr->addTrack( track );
            track->setYear( yearPtr );

            m_memoryCollection->addTrack( Meta::TrackPtr::staticCast( track ) );

            foreach( Meta::PlaydarLabelPtr label, track->playdarLabels() )
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
    PlaydarCollection::memoryCollection() const
    {
        return m_memoryCollection;
    }

    void
    PlaydarCollection::slotPlaydarError( Playdar::Controller::ErrorState error )
    {
        if( error == Playdar::Controller::ErrorState( 1 ) )
            emit remove();
    }
}
