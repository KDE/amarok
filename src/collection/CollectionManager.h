/*
 *  Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_COLLECTIONMANAGER_H
#define AMAROK_COLLECTIONMANAGER_H

#include "amarok_export.h"
#include "Collection.h"
#include "Meta.h"
#include "QueryMaker.h"

#include <QList>
#include <QObject>

class SqlStorage;
class CollectionManagerSingleton;

class AMAROK_EXPORT CollectionManager : public QObject
{
    Q_OBJECT

    public:
        static CollectionManager * instance();

        ~CollectionManager();

        QueryMaker * queryMaker() const;

        QList<Collection*> collections() const;

        Collection* primaryCollection() const;

        /**
            This method will try to get a Track object for the given url. This method will return 0 if no Track object
            could be created for the url.
        */
        Meta::TrackPtr trackForUrl( const KUrl &url );
        Meta::TrackList tracksForUrls( const KUrl::List &urls );
        Meta::ArtistList relatedArtists( Meta::ArtistPtr artist, int maxArtists );

        /**
            retrieve an interface which allows client-code to store/load data in a relational database.
            Note: code using this method does NOT take ownership of the pointer.
        */
        SqlStorage* sqlStorage() const;

        void addUnmanagedCollection( Collection *newCollection );
        void removeUnmanagedCollection( Collection *collection );

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
        void addTrackProvider( TrackProvider *provider );

        /**
         * removes a TrackProvider. Does not do anything if
         * CollectionManager does not know the given TrackProvider.
         *
         * Note: collections will be automatically removed from
         * the list of available TrackProviders.
         *
         * @param provider the provider to be removed
         */
        void removeTrackProvider( TrackProvider *provider );

    public slots:
        void startFullScan();
        void checkCollectionChanges();

    signals:
        void collectionAdded( Collection *newCollection );
        void collectionRemoved( QString collectionId );
        void trackProviderAdded( TrackProvider *provider );
        //this signal will be emitted after major changes to the collection
        //e.g. new songs where added, or an album changed
        //from compilation to non-compilation (and vice versa)
        //it will not be emitted on minor changes (e.g. the tags of a song were changed)
        void collectionDataChanged( Collection *changedCollection );

    private slots:
        void slotNewCollection( Collection *newCollection );
        void slotRemoveCollection();
        void slotCollectionChanged();

    private:
        friend class CollectionManagerSingleton;
        CollectionManager();

        void init();

        class Private;
        Private * const d;
};

#endif /* AMAROK_COLLECTIONMANAGER_H */
