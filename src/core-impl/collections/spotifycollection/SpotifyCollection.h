/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#ifndef SPOTIFY_COLLECTION_H
#define SPOTIFY_COLLECTION_H

#include "core/collections/Collection.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "SpotifyMeta.h"
#include "support/Controller.h"
#include "core/support/Debug.h"
#include <QObject>
#include <QString>

namespace Collections
{
    class QueryMaker;

    class SpotifyCollectionFactory: public CollectionFactory
    {
        Q_OBJECT
        public:
            SpotifyCollectionFactory( QObject* parent, const QVariantList& args );
            virtual ~SpotifyCollectionFactory();

            virtual void init();

        private Q_SLOTS:
            void checkStatus();
            void spotifyReady();
            void slotSpotifyError( const Spotify::Controller::ErrorState );
            void collectionRemoved();

        private:

            Spotify::Controller* m_controller;
            QWeakPointer< SpotifyCollection > m_collection;
            bool m_collectionIsManaged;
    };

    class SpotifyCollection: public Collection
    {
        Q_OBJECT
        public:
            SpotifyCollection( Spotify::Controller* controller );
            ~SpotifyCollection();

            QueryMaker* queryMaker();
            Playlists::UserPlaylistProvider* userPlaylistProvider();

            QString uidUrlProtocol() const;
            QString collectionId() const;

            QString prettyName() const;
            KIcon icon() const;

            bool isWritable() const;
            bool isOrganizable() const;

            //Methods from Collections::TrackProvider
            bool possiblyContainsTrack( const KUrl &url ) const;
            Meta::TrackPtr trackForUrl( const KUrl &url );

            //Methods from Collections::CollectionBase
            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            //SpotifyCollection-specific
            void addNewTrack( Meta::SpotifyTrackPtr track );
            QSharedPointer< MemoryCollection > memoryCollection();

            Spotify::Controller* controller() { return m_controller; }

        private Q_SLOTS:
            void slotSpotifyError( const Spotify::Controller::ErrorState );

        private:
            QString m_collectionId;

            QSharedPointer< MemoryCollection > m_memoryCollection;
            Spotify::Controller* m_controller;
    };
} // namespace Collections


#endif
