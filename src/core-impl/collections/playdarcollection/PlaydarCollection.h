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

#ifndef PLAYDAR_COLLECTION_H
#define PLAYDAR_COLLECTION_H

#include "core/collections/Collection.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "PlaydarQueryMaker.h"
#include "PlaydarMeta.h"
#include "support/Controller.h"
#include "support/ProxyResolver.h"

#include <QIcon>

#include <QObject>
#include <QString>
#include <QSharedPointer>

namespace Playlists
{
class UserPlaylistProvider;
}
namespace Playdar
{
class ProxyResolver;
}
namespace Collections
{
    
    class QueryMaker;

    class PlaydarCollectionFactory : public CollectionFactory
    {
        Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-playdarcollection.json")
        Q_INTERFACES(Plugins::PluginFactory)
        Q_OBJECT

        public:
            PlaydarCollectionFactory();
            ~PlaydarCollectionFactory() override;
            
            void init() override;
            
        private Q_SLOTS:
            void checkStatus();
            void playdarReady();
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            void collectionRemoved();

        private:
            Playdar::Controller* m_controller;
            QPointer< PlaydarCollection > m_collection;
            bool m_collectionIsManaged;
    };
    
    class PlaydarCollection : public Collection
    {
        Q_OBJECT
        public:
            
            PlaydarCollection();
            ~PlaydarCollection() override;
            
            QueryMaker* queryMaker() override;
            Playlists::UserPlaylistProvider* userPlaylistProvider();
            
            QString uidUrlProtocol() const override;
            QString collectionId() const override;
            QString prettyName() const override;
            QIcon icon() const override;
            
            bool isWritable() const override;
            bool isOrganizable() const override;
            
            //Methods from Collections::TrackProvider
            bool possiblyContainsTrack( const QUrl &url ) const override;
            Meta::TrackPtr trackForUrl( const QUrl &url ) override;
            
            //Methods from Collections::CollectionBase
            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

            //PlaydarCollection-specific
            void addNewTrack( Meta::PlaydarTrackPtr track );
            QSharedPointer< MemoryCollection > memoryCollection();

        private Q_SLOTS:
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            
        private:
            QString m_collectionId;
            
            QSharedPointer< MemoryCollection > m_memoryCollection;
            QList< QPointer< Playdar::ProxyResolver > > m_proxyResolverList;
    };
}

#endif
