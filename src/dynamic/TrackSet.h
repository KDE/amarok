/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TRACKSET_H
#define AMAROK_TRACKSET_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <QBitArray>
#include <QExplicitlySharedDataPointer>
#include <QHash>
#include <QMetaType>
#include <QSharedData>
#include <QString>
#include <QStringList>

namespace Dynamic
{
    class TrackSet;
    class TrackCollection;

    typedef QExplicitlySharedDataPointer<TrackCollection> TrackCollectionPtr;

    /**
     * We keep a list here of the uid of every track in the set
     * collection being considered. This is unfortunately necessary
     * because the algorithm in generateInitialPlaylist performs many
     * set subtractions and intersections which would be impractical and
     * inefficient to perform using database queries. Instead we
     * represent a set of tracks as a bit list, where the n'th bit
     * indicates whether the n'th track in s_universe is included in the
     * set. Set operations can then be performed extremely quickly using
     * bitwise operations, rather than tree operations which QSet would
     * use.
     */

    /** The TrackCollection stores all the uids that a TrackSet can contain.
        Usually the dynamic playlist queries all the uids before computing a playlist.
    */
    class AMAROK_EXPORT TrackCollection : public QSharedData
    {
        public:
            explicit TrackCollection( const QStringList& uids );

            int count() const;
            QStringList uids() const;

        private:
            QStringList m_uids;
            QHash<QString, int> m_ids;

            friend class TrackSet;
    };

    /**
     * A representation of a set of tracks as a bit array, relative to the
     * given universe set.
     * Intersecting TrackSets from different universes is not a good idea.
     * The BiasSolver uses this class to do a lot of set operations.
     * QSet is more space efficient for sparse sets, but set
     * operations generally aren't linear.
     */
    class AMAROK_EXPORT TrackSet
    {
        public:
            /** Creates a TrackSet that is outstanding
                @see isOutstanding() */
            TrackSet();

            TrackSet( const TrackSet& other );

            /** Creates a TrackSet that represents the whole universe.
             *  @param collection The collection of the tracks
             *  @param value If true set is set to "full". Else to "empty".
            */
            TrackSet( const Dynamic::TrackCollectionPtr collection, bool value );

            /** Includes or excludes all tracks in the set.
                @param value If true set is set to "full". Else to "empty".
            */
            void reset( bool value );

            /** Returns true if the results of this track set are not yet available */
            bool isOutstanding() const;

            /** The number of songs contained in this trackSet */
            int trackCount() const;

            /** True if none of the tracks are included in the set. */
            bool isEmpty() const;

            /** True if all of the tracks are included in the set. */
            bool isFull() const;
            bool contains( const Meta::TrackPtr& ) const;

            /** Returns true if the uid is included in the set */
            bool contains( const QString& uid ) const;

            /** Returns the uids of a random track contains in this set */
            QString getRandomTrack() const;

            void unite( const Meta::TrackPtr& );
            void unite( const TrackSet& );
            void unite( const QStringList& uids );
            void intersect( const TrackSet& );
            void intersect( const QStringList& uids );
            void subtract( const Meta::TrackPtr& );
            void subtract( const TrackSet& );
            void subtract( const QStringList& uids );

            TrackSet& operator=( const TrackSet& );

        private:
            QBitArray m_bits;
            TrackCollectionPtr m_collection;
    };
}

Q_DECLARE_METATYPE( Dynamic::TrackSet )

#endif

