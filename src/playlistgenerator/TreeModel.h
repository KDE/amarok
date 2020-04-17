/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_TREEMODEL
#define APG_TREEMODEL

#include "core/meta/forward_declarations.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class ConstraintNode;

namespace APG {
    class TreeModel : public QAbstractItemModel
    {
        Q_OBJECT

        public:
            explicit TreeModel( ConstraintNode*, QObject* parent = nullptr );
            ~TreeModel() override;

            QVariant data( const QModelIndex& index, int role ) const override;
            Qt::ItemFlags flags( const QModelIndex& index ) const override;
            QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
            QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
            QModelIndex parent( const QModelIndex& child ) const override;
            int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
            int columnCount( const QModelIndex& parent = QModelIndex() ) const override { Q_UNUSED( parent ); return 1; }
            bool removeNode( const QModelIndex& );
            QModelIndex insertGroup( const QModelIndex& );
            QModelIndex insertConstraint( const QModelIndex&, const QString& );

        private Q_SLOTS:
            void slotConstraintDataChanged();

        private:
            ConstraintNode* m_rootNode;

            void connectDCSlotToNode( ConstraintNode* );
    };
}
#endif
