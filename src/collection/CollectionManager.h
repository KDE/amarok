/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef AMAROK_COLLECTIONMANAGER_H
#define AMAROK_COLLECTIONMANAGER_H

#include "amarok_export.h"
#include "Collection.h"
#include "meta/Meta.h"
#include "QueryMaker.h"

#include <QList>
#include <QObject>

class SqlStorage;
class CollectionManagerSingleton;
class TimecodeTrackProvider;

class AMAROK_EXPORT CollectionManager : public QObject
{
    Q_OBJECT
    Q_ENUMS( CollectionStatus )

    public:

        /**
         * defines the status of a collection in respect to global queries (i.e. queries that query all known collections)
         * or the collection browser.
         */
        enum CollectionStatus { CollectionDisabled = 1, //Collection neither viewable nor queryable
                                CollectionViewable = 2, //Collection will not be queried by CollectionManager::queryMaker
                                CollectionQueryable= 4, //Collection wil not show up in the browser, but is queryable by global queries
                                CollectionEnabled = CollectionViewable | CollectionQueryable //Collection viewable in the browser and queryable
        };

        static CollectionManager * instance();
        static void destroy();

        QueryMaker * queryMaker() const;

        /**
         * returns all viewable collections.
         */
        QList<Amarok::Collection*> viewableCollections() const;

        /**
         * returns all queryable collections.
         */
        QList<Amarok::Collection*> queryableCollections() const;

        Amarok::Collection* primaryCollection() const;

        /**
            This method will try to get a Track object for the given url. This method will return 0 if no Track object
            could be created for the url.
        */
        Meta::TrackPtr trackForUrl( const KUrl &url );
        Meta::TrackList tracksForUrls( const KUrl::List &urls );

        /**
         * When using this method, you must watch for the foundRelatedArtists signal
         * for the returned Meta::ArtistList
         */
        void relatedArtists( Meta::ArtistPtr artist, int maxArtists );

        /**
            retrieve an interface which allows client-code to store/load data in a relational database.
            Note: code using this method does NOT take ownership of the pointer, but may cache the pointer
        */
        SqlStorage* sqlStorage() const;

        /**
         * add a collection whose lifecycle is not managed by CollectionManager.
         * CollectionManger uses the default status passed as the second argument unless a custom
         * status is stored in Amarok's config file.
         */
        void addUnmanagedCollection( Amarok::Collection *newCollection, CollectionStatus defaultStatus );
        void removeUnmanagedCollection( Amarok::Collection *collection );

        void setCollectionStatus( const QString &collectionId, CollectionStatus status );
        CollectionStatus collectionStatus( const QString &collectionId ) const;
        QHash<Amarok::Collection*, CollectionStatus> collections() const;

        /**
         * adds a TrackProvider to the list of TrackProviders,
         * which allows CollectionManager to create tracks in trackForUrl.
         * CollectionManager does not take ownership of the TrackProvider pointer
         *
         * Note: collections that CollectionManager knows about are automatically
         * added to the list of TrackProviders.
         *
         * @param provider the new TrackProvider
         */
        void addTrackProvider( Amarok::TrackProvider *provider );

        /**
         * removes a TrackProvider. Does not do anything if
         * CollectionManager does not know the given TrackProvider.
         *
         * Note: collections will be automatically removed from
         * the list of available TrackProviders.
         *
         * @param provider the provider to be removed
         */
        void removeTrackProvider( Amarok::TrackProvider *provider );

        bool haveEmbeddedMysql() { return m_haveEmbeddedMysql; }

    public slots:
        void startFullScan();
        void stopScan();
        void checkCollectionChanges();

    signals:
        void scanFinished();
        void collectionAdded( Amarok::Collection *newCollection );
        void collectionRemoved( QString collectionId );
        void trackProviderAdded( Amarok::TrackProvider *provider );
        //this signal will be emitted after major changes to the collection
        //e.g. new songs where added, or an album changed
        //from compilation to non-compilation (and vice versa)
        //it will not be emitted on minor changes (e.g. the tags of a song were changed)
        void collectionDataChanged( Amarok::Collection *changedCollection );

        void foundRelatedArtists( Meta::ArtistList artists );

    private slots:
        void slotNewCollection( Amarok::Collection *newCollection );
        void slotRemoveCollection();
        void slotCollectionChanged();
        void slotArtistQueryResult( QString collectionId, Meta::ArtistList artists );
        void slotContinueRelatedArtists();

    private:
        static CollectionManager* s_instance;
        CollectionManager();
        ~CollectionManager();

        // Disable copy constructor and assignment
        CollectionManager( const CollectionManager& );
        CollectionManager& operator= ( const CollectionManager& );

        void init();

        //used for related artists query
        QSet<QString>    m_artistNameSet;
        Meta::ArtistList m_resultArtistList;
        bool             m_resultEmitted;
        int              m_maxArtists;

        bool             m_haveEmbeddedMysql;
        TimecodeTrackProvider *m_timecodeTrackProvider;

        struct Private;
        Private * const d;
};

#endif /* AMAROK_COLLECTIONMANAGER_H */
