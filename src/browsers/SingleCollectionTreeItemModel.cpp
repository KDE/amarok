/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "SingleCollectionTreeItemModel"

#include "SingleCollectionTreeItemModel.h"

#include <amarokconfig.h>
#include "core/support/Amarok.h"
#include "core/collections/Collection.h"
#include "CollectionTreeItem.h"
#include "core/support/Debug.h"

#include <KLocale>

SingleCollectionTreeItemModel::SingleCollectionTreeItemModel( Collections::Collection *collection,
                                                              const QList<CategoryId::CatMenuId> &levelType )
    :CollectionTreeItemModelBase( )
{
    m_collection = collection;
    //we only have one collection that, by its very nature, is always expanded
    m_expandedCollections.insert( m_collection );
    setLevels( levelType );

    connect( collection, SIGNAL(updated()), this, SLOT(slotFilter()) ) ;
}

void
SingleCollectionTreeItemModel::setLevels( const QList<CategoryId::CatMenuId> &levelType )
{
    if( m_levelType == levelType && m_rootItem )
        return;

    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    m_rootItem = new CollectionTreeItem( m_collection, 0, this );

    m_collections.insert( m_collection->collectionId(), CollectionRoot( m_collection, m_rootItem ) );

    updateHeaderText();
    m_expandedItems.clear();
    m_expandedSpecialNodes.clear();
    m_runningQueries.clear();
    m_childQueries.clear();
    m_compilationQueries.clear();
    reset(); //resets the whole model, as the data changed
}

QVariant
SingleCollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid() )
        return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );
    return dataForItem( item, role );
}

Qt::ItemFlags
SingleCollectionTreeItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = CollectionTreeItemModelBase::flags( index );
    return ( f &= ~Qt::ItemIsEditable );
}

bool
SingleCollectionTreeItemModel::canFetchMore( const QModelIndex &parent ) const
{
    if ( !parent.isValid() )
       return m_rootItem->requiresUpdate();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() < m_levelType.count() && item->requiresUpdate();
}

void
SingleCollectionTreeItemModel::fetchMore( const QModelIndex &parent )
{
    CollectionTreeItem *item;
    if ( parent.isValid() )
        item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    else
        item = m_rootItem;

    ensureChildrenLoaded( item );
}

void
SingleCollectionTreeItemModel::filterChildren()
{
    markSubTreeAsDirty( m_rootItem );
    ensureChildrenLoaded( m_rootItem );
}

#include "SingleCollectionTreeItemModel.moc"

