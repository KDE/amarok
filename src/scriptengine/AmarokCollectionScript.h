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

#include <QObject>
#include <QStringList>

class QScriptEngine;
namespace Collections
{
    class QueryMaker;
    class Collection;
    typedef QList<Collection*> CollectionList;
}

namespace AmarokScript
{
    class AmarokCollectionScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokCollectionScript( QScriptEngine* scriptEngine );

        public slots:
            int totalAlbums() const;
            int totalArtists() const;
            int totalComposers() const;
            int totalGenres() const;
            int totalTracks() const;

            //DEPRECATE
            QStringList collectionLocation() const;

            QStringList query( const QString& sql ) const;
            QString escape( const QString& sql ) const;
            void scanCollection() const;
            void scanCollectionChanges() const;

            /**
             * Returns a query maker that queries all collections.
             */
            Collections::QueryMaker *queryMaker();

            /**
             * Return a
             */
            Collections::CollectionList queryableCollections();
            Collections::CollectionList viewableCollections();

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

            void collectionRemoved( QString collectionId );
    };
}

#endif
