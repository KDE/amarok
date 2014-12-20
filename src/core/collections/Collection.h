/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef AMAROK_COLLECTION_H
#define AMAROK_COLLECTION_H

#include "core/amarokcore_export.h"
#include "core/interfaces/MetaCapability.h"
#include "core/support/PluginFactory.h"
#include "core/storage/SqlStorage.h"

#include <QObject>

namespace Meta {
    class Track;
    typedef KSharedPtr<Track> TrackPtr;
}
namespace Playlists {
    class UserPlaylistProvider;
}

class KIcon;

namespace Collections
{
    class Collection;
    class CollectionLocation;
    class QueryMaker;
    typedef QList<Collection*> CollectionList;

    /** A plugin that creates new collections.
     */
    class AMAROK_CORE_EXPORT CollectionFactory : public Plugins::PluginFactory
    {
        Q_OBJECT

        public:
            CollectionFactory( QObject *parent, const QVariantList &args );
            virtual ~CollectionFactory();

            virtual void init() = 0;

        signals:
            void newCollection( Collections::Collection *newCollection );

            /** Emitted whenever the factory produces a new storage.
             *
             *  This should usually happen only once before the creation
             *  of the first primary (local) collection.
             *
             *  This function will disapper as soon as we have dedictated
             *  StorageFactories.
             */
            void newStorage( SqlStorage *newStorage );

    };

    /** A TrackProvider is a class that can lookup urls and return Track objects.
     *
     *  A track provider is implemented by every collection, but there
     *  are also a couple of other track providers.
     *  All TrackProvider are managed by the CollectionManager.
     */
    class AMAROK_CORE_EXPORT TrackProvider
    {
        public:
            TrackProvider();
            virtual ~TrackProvider();

            /**
             * Returns true if this track provider has a chance of providing the
             * track specified by @p url.
             * This should do a minimal amount of checking, and return quickly.
             */
            virtual bool possiblyContainsTrack( const KUrl &url ) const;

            /**
             * Creates a TrackPtr object for url @p url.  Returns a null track Ptr if
             * it cannot be done.
             * If asynchronysity is desired it is suggested to return a MetaProxy track here
             * and have the proxy watch for the real track.
             */
            virtual Meta::TrackPtr trackForUrl( const KUrl &url );
    };

    class AMAROK_CORE_EXPORT Collection : public QObject, public TrackProvider, public MetaCapability
    {
        Q_OBJECT

        public:
            virtual ~Collection();

            /**
             * The collection's querymaker
             * @return A querymaker that belongs to this collection.
             */
            virtual QueryMaker *queryMaker() = 0;

            /**
             * The protocol of uids coming from this collection.
             * @return A string of the protocol, without the ://
             */
            virtual QString uidUrlProtocol() const;

            /**
             * @return A unique identifier for this collection
             */
            virtual QString collectionId() const = 0;

            /**
             * @return a user visible name for this collection, to be displayed in the collectionbrowser and elsewhere
             */
            virtual QString prettyName() const = 0;

            /**
             * @return an icon representing this collection
             */
            virtual KIcon icon() const = 0;

            virtual bool hasCapacity() const { return false; }
            virtual float usedCapacity() const { return 0.0; }
            virtual float totalCapacity() const { return 0.0; }

            /**
             * Create collection location that can be used to copy track to this
             * collection or to delete collection tracks. If you don't call
             * prepare{Move,Copy,Remove} on it, you must delete it after use.
             */
            virtual Collections::CollectionLocation *location();

            /**
             * Return true if this collection can be written to (tracks added, removed).
             * Convenience short-cut for calling CollectionLocation's isWritable.
             */
            virtual bool isWritable() const;

            /**
             * Return true if user can choose track file path within this collection.
             * Convenience short-cut for calling CollectionLocation's isOrganizable.
             */
            virtual bool isOrganizable() const;

        signals:
            /**
             * Once you register a collection with CollectionManager, this signal is the
             * only way to safely destroy it. CollectionManger will remove this collection
             * from the list of active ones and will destroy this collection after some
             * time.
             */
            void remove();

            /**
             * This signal must be emitted when the collection contents has changed
             * significantly.
             *
             * More specifically, you must emit this signal (only) in such situations:
             *  a) the set of entities (tracks, albums, years, ...) in this collection has
             *     changed: a track was added, album is renamed, year was removed...
             *  b) the relationship between the entities has changed: the track changed
             *     album, album is no longer associated to an album artist and bacame a
             *     compilation, an alum changed its year...
             *
             * You should not emit this signal when some minor data of an entity change,
             * for example when a track comment changes, etc.
             *
             * Also note there are ::notifyObservers() methods of various entities.
             * ::notifyObservers() and Collection::updated() are perpendicular and
             * responsibility to call one of these may and may not mean need to call the
             * other.
             *
             * This signal spedifically this means that previous done searches can no
             * longer be considered valid.
             */
            void updated();
    };
}

Q_DECLARE_METATYPE( Collections::Collection* )
Q_DECLARE_METATYPE( Collections::CollectionList )

#define AMAROK_EXPORT_COLLECTION( classname, libname ) \
    K_PLUGIN_FACTORY( factory, registerPlugin<classname>(); ) \
            K_EXPORT_PLUGIN( factory( "amarok_collection-" #libname ) )

#endif /* AMAROK_COLLECTION_H */
