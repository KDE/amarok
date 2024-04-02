/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "CollectionTreeItemModel"

#include "CollectionTreeItemModel.h"

#include <amarokconfig.h>
#include "AmarokMimeData.h"
#include "CollectionTreeItem.h"
#include "core/support/Debug.h"
#include "core/support/Amarok.h"
#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/FileCollectionLocation.h"

#include <KLocalizedString>

#include <QTimer>
#include <QMap>

CollectionTreeItemModel::CollectionTreeItemModel( const QList<CategoryId::CatMenuId> &levelType )
    : CollectionTreeItemModelBase()
{
    m_rootItem = new CollectionTreeItem( this );
    CollectionManager *collMgr = CollectionManager::instance();
    connect( collMgr, &CollectionManager::collectionAdded, this, &CollectionTreeItemModel::collectionAdded, Qt::QueuedConnection );
    connect( collMgr, &CollectionManager::collectionRemoved, this, &CollectionTreeItemModel::collectionRemoved );

    QList<Collections::Collection *> collections = CollectionManager::instance()->viewableCollections();
    foreach( Collections::Collection *coll, collections )
    {
        connect( coll, &Collections::Collection::updated, this, &CollectionTreeItemModel::slotFilterWithoutAutoExpand );
        m_collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem, this ) ) );
    }

    setLevels( levelType );
}

Qt::ItemFlags
CollectionTreeItemModel::flags( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return {};

    Qt::ItemFlags flags = CollectionTreeItemModelBase::flags( idx );
    if( idx.parent().isValid() )
        return flags; // has parent -> not a collection -> no drops

    // we depend on someone (probably CollectionTreeView) to call
    // CollectionTreeItemModelBase::setDragSourceCollections() every time a drag is
    // initiated or enters collection browser widget
    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( idx.internalPointer() );
    Q_ASSERT(item->type() == CollectionTreeItem::Collection);
    if( m_dragSourceCollections.contains( item->parentCollection() ) )
        return flags; // attempt to drag tracks from the same collection, don't allow this (bug 291068)

    if( !item->parentCollection()->isWritable() )
        return flags; // not writeable, disallow drops

    // all paranoid checks passed, tracks can be dropped to this item
    return flags | Qt::ItemIsDropEnabled;
}

QVariant
CollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());
    // subtract one here because there is a collection level for this model
    return dataForItem( item, role, item->level() - 1 );
}

bool
CollectionTreeItemModel::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                       int row, int column, const QModelIndex &parent )
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    //no drops on empty areas
    if( !parent.isValid() )
        return false;

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    Q_ASSERT(item->type() == CollectionTreeItem::Collection);

    Collections::Collection *targetCollection = item->parentCollection();
    Q_ASSERT(targetCollection);

    //TODO: accept external drops.
    const AmarokMimeData *mimeData = qobject_cast<const AmarokMimeData *>( data );
    Q_ASSERT(mimeData);

    //TODO: optimize for copy from same provider.
    Meta::TrackList tracks = mimeData->tracks();
    QMultiMap<Collections::Collection *, Meta::TrackPtr> collectionTrackMap;

    foreach( Meta::TrackPtr track, tracks )
    {
        Collections::Collection *sourceCollection = track->collection();
        collectionTrackMap.insert( sourceCollection, track );
    }

    foreach( Collections::Collection *sourceCollection, collectionTrackMap.uniqueKeys() )
    {
        if( sourceCollection == targetCollection )
            continue; // should be already caught by ...Model::flags(), but hey

        Collections::CollectionLocation *sourceLocation;
        if( sourceCollection )
        {
            sourceLocation = sourceCollection->location();
            Q_ASSERT(sourceLocation);
        }
        else
        {
            sourceLocation = new Collections::FileCollectionLocation();
        }

        // we need to create target collection location per each source collection location
        // -- prepareSomething() takes ownership of the pointer.
        Collections::CollectionLocation *targetLocation = targetCollection->location();
        Q_ASSERT(targetLocation);

        if( action == Qt::CopyAction )
        {
            sourceLocation->prepareCopy( collectionTrackMap.values( sourceCollection ),
                                         targetLocation );
        }
        else if( action == Qt::MoveAction )
        {
            sourceLocation->prepareMove( collectionTrackMap.values( sourceCollection ),
                                         targetLocation );
        }
    }
    return true;
}


bool
CollectionTreeItemModel::canFetchMore( const QModelIndex &parent ) const
{
    if ( !parent.isValid() )
        return false;       //children of the root item are the collections, and they are always known

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() <= m_levelType.count() && item->requiresUpdate();
}

void
CollectionTreeItemModel::fetchMore( const QModelIndex &parent )
{
    if ( !parent.isValid() )
        return;

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    ensureChildrenLoaded( item );
}

Qt::DropActions
CollectionTreeItemModel::supportedDropActions() const
{
    // this also causes supportedDragActions() to contain move action
    return CollectionTreeItemModelBase::supportedDropActions() | Qt::MoveAction;
}

void
CollectionTreeItemModel::collectionAdded( Collections::Collection *newCollection )
{
    if( !newCollection )
        return;

    connect( newCollection, &Collections::Collection::updated, this, &CollectionTreeItemModel::slotFilterWithoutAutoExpand ) ;

    QString collectionId = newCollection->collectionId();
    if( m_collections.contains( collectionId ) )
        return;

    //inserts new collection at the end.
    beginInsertRows( QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount() );
    m_collections.insert( collectionId, CollectionRoot( newCollection, new CollectionTreeItem( newCollection, m_rootItem, this ) ) );
    endInsertRows();

    if( m_collections.count() == 1 )
        QTimer::singleShot( 0, this, &CollectionTreeItemModel::requestCollectionsExpansion );
}

void
CollectionTreeItemModel::collectionRemoved( const QString &collectionId )
{
    int count = m_rootItem->childCount();
    for( int i = 0; i < count; i++ )
    {
        CollectionTreeItem *item = m_rootItem->child( i );
        if( item && !item->isDataItem() && item->parentCollection()->collectionId() == collectionId )
        {
            beginRemoveRows( QModelIndex(), i, i );
            m_rootItem->removeChild( i );
            m_collections.remove( collectionId );
            m_expandedCollections.remove( item->parentCollection() );
            endRemoveRows();
        }
    }
}

void
CollectionTreeItemModel::filterChildren()
{
    int count = m_rootItem->childCount();
    for ( int i = 0; i < count; i++ )
    {
        markSubTreeAsDirty( m_rootItem->child( i ) );
        ensureChildrenLoaded( m_rootItem->child( i ) );
    }
}

void
CollectionTreeItemModel::requestCollectionsExpansion()
{
    for( int i = 0, count = m_rootItem->childCount(); i < count; i++ )
    {
        Q_EMIT expandIndex( itemIndex( m_rootItem->child( i ) ) );
    }
}

