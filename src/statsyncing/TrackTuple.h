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

#include "statsyncing/Provider.h"
#include "statsyncing/Track.h"

#include <QMap>

namespace StatSyncing
{
    class Options;

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
             * Inserts a track into this tuple; if it already contains a track from
             * provider, the old track si replaced with the new one.
             *
             * It does make sense to only add tracks that are in some sence equal to tracks
             * already present in the tuple.
             *
             * @param provider the provider
             * @param track the track
             */
            void insert( ProviderPtr provider, const TrackPtr &track );

            /**
             * Returns a list of providers that have tracks in this tuple.
             */
            ProviderPtrList providers() const;

            /**
             * Returns provider of the i-th track in this tuple. If i is out of bounds,
             * returns null.
             */
            ProviderPtr provider( int i ) const;

            /**
             * Returns track associated with @p provider. Asserts that there's
             * a track from @param provider
             */
            TrackPtr track( const ProviderPtr &provider ) const;

            /**
             * Returns a number of tracks in this tuple.
             */
            int count() const;

            /**
             * Returns true if there are no tracks in the tuple, false otherwise.
             */
            bool isEmpty() const;

            /**
             * Return true if Meta::val* field @p field is going to be updated.
             * If @p provider is null, returns true if at least one child track
             * is going to be updated; otherwise works on a track from @p provider.
             *
             * @param field the field.
             * @param options the options.
             * @param provider the provider.
             */
            bool fieldUpdated( qint64 field, const Options &options, ProviderPtr provider = ProviderPtr() ) const;

            /**
             * Return true if there's at least one field going to be updated.
             */
            bool hasUpdate( const Options &options ) const;

            /**
             * Returns true if there's a (perhaps resolved) conflict in field &field
             */
            bool fieldHasConflict( qint64 field, const Options &options, bool includeResolved = true ) const;

            /**
             * Return true if there's a (perhaps resolved) conflict in this tuple.
             */
            bool hasConflict( const Options &options ) const;

            /**
             * Returns a provider whose track's rating will be used in case of conflict.
             * Will be null if rating provider hasn't been explicitly set.
             */
            ProviderPtr ratingProvider() const;

            /**
             * Sets the rating provider. Only accepts null provider or a provider of one
             * track in this tuple.
             */
            void setRatingProvider( const ProviderPtr &provider );

            /**
             * Returns providers whose labels will be OR-ed together in case of conflict.
             * Will be empty if no provider hasn't been explicitly set.
             */
            ProviderPtrSet labelProviders() const;

            /**
             * Sets label providers. Only accepts empty set a or a set of providers that
             * are contained in this tuple.
             */
            void setLabelProviders( const ProviderPtrSet &providers );

            /**
             * Return synchronized rating. Specifically, returns -1 if there's unsolved
             * rating conflict.
             */
            int syncedRating( const Options &options ) const;
            QDateTime syncedFirstPlayed( const Options &options ) const;
            QDateTime syncedLastPlayed( const Options &options ) const;
            int syncedPlaycount( const Options &options ) const;
            QSet<QString> syncedLabels( const Options &options ) const;

            /**
             * Perform actual synchronization. For each track, only sets fields that are
             * in fieldUpdated( .., .., provider). Specifically this method does not write
             * ratings or labels if there's unresolved rating/label conflict. Can only be
             * called from non-main thread and may block for longer time.
             *
             * @return a set of providers that had their track updated
             */
            ProviderPtrSet synchronize( const Options &options ) const;

        private:
            int syncedRating( const Options &options, ProviderPtr ratingProvider ) const;
            // @param hasConflict is set to true or false
            QSet<QString> syncedLabels( const Options &options, const ProviderPtrSet &labelProviders,
                                        bool &hasConflict ) const;

            static const QList<qint64> s_fields; /// list of Meta::val* fields capable of syncing
            QMap<ProviderPtr, TrackPtr> m_map;
            ProviderPtr m_ratingProvider; /// source of rating in the event of conflict
            ProviderPtrSet m_labelProviders; /// sources of labels in the event of conflict
    };

} // namespace StatSyncing

#endif // STATSYNCING_TRACKTUPLE_H
