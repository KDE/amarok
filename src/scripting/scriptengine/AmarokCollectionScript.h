/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_COLLECTION_SCRIPT_H
#define AMAROK_COLLECTION_SCRIPT_H

#include "ScriptingDefines.h"

#include <QObject>
#include <QStringList>

class QScriptEngine;
namespace Collections
{
    class QueryMaker;
    class Collection;
    typedef QList<Collection*> CollectionList;
}

namespace Playlists
{
    class PlaylistProvider;
}

namespace AmarokScript
{
    /**
     * Previously Amarok.Collection
     */
    // SCRIPTDOX Amarok.CollectionManager
    class AmarokCollectionScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokCollectionScript( AmarokScriptEngine* scriptEngine );

            Q_INVOKABLE int totalAlbums() const;
            Q_INVOKABLE int totalArtists() const;
            Q_INVOKABLE int totalComposers() const;
            Q_INVOKABLE int totalGenres() const;
            Q_INVOKABLE int totalTracks() const;

            /**
             * DEPRECATED - Use Amarok.CollectionManager.queryableCollections[0].actualLocation instead
             * A list of locations that are part of the local collection
             */
            Q_INVOKABLE QStringList collectionLocation() const;

            /**
             * Directly query the Amarok SQLDB
             */
            Q_INVOKABLE QStringList query( const QString& sql ) const;

            /**
             * Escapes characters in string for use in an SQL statement.
             */
            Q_INVOKABLE QString escape( const QString& sql ) const;

            /**
             * Perform a full scan.
             */
            Q_INVOKABLE void scanCollection() const;

            /**
             * Perform an incremental scan.
             */
            Q_INVOKABLE void scanCollectionChanges() const;

            /**
             * Returns a query maker that queries all collections.
             */
            Q_INVOKABLE Collections::QueryMaker *queryMaker() const;

            /**
             * Returns a list of collections that can be queried with a querymaker.
             */
            Q_INVOKABLE Collections::CollectionList queryableCollections() const;

            /**
             * Return a list of collections that are viewable in the collection browser.
             */
            Q_INVOKABLE Collections::CollectionList viewableCollections() const;

        signals:
            /**
             * Emitted when a new collection is added.
             */
            void collectionAdded( Collections::Collection *newCollection );

            /**
             * This signal will be emitted after major changes to the collection
             * e.g. new songs where added, or an album changed
             * from compilation to non-compilation (and vice versa)
             * it will not be emitted on minor changes (e.g. the tags of a song were changed)
             *
             * This means that previously done searches can no longer
             * be considered valid.
             */
            void collectionDataChanged( Collections::Collection *changedCollection );

            /**
             * Emitted when a collection is removed.
             * @param collectionId The collectionID of the collection that was removed.
             */
            void collectionRemoved( QString collectionId );
    };
}

#endif
