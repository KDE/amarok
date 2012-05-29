/****************************************************************************************
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

#ifndef STATSYNCING_TRACKTUPLE_H
#define STATSYNCING_TRACKTUPLE_H
#include "Track.h"
#include <QMap>

namespace StatSyncing
{

class Provider;
    /**
     * Smallest element of synchronization, a container for provider-to-one-track map with
     * methods to perform statistics synchronization and querying methods.
     */
    class TrackTuple
    {
        public:
            /**
             * Constructs an empty tuple.
             */
            TrackTuple();

            /**
             * Inserts a track into this tuple; if it already contains a track from @param
             * provider, the old track si replaced with the new one.
             *
             * It does make sense to only add tracks that are in some sence equal to tracks
             * alredy present in the tuple.
             */
            void insert( const Provider *provider, const TrackPtr &track );

            /**
             * Returns a list of providers that have tracks in this tuple.
             */
            QList<const Provider *> providers() const;

            /**
             * Returns provider of the i-th track in this tuple. If i is out of bounds,
             * returns null.
             */
            const Provider *provider( int i ) const;

            /**
             * Returns track associated with @provider provider. Asserts that there's
             * a track from @param provider
             */
            TrackPtr track( const Provider *provider ) const;

            /**
             * Returns a number of tracks in this tuple.
             */
            int count() const;

            /**
             * Returns true if there are no tracks in the tuple, false otherwise.
             */
            bool isEmpty() const;

        private:
            QMap<const Provider *, TrackPtr> m_map;
    };

} // namespace StatSyncing

#endif // STATSYNCING_TRACKTUPLE_H
