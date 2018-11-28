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

#ifndef STATSYNCING_MATCHEDTRACKSMODEL_H
#define STATSYNCING_MATCHEDTRACKSMODEL_H

#include "statsyncing/Provider.h"
#include "statsyncing/models/CommonModel.h"

#include <QAbstractItemModel>

namespace StatSyncing
{
    class TrackTuple;

    /**
     * Model that provides data about matched tracks that should participate in statistics
     * synchronization.
     */
    class MatchedTracksModel : public QAbstractItemModel, protected CommonModel
    {
        Q_OBJECT

        public:
            enum {
                TupleFlagsRole = CommonModel::UserRole,
            };

            /**
             * Flags for track tuple status
             */
            enum TupleFlag {
                HasConflict = 1 << 0, /// there is at least one potential rating conflict
                HasUpdate = 1 << 1, /// there is at least one field going to be updated
            };

            /**
             * Construct model of matched tracks.
             *
             * @param matchedTuples list of matched track tuples
             * @param columns list of Meta::val* fields that will form columns of the model
             *                must include Meta::valTitle, may include: valRating,
             *                valFirstPlayed, valLastPlayed, valPlaycount, valLabel.
             * @param options options for synchronizing individual tracks
             */
            MatchedTracksModel( const QList<TrackTuple> &matchedTuples,
                                const QList<qint64> &columns, const Options &options,
                                QObject *parent = 0 );

            QModelIndex index( int row, int column,
                               const QModelIndex &parent = QModelIndex() ) const;
            QModelIndex parent( const QModelIndex &child ) const;

            bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
            int rowCount( const QModelIndex &parent = QModelIndex() ) const;
            int columnCount( const QModelIndex &parent = QModelIndex() ) const;

            QVariant headerData( int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole ) const;
            QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
            bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
            Qt::ItemFlags flags( const QModelIndex &index ) const;

            // MatchedTracksModel-specific methods:
            /**
             * Return a list of matched tuples, the same passed to model constructor, but
             * some may be changed, e.g. conflict-resolved etc.
             */
            const QList<TrackTuple> &matchedTuples();

            /**
             * Return true if at least one of the tuple is going to be updated. Warning:
             * can depend on whether a conflict of one tuple is resolved or not.
             */
            bool hasUpdate() const;

            /**
             * Return true if given or at least one tuple has potential conflict.
             * @param i if >= 0, queries tuple at position i; otherwise match any tuple
             */
            bool hasConflict( int i = -1 ) const;

            /**
             * Go through all tuples with (both resolved and unresolved) rating conflict
             * and (re)set their preferred rating provider to @p provider. Null
             * @param provider resets all tuples to "undecided". If @p provider is
             * not null and given tuple has no track from provider, its state remains
             * unchanged.
             */
            void takeRatingsFrom( const ProviderPtr &provider );

            /**
             * Go through all tuples with (both resolved and unresolved) labels conflict
             * and add @param provider to list of their label sources. Tracks that don't
             * have @param provider in their providers remain unchanged.
             */
            void includeLabelsFrom( const ProviderPtr &provider );

            /**
             * Go through all tuples with (both resolved and unresolved) labels conflict
             * and remove @param provider from their list of label sources. Tracks that
             * don't have @param provider in their label sources remain unchanged.
             *
             * If @param provider is null, this methods resets all tubles to "undecided"
             * wrt labels (clears their list of label sources).
             */
            void excludeLabelsFrom( const ProviderPtr &provider );

        private:
            QVariant tupleData( const TrackTuple &tuple, qint64 field, int role ) const;
            QVariant trackData( ProviderPtr provider, const TrackTuple &tuple,
                                qint64 field, int role ) const;
            using CommonModel::trackData;

            QList<TrackTuple> m_matchedTuples;
            int m_titleColumn;
    };

} // namespace StatSyncing

#endif // STATSYNCING_MATCHEDTRACKSMODEL_H
