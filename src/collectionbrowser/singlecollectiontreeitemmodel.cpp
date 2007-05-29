/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#define DEBUG_PREFIX "SingleCollectionTreeItemModel"

#include "singlecollectiontreeitemmodel.h"

#include "collectiontreeitem.h"
//#include "collection/sqlregistry.h"
#include "debug.h"
#include "amarok.h"
#include "collection.h"
#include "collectionmanager.h"
#include "querymaker.h"

#include <KLocale>
#include <KIcon>
#include <KIconLoader>
#include <QMapIterator>
#include <QMimeData>
#include <QPixmap>
#include <QTimer>


SingleCollectionTreeItemModel::SingleCollectionTreeItemModel( Collection * collection, const QList<int> &levelType )
    :CollectionTreeItemModelBase( ) 
{
    m_collection = collection;
    setLevels( levelType );

}


void
SingleCollectionTreeItemModel::setLevels( const QList<int> &levelType ) {
    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    m_rootItem = new CollectionTreeItem( m_collection, 0 );
    updateHeaderText();
    reset(); //resets the whole model, as the data changed
}



QVariant
SingleCollectionTreeItemModel::data(const QModelIndex &index, int role) const
{

    //DEBUG_BLOCK
    if (!index.isValid())
        return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());

    if ( item->isDataItem() )
    {
        if ( role == Qt::DecorationRole ) {
            //don't substract one here like in collectiontreeitemmodel because
            //there is no collection level here

            //check if the item being queried is currently being populated

            int level = item->level();

            if ( d->m_childQueries.values().contains( item ) ) {
                if ( level < m_levelType.count() )
                    return m_currentAnimPixmap;
            }


            if ( level < m_levelType.count() )
                return iconForLevel( level );
        }
    }

    return item->data( role );
}


bool
SingleCollectionTreeItemModel::hasChildren ( const QModelIndex & parent ) const {
    //DEBUG_BLOCK
    CollectionTreeItem *item;
     if (!parent.isValid())
        item = m_rootItem;  // must be root item!
    else
        item = static_cast<CollectionTreeItem*>(parent.internalPointer());

    //we added the collection level so we have to be careful with the item level
    //return item->childrenLoaded() || item->level() == m_levelType.count();  //that's track level
    return item->level() < m_levelType.count();


}

void
SingleCollectionTreeItemModel::ensureChildrenLoaded( CollectionTreeItem *item ) const {
    if ( !item->childrenLoaded() ) {
        listForLevel( item->level() +1, item->queryMaker(), item );
    }
}

bool
SingleCollectionTreeItemModel::canFetchMore( const QModelIndex &parent ) const {
    //DEBUG_BLOCK
    if ( !parent.isValid() )
       return !m_rootItem->childrenLoaded();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() < m_levelType.count() && !item->childrenLoaded();
}

void
SingleCollectionTreeItemModel::fetchMore( const QModelIndex &parent ) {
    //DEBUG_BLOCK
    CollectionTreeItem *item;
    if ( parent.isValid() )
        item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    else
        item = m_rootItem;
    ensureChildrenLoaded( item );
}




#include "singlecollectiontreeitemmodel.moc"
