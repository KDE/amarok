 /*
  Copyright (c) 2007  Alexandre Pereira de Oliveira <aleprj@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "collectiontreeitemmodel.h"
#include "collectiontreeitem.h"
#include "collection/sqlregistry.h"
#include "debug.h"

CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    :QAbstractItemModel()
    , m_levelType( levelType )
{
    m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    setupModelData( listForLevel(0), m_rootItem );
}


CollectionTreeItemModel::~CollectionTreeItemModel() {
    delete m_rootItem;
}


QModelIndex CollectionTreeItemModel::index(int row, int column, const QModelIndex &parent)
             const
{

     CollectionTreeItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootItem;
     else
         parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

     CollectionTreeItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();
}

QModelIndex CollectionTreeItemModel::parent(const QModelIndex &index) const
{
     if (!index.isValid())
         return QModelIndex();

     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();

     if (parentItem == m_rootItem)
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);
}

int CollectionTreeItemModel::rowCount(const QModelIndex &parent) const
{
     CollectionTreeItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootItem;
     else
         parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

     return parentItem->childCount();
}

int CollectionTreeItemModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}


QVariant CollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

     if (role != Qt::DisplayRole)
         return QVariant();

     CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());

     return item->data(index.column());
}

Qt::ItemFlags CollectionTreeItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant CollectionTreeItemModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    return QVariant();
}


void CollectionTreeItemModel::setupModelData(const QList<Meta::DataPtr> &dataList, CollectionTreeItem *parent) {
    foreach( Meta::DataPtr data, dataList ) {
        CollectionTreeItem *item = new CollectionTreeItem( data, parent );
        QList<Meta::DataPtr> childrenData = listForLevel( item->level(), item->queryBuilder() );
        setupModelData( childrenData, item );
    }
}

QList<Meta::DataPtr> CollectionTreeItemModel::listForLevel( int level, QueryBuilder qb ) const {
    if ( level > m_levelType.count() )
        return QList<Meta::DataPtr>();
    if ( level == m_levelType.count() ) {
        return SqlRegistry::instance()->getTracks( qb );
    }
    switch( m_levelType[level] ) {
        case CategoryId::Album : return SqlRegistry::instance()->getAlbums( qb );
        case CategoryId::Artist : return SqlRegistry::instance()->getArtists( qb );
        case CategoryId::Composer : return SqlRegistry::instance()->getComposers( qb );
        case CategoryId::Genre : return SqlRegistry::instance()->getGenres( qb );
        case CategoryId::Year : return SqlRegistry::instance()->getYears( qb );
        default: return QList<Meta::DataPtr>();
    }
}
