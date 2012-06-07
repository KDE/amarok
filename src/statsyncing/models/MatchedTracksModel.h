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

#include "statsyncing/Options.h"
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
            /**
             * Construct model of matched tracks.
             *
             * @param matchedTuples list of matched track tuples
             * @param columns list of Meta::val* fields that will form colums of the model
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

        private:
            QVariant tupleData( const TrackTuple &tuple, qint64 field, int role ) const;
            QVariant trackData( const Provider *provider, const TrackTuple &tuple,
                                qint64 field, int role ) const;
            using CommonModel::trackData;

            QList<TrackTuple> m_matchedTuples;
            int m_titleColumn;
            Options m_options;
    };

} // namespace StatSyncing

#endif // STATSYNCING_MATCHEDTRACKSMODEL_H
