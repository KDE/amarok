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

namespace Collections
{
    
    class QueryMaker;

    class PlaydarCollectionFactory : public CollectionFactory
    {
        Q_OBJECT
        public:
            PlaydarCollectionFactory( QObject* parent, const QVariantList &args );
            virtual ~PlaydarCollectionFactory();
            
            virtual void init();
            
        private Q_SLOTS:
            void checkStatus();
            void playdarReady();
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            void collectionRemoved();

        private:
            Playdar::Controller* m_controller;
            QWeakPointer< PlaydarCollection > m_collection;
            bool m_collectionIsManaged;
    };
    
    class PlaydarCollection : public Collection
    {
        Q_OBJECT
        public:
            
            PlaydarCollection();
            ~PlaydarCollection();
            
            QueryMaker* queryMaker();
            Playlists::UserPlaylistProvider* userPlaylistProvider();
            
            QString uidUrlProtocol() const;
            QString collectionId() const;
            QString prettyName() const;
            QIcon icon() const;
            
            bool isWritable() const;
            bool isOrganizable() const;
            
            //Methods from Collections::TrackProvider
            bool possiblyContainsTrack( const QUrl &url ) const;
            Meta::TrackPtr trackForUrl( const QUrl &url );
            
            //Methods from Collections::CollectionBase
            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            //PlaydarCollection-specific
            void addNewTrack( Meta::PlaydarTrackPtr track );
            QSharedPointer< MemoryCollection > memoryCollection();

        private Q_SLOTS:
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            
        private:
            QString m_collectionId;
            
            QSharedPointer< MemoryCollection > m_memoryCollection;
            QList< QWeakPointer< Playdar::ProxyResolver > > m_proxyResolverList;
    };
}

#endif
