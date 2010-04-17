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

#define DEBUG_PREFIX "APG::TreeModel"

#include "TreeModel.h"

#include "Constraint.h"
#include "ConstraintFactory.h"

#include "core/support/Debug.h"


APG::TreeModel::TreeModel( ConstraintNode* r, QObject* p ) : QAbstractItemModel( p ), m_rootNode( r )
{
    if ( m_rootNode->getRowCount() < 1 )
        ConstraintFactory::instance()->createGroup( m_rootNode, 0 );

    connectDCSlotToNode( m_rootNode );
}

APG::TreeModel::~TreeModel()
{
}

QVariant
APG::TreeModel::data( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() || ( role != Qt::DisplayRole ) )
        return QVariant();

    ConstraintNode* n = static_cast<ConstraintNode*>( index.internalPointer() );
    if ( index.column() == 0 ) {
        return QVariant( n->getName() );
    } else {
        return QVariant();
    }
}

Qt::ItemFlags
APG::TreeModel::flags( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
APG::TreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        if ( section == 0 ) {
            return QVariant( i18n("Name") );
        } else {
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex
APG::TreeModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !hasIndex( row, column, parent ) )
        return QModelIndex();

    ConstraintNode* parentItem;

    if ( !parent.isValid() )
        parentItem = m_rootNode;
    else
        parentItem = static_cast<ConstraintNode*>( parent.internalPointer() );

    ConstraintNode* childItem = parentItem->getChild( row );
    if ( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex
APG::TreeModel::parent( const QModelIndex& child ) const
{
    if ( !child.isValid() )
        return QModelIndex();

    ConstraintNode* childItem = static_cast<ConstraintNode*>( child.internalPointer() );
    ConstraintNode* parentItem = static_cast<ConstraintNode*>( childItem->parent() );

    if ( !parentItem || parentItem == m_rootNode )
        return QModelIndex();

    return createIndex( parentItem->row(), 0, parentItem );
}

int
APG::TreeModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    ConstraintNode* parentItem;
    if ( !parent.isValid() )
        parentItem = m_rootNode;
    else
        parentItem = static_cast<ConstraintNode*>( parent.internalPointer() );

    return parentItem->getRowCount();
}

bool
APG::TreeModel::removeNode( const QModelIndex& index )
{
    int row = index.row();
    bool r = false;

    QModelIndex parentIndex = index.parent();
    // prevent removal of root node
    if ( parentIndex.isValid() ) {
        ConstraintNode* parent;
        parent = static_cast<ConstraintNode*>( parentIndex.internalPointer() );
        beginRemoveRows( parentIndex, row, row );
        r = parent->removeChild( row );
        endRemoveRows();
        return r;
    } else {
        return r;
    }
}

QModelIndex
APG::TreeModel::insertGroup( const QModelIndex& thisIdx )
{
    int row = thisIdx.row();
    ConstraintNode* p = 0;
    ConstraintNode* n = 0;

    if ( thisIdx.isValid() )
        p = static_cast<ConstraintNode*>( thisIdx.internalPointer() );
    else
        p = m_rootNode;

    if ( p->getNodeType() == Constraint::ConstraintGroupType ) {
        beginInsertRows( thisIdx, 0, 0 );
        n = ConstraintFactory::instance()->createGroup( p, 0 );
        endInsertRows();
        if ( n != 0 ) {
            connectDCSlotToNode( n );
            return createIndex( 0, 0, n );
        }
    } else {
        p = p->parent();
        QModelIndex parentIdx = parent( thisIdx );
        beginInsertRows( parentIdx, row + 1, row + 1 );
        n = ConstraintFactory::instance()->createGroup( p, row + 1 );
        endInsertRows();
        if ( n != 0 ) {
            connectDCSlotToNode( n );
            return createIndex( row + 1, 0, n );
        }
    }
    return thisIdx;
}

QModelIndex
APG::TreeModel::insertConstraint( const QModelIndex& thisIdx, const QString& constraintType )
{
    int row = thisIdx.row();
    if ( thisIdx.isValid() ) {
        ConstraintNode* p = static_cast<ConstraintNode*>( thisIdx.internalPointer() );
        ConstraintNode* n = 0;
        if ( p->getNodeType() == Constraint::ConstraintGroupType ) {
            beginInsertRows( thisIdx, 0, 0 );
            n = ConstraintFactory::instance()->createConstraint( constraintType, p, 0 );
            endInsertRows();
            if ( n != 0 ) {
                connectDCSlotToNode( n );
                return createIndex( 0, 0, n );
            }
        } else {
            p = p->parent();
            QModelIndex parentIdx = parent( thisIdx );
            beginInsertRows( parentIdx, row + 1, row + 1 );
            n = ConstraintFactory::instance()->createConstraint( constraintType, p, row + 1 );
            endInsertRows();
            if ( n != 0 ) {
                connectDCSlotToNode( n );
                return createIndex( row + 1, 0, n );
            }
        }
    }
    return thisIdx;
}

void
APG::TreeModel::slotConstraintDataChanged()
{
    ConstraintNode* n = static_cast<ConstraintNode*>( sender() );
    if ( n ) {
        QModelIndex idx = createIndex( n->row(), 0, n );
        emit dataChanged( idx, idx );
    }
}

void
APG::TreeModel::connectDCSlotToNode( ConstraintNode* n )
{
    if ( n ) {
        connect( n, SIGNAL( dataChanged() ), this, SLOT ( slotConstraintDataChanged() ) );
        int rc = n->getRowCount();
        for ( int i = 0; i < rc; i++ ) {
            connectDCSlotToNode( n->getChild( i ) );
        }
    }
}
