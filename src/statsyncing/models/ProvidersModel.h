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

#ifndef STATSYNCING_PROVIDERSMODEL_H
#define STATSYNCING_PROVIDERSMODEL_H

#include "statsyncing/Provider.h"

#include <QAbstractListModel>
#include <QSet>
#include <QSharedPointer>

class QItemSelectionModel;

namespace StatSyncing
{
    class ProvidersModel : public QAbstractListModel
    {
        Q_OBJECT

        public:
            ProvidersModel( const ProviderPtrList &providers,
                            const ProviderPtrSet &checkedProviders, QObject *parent = 0 );
            virtual ~ProvidersModel();

            // QAbstractItemModel methods:
            QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
            int rowCount( const QModelIndex &parent = QModelIndex() ) const;
            Qt::ItemFlags flags( const QModelIndex &index ) const;
            bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

            // ProvidersModel methods:
            ProviderPtrSet checkedProviders() const;
            /// All providers - checkedProviders
            ProviderPtrSet unCheckedProviders() const;
            ProviderPtrList selectedProviders() const;

            /**
             * Return binary OR of fields that are reliable for track matching across
             * selected providers.
             */
            qint64 reliableTrackMetadataIntersection() const;

            /**
             * Return binary OR of fields that can be synchronized across selected
             * providers. At-least-2 intersection is used - field is considered useful
             * in synchronization only if at least 2 providers support it.
             */
            qint64 writableTrackStatsDataIntersection() const;

            /**
             * You must assign this selection model to your view so that selectedProviders
             * gives accurate results. ProvidersModel owns the selection model.
             */
            QItemSelectionModel *selectionModel() const;

            /**
             * Returns fields bit-field as i18n'ed string of comma separated field names.
             */
            QString fieldsToString( qint64 fields ) const;

        signals:
            void selectedProvidersChanged();

        private:
            ProviderPtrList m_providers;
            ProviderPtrSet m_checkedProviders;
            QItemSelectionModel *m_selectionModel;
    };
} // namespace StatSyncing

#endif // STATSYNCING_PROVIDERSMODEL_H
