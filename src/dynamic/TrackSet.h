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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TRACKSET_H
#define AMAROK_TRACKSET_H

#include "Meta.h"

#include <QBitArray>

namespace Dynamic
{
    /* A representation of a set of tracks as a bit array, relative to the
     * universe set that DynamicModel keeps. The solver does a lot of set
     * operations. QSet is more space efficient for sparse sets, but set
     * operations generally aren't linear.
     */
    class TrackSet
    {
        public:
            TrackSet();
            TrackSet( const QList<QByteArray>& uidList );
            TrackSet( const QSet<QByteArray>& uidSet );

            void reset();

            int size() const;

            void clear(); //! make this the empty set
            void setUniverseSet(); //! make this the universe set


            void setTracks( const QList<QByteArray>& );
            void addTracks( const QList<QByteArray>& );
            void setTracks( const QSet<QByteArray>& );
            void addTracks( const QSet<QByteArray>& );

            QList<QByteArray> uidList() const;

            void unite( const TrackSet& );
            void intersect( const TrackSet& );
            void subtract( const TrackSet& );


            TrackSet& operator=( const TrackSet& );

        private:
            QBitArray m_bits;
    };
}

#endif

