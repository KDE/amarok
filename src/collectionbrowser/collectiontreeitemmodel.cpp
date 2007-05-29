/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#define DEBUG_PREFIX "CollectionTreeItemModel"

#include "collectiontreeitemmodel.h"

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
#include <QMimeData>
#include <QPixmap>
#include <QTimer>



CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    :CollectionTreeItemModelBase()

{
    CollectionManager* collMgr = CollectionManager::instance();
    connect( collMgr, SIGNAL( collectionAdded( Collection* ) ), this, SLOT( collectionAdded( Collection* ) ), Qt::QueuedConnection );
    connect( collMgr, SIGNAL( collectionRemoved( QString ) ), this, SLOT( collectionRemoved( QString ) ), Qt::QueuedConnection );
    setLevels( levelType );
    debug() << "Collection root has " << m_rootItem->childCount() << " childrens" << endl;
}


void
CollectionTreeItemModel::setLevels( const QList<int> &levelType ) {
    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    d->m_collections.clear();
    QList<Collection*> collections = CollectionManager::instance()->collections();
    foreach( Collection *coll, collections )
    {
        d->m_collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem ) ) );
    }
    m_rootItem->setChildrenLoaded( true ); //childrens of the root item are the collection items
    updateHeaderText();

    reset(); //resets the whole model, as the data changed
}


QVariant
CollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());

    if ( item->isDataItem() )
    {
        if ( role == Qt::DecorationRole ) {
            int level = item->level() -1;

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
CollectionTreeItemModel::hasChildren ( const QModelIndex & parent ) const {
     if (!parent.isValid())
         return true; // must be root item!

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(parent.internalPointer());
    //we added the collection level so we have to be careful with the item level
    return !item->isDataItem() || item->level() <= m_levelType.count(); 

}

void
CollectionTreeItemModel::ensureChildrenLoaded( CollectionTreeItem *item ) const {
    if ( !item->childrenLoaded() ) {
        listForLevel( item->level() /* +1 -1 */, item->queryMaker(), item );
    }
}

bool
CollectionTreeItemModel::canFetchMore( const QModelIndex &parent ) const {
    DEBUG_BLOCK
    if ( !parent.isValid() )
        return false;       //children of the root item are the collections, and they are alwas known
    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() <= m_levelType.count() && !item->childrenLoaded();
}

void
CollectionTreeItemModel::fetchMore( const QModelIndex &parent ) {
    DEBUG_BLOCK
    if ( !parent.isValid() )
        return;

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    ensureChildrenLoaded( item );
}

void
CollectionTreeItemModel::collectionAdded( Collection *newCollection ) {
    DEBUG_BLOCK
    if ( !newCollection )
        return;

    debug() << "Added new collection in collectionAdded with id " << newCollection->collectionId() << endl;
    QString collectionId = newCollection->collectionId();
    if ( d->m_collections.contains( collectionId ) )
        return;
    //inserts new collection at the end. sort collection alphabetically?
    beginInsertRows( QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount() );
    d->m_collections.insert( collectionId, CollectionRoot( newCollection, new CollectionTreeItem( newCollection, m_rootItem ) ) );
    endInsertRows();
}

void
CollectionTreeItemModel::collectionRemoved( const QString &collectionId ) {
    int count = m_rootItem->childCount();
    for ( int i = 0; i < count; i++ ) {
        CollectionTreeItem *item = m_rootItem->child( i );
        if ( !item->isDataItem() && item->parentCollection()->collectionId() == collectionId ) {
            beginRemoveRows( QModelIndex(), i, i );
            m_rootItem->removeChild( i );
            d->m_collections.remove( collectionId );
            endRemoveRows();
        }
    }
}


#include "collectiontreeitemmodel.moc"
