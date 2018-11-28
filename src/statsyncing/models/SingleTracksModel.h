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

#ifndef STATSYNCING_SINGLETRACKSMODEL_H
#define STATSYNCING_SINGLETRACKSMODEL_H

#include "statsyncing/Track.h"
#include "statsyncing/models/CommonModel.h"

#include <QAbstractTableModel>

namespace StatSyncing
{
    /**
     * Model that provides data about single tracks that for some radon didn't end up
     * in synchronization.
     */
    class SingleTracksModel : public QAbstractTableModel, protected CommonModel
    {
        Q_OBJECT

        public:
            /**
             * Construct model of single tracks.
             *
             * @param tracks list of tracks
             * @param columns list of Meta::val* fields that will form columns of the model
             *                must include Meta::valTitle, may include: valRating,
             *                valFirstPlayed, valLastPlayed, valPlaycount, valLabel.
             * @param options the options
             * @param parent the parent QObject
             */
            SingleTracksModel( const TrackList &tracks, const QList<qint64> &columns,
                               const Options &options, QObject *parent = 0 );

            int rowCount( const QModelIndex &parent = QModelIndex() ) const;
            int columnCount( const QModelIndex &parent = QModelIndex() ) const;

            QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

            QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
            Qt::ItemFlags flags( const QModelIndex &index ) const;

            QStringList mimeTypes() const;
            QMimeData *mimeData( const QModelIndexList &indexes ) const;

        private:
            TrackList m_tracks;
    };

} // namespace StatSyncing

#endif // STATSYNCING_SINGLETRACKSMODEL_H
