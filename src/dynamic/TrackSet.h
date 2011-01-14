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

#include "core/meta/Meta.h"

#include <QBitArray>
#include <QStringList>

namespace Dynamic
{
    /**
     * A representation of a set of tracks as a bit array, relative to the
     * given universe set.
     * Intersecting TrackSets from different universes is not a good idea.
     * The BiasSolver uses this class to do a lot of set operations.
     * QSet is more space efficient for sparse sets, but set
     * operations generally aren't linear.
     */
    class TrackSet
    {
        public:
            /** Creates a TrackSet that represents the whole universe. All tracks are included.
             */
            TrackSet( const QList<QByteArray>& universe );

            TrackSet( const QList<QByteArray>& universe, const QList<QByteArray>& uidList );
            TrackSet( const QList<QByteArray>& universe, const QSet<QByteArray>& uidList );

            void reset();

            /**
             * The number of songs contained in this trackSet
             */
            int trackCount() const;

            QByteArray getRandomTrack( const QList<QByteArray>& universe ) const;

            void intersect( const TrackSet& );
            void subtract( const TrackSet& );

            TrackSet& operator=( const TrackSet& );

        private:
            QBitArray m_bits;
    };
}

#endif

