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
#include "core/transcoding/TranscodingConfiguration.h"
#include <QObject>

namespace Collections
{
    class QueryMaker;
    class Collection;
}
class QScriptEngine;
class QScriptValue;
class QIcon;

using Transcoding::Configuration;

namespace AmarokScript
{
    #ifdef DEBUG
        class AMAROK_EXPORT
    #else
        class
    #endif
    CollectionPrototype : public QObject
    {
        Q_OBJECT

        /**
         * Indicated whether user can choose track file path within this collection.
         */
        Q_PROPERTY( bool isOrganizable READ isOrganizable )

        /**
         * Indicated whether this collection can be written to (tracks added, removed).
         */
        Q_PROPERTY( bool isWritable READ isWritable )

        /**
         * A unique identifier for this collection
         */
        Q_PROPERTY( QString collectionId READ collectionId )

        /**
         * A user visible name for this collection, to be displayed in the collection-browser and elsewhere
         */
        Q_PROPERTY( QString prettyName READ prettyName )

        /**
         * Return the used space on this collection.
         */
        Q_PROPERTY( float usedCapacity READ usedCapacity )

        /**
         * Return the total storage capacity the collection.
         */
        Q_PROPERTY( float totalCapacity READ totalCapacity )

        /**
         *
         */
        Q_PROPERTY( bool isValid READ isValid )

        /**
         *
         */
        Q_PROPERTY( QIcon icon READ icon )

        /**
         *
         */
        Q_PROPERTY( bool isQueryable READ isQueryable )

        /**
         *
         */
        Q_PROPERTY( bool isViewable READ isViewable )

        /**
         * Indicated whether tracks may be transcoded when copying tracks to this collection.
         */
        Q_PROPERTY( bool supportsTranscode READ supportsTranscode )

        public:
            static void init( QScriptEngine *engine );
            static QScriptValue toScriptValue( QScriptEngine *engine,
                                            Collections::Collection* const &collection );
            static void fromScriptValue( const QScriptValue &obj,
                                         Collections::Collection* &collection );

        public slots:

            /**
             * Copy [and optionally transcode] a list of tracks to the destination collection
             */
            void copyTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection,
                             Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             * Copy [and optionally transcode] a single track to the destination collection
             */
            void copyTrack( const Meta::TrackPtr track, Collections::Collection *targetCollection,
                             Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             *
             */
            void queryAndcopyTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection,
                        Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             * Copy [and optionally transcode] a list of tracks to the destination collection.
             */
            void moveTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection,
                             Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             * Move [and optionally transcode] a single track to the destination collection.
             */
            void moveTrack( const Meta::TrackPtr track, Collections::Collection *targetCollection,
                             Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             *
             */
            void queryAndmoveTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection,
                        Configuration tc = Configuration( Transcoding::JUST_COPY ) );

            /**
             * Remove tracks from collection.
             */
            void removeTracks( const Meta::TrackList &trackList );

            /**
             * Remove single track from collection.
             */
            void removeTrack( const Meta::TrackPtr track );

            /**
             *
             */
            void queryAndRemoveTracks( Collections::QueryMaker *qm );

            /**
             * Return a query maker object for querying the collection
             */
            Collections::QueryMaker *queryMaker();

        private:
            Collections::Collection *m_collection;

            bool isOrganizable() const;
            bool isWritable() const;
            QString collectionId() const;
            QString prettyName() const;
            float usedCapacity() const;
            float totalCapacity() const;
            bool isValid() const;
            QIcon icon() const;
            bool isQueryable();
            bool isViewable();
            bool supportsTranscode();

            CollectionPrototype( QScriptEngine *engine,
                                Collections::Collection *collection );

        private slots:
            //set m_collection to null when collection destroyed
            void slotCollectionDestroyed();

        signals:
            void finishCopy();
            void finishRemove();
            void aborted();
            void updated();
    };
}

#endif
