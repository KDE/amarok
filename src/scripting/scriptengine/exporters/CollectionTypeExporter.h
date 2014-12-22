/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef COLLECTIONTYPE_EXPORTER_H
#define COLLECTIONTYPE_EXPORTER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"
#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"

#include <QIcon>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QWeakPointer>

namespace Collections
{
    class Collection;
    class QueryMaker;
}
class QScriptEngine;
class QScriptValue;

namespace AmarokScript
{
    // SCRIPTDOX PROTOTYPE Collections::Collection Collection
    #ifdef DEBUG
        class AMAROK_EXPORT
    #else
        class
    #endif
    CollectionPrototype : public QObject
    {
        Q_OBJECT

        /**
         * Indicates whether user can choose track file path within this collection.
         */
        Q_PROPERTY( bool isOrganizable READ isOrganizable )

        /**
         * Indicates whether this collection can be written to (tracks added, removed).
         */
        Q_PROPERTY( bool isWritable READ isWritable )

        /**
         * A unique identifier for this collection
         */
        Q_PROPERTY( QString collectionId READ collectionId )

        /**
         * A user visible name for this collection.
         */
        Q_PROPERTY( QString prettyName READ toString )

        /**
         * The used space on this collection.
         */
        Q_PROPERTY( float usedCapacity READ usedCapacity )

        /**
         * The total storage capacity the collection.
         */
        Q_PROPERTY( float totalCapacity READ totalCapacity )

        /**
         * Indicates whether this collection still exists.
         */
        Q_PROPERTY( bool isValid READ isValid )

        /**
         * An icon representing this collection.
         */
        Q_PROPERTY( QIcon icon READ icon )

        /**
         * Indicates whether this collection can be queried using a
         * QueryMaker object
         */
        Q_PROPERTY( bool isQueryable READ isQueryable )

        /**
         * Indicates whether the collection is viewable in the browser.
         */
        Q_PROPERTY( bool isViewable READ isViewable )

        /**
         * Indicated whether tracks may be transcoded when copying tracks to this collection.
         */
        Q_PROPERTY( bool supportsTranscode READ supportsTranscode )

        /**
         * A displayable string representation of the collection's location. use the return
         * value of this method to display the collection location to the user.
         * @return a string representation of the collection location
         */
        Q_PROPERTY( QString prettyLocation READ prettyLocation )

        /**
         * Returns a list of machine usable string*s representingthe collection location.
         * For example, a local collection would return a list of paths where tracks are
         * stored, while an Ampache collection would return a list with one string
         * containing the URL of an ampache server. Empty for collections like iPod collection and
         * MTP devices.
         */
        Q_PROPERTY( QStringList actualLocation READ actualLocation )

        public:
            static void init( QScriptEngine *engine );
            CollectionPrototype( Collections::Collection *collection );
            Collections::Collection* data() const { return m_collection.data(); }

            /**
             * Copy a list of tracks to the destination collection
             */
            Q_INVOKABLE void copyTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection );

            /**
             * Copy a single track to the destination collection
             */
            Q_INVOKABLE void copyTracks( const Meta::TrackPtr track, Collections::Collection *targetCollection );

            /**
             * Convenience method for copying tracks based on QueryMaker results,
             * takes ownership of the @param qm (The querymaker is rendered invalid
             * after copying).
             * @see copyTracks( Meta::TrackList, CollectionLocation* )
             */
            Q_INVOKABLE void queryAndCopyTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection );

            /**
             * Copy an array of tracks to the destination collection.
             */
            Q_INVOKABLE void moveTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection );

            /**
             * Move a single track to the destination collection.
             */
            Q_INVOKABLE void moveTracks( const Meta::TrackPtr track, Collections::Collection *targetCollection );

            /**
             * Convenience method for moving tracks based on QueryMaker results,
             * takes ownership of the @param qm (The querymaker is rendered invalid
             * after moving).
             * @see moveTracks( Meta::TrackList, CollectionLocation* )
             */
            Q_INVOKABLE void queryAndMoveTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection );

            /**
             * Remove an array of tracks from collection.
             */
            Q_INVOKABLE void removeTracks( const Meta::TrackList &trackList );

            /**
             * Remove single track from collection.
             */
            Q_INVOKABLE void removeTracks( const Meta::TrackPtr track );

            /**
             * Convenience method for removing tracks selected by QueryMaker,
             * takes ownership of the @param qm (The querymaker is rendered invalid
             * after the removal).
             * @see removeTracks( Meta::TrackList )
             */
            Q_INVOKABLE void queryAndRemoveTracks( Collections::QueryMaker *qm );

            /**
             * A querymaker object for querying the collection.
             */
            Q_INVOKABLE Collections::QueryMaker *queryMaker();

        private:
            QWeakPointer<Collections::Collection> m_collection;

            bool isOrganizable() const;
            bool isWritable() const;
            QString collectionId() const;
            QString toString() const;
            float usedCapacity() const;
            float totalCapacity() const;
            bool isValid() const;
            QIcon icon() const;
            bool isQueryable() const;
            bool isViewable() const ;
            bool supportsTranscode() const;
            QString prettyLocation() const;
            QStringList actualLocation() const;

            Meta::TrackList removeInvalidTracks( const Meta::TrackList &tracks );

       signals:
           /**
            * This signal will be emitted after major changes to the collection
            * e.g. new songs where added, or an album changed
            * from compilation to non-compilation (and vice versa)
            * it will not be emitted on minor changes (e.g. the tags of a song were changed)
            *
            * This means that previously done searches can no longer
            * be considered valid.
            */
           void updated();

            /**
             * Emited when this collection is removed.
             */
            void removed();

            /**
             * Emitted when a copy operation from/ to this collection has finished.
             */
            void finishCopy();

            /**
             * Emitted when a removal operation on this collection has finished.
             */
            void finishRemove();

            /**
             * Emitted when a copy/ move operation from/ to this collection was aborted.
             */
            void aborted();
    };
}

#endif
