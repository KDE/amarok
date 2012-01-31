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

#include <KLocale>

#include <QTimer>
#include <QMap>

CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    : CollectionTreeItemModelBase()
{
    CollectionManager* collMgr = CollectionManager::instance();
    connect( collMgr, SIGNAL( collectionAdded( Collections::Collection* ) ), this, SLOT( collectionAdded( Collections::Collection* ) ), Qt::QueuedConnection );
    connect( collMgr, SIGNAL( collectionRemoved( QString ) ), this, SLOT( collectionRemoved( QString ) ) );
    //delete m_rootItem; //clears the whole tree!
    m_rootItem = new CollectionTreeItem( this );
    d->collections.clear();
    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();
    foreach( Collections::Collection *coll, collections )
    {
        connect( coll, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;
        d->collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem, this ) ) );
    }
    //m_rootItem->setChildrenLoaded( true ); //children of the root item are the collection items
    updateHeaderText();
    setLevels( levelType );
    debug() << "Collection root has " << m_rootItem->childCount() << " children";
}

CollectionTreeItemModel::~CollectionTreeItemModel()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "Collection Browser" );
    config.writeEntry( "TreeCategory", levels() );
}

void
CollectionTreeItemModel::setLevels( const QList<int> &levelType )
{
    if( m_levelType == levelType && m_rootItem )
        return;

    m_levelType = levelType;
    delete m_rootItem; //clears the whole tree!
    m_rootItem = new CollectionTreeItem( this );
    d->collections.clear();
    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();
    foreach( Collections::Collection *coll, collections )
    {
        connect( coll, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;
        d->collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem, this ) ) );
    }
    m_rootItem->setRequiresUpdate( false );  //all collections have been loaded already
    updateHeaderText();
    m_expandedItems.clear();
    m_expandedSpecialNodes.clear();
    d->runningQueries.clear();
    d->childQueries.clear();
    d->compilationQueries.clear();
    reset();
    if ( d->collections.count() == 1 )
        QTimer::singleShot( 0, this, SLOT( requestCollectionsExpansion() ) );
}

Qt::ItemFlags
CollectionTreeItemModel::flags( const QModelIndex &idx ) const
{
    Qt::ItemFlags flags = CollectionTreeItemModelBase::flags( idx );
    //TODO: check for CollectionLocation::isWritable().
    if( !idx.parent().isValid() )
        return flags | Qt::ItemIsDropEnabled;
    else
        return flags;
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
CollectionTreeItemModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row,
                                  int column, const QModelIndex &parent )
{
    //no drops on empty areas
    if( !parent.isValid() )
        return false;

    if( parent.isValid() && ( row != -1 && column != -1 ) )
        return false; //only droppable on root (collection header) items.

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    Q_ASSERT(item->type() == CollectionTreeItem::Collection);

    Collections::CollectionLocation *targetLocation = item->parentCollection()->location();
    Q_ASSERT(targetLocation);

    //TODO: accept external drops.
    const AmarokMimeData *mimeData = qobject_cast<const AmarokMimeData *>( data );
    Q_ASSERT(mimeData);

    //TODO: optimize for copy from same provider.
    Meta::TrackList tracks = mimeData->tracks();
    QMap<Collections::Collection *, Meta::TrackPtr> collectionTrackMap;

    foreach( Meta::TrackPtr track, tracks )
    {
        Collections::Collection *sourceCollection = track->collection();
        collectionTrackMap.insertMulti( sourceCollection, track );
    }

    foreach( Collections::Collection *sourceCollection, collectionTrackMap.uniqueKeys() )
    {
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

        if( sourceLocation == targetLocation )
            continue;

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

void
CollectionTreeItemModel::collectionAdded( Collections::Collection *newCollection )
{
    DEBUG_BLOCK

    if ( !newCollection )
        return;

    connect( newCollection, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;

    QString collectionId = newCollection->collectionId();
    if ( d->collections.contains( collectionId ) )
        return;

    debug() << "Added collection id:" << collectionId;

    //inserts new collection at the end.
    beginInsertRows( QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount() );
    d->collections.insert( collectionId, CollectionRoot( newCollection, new CollectionTreeItem( newCollection, m_rootItem, this ) ) );
    endInsertRows();

    if( d->collections.count() == 1 )
        QTimer::singleShot( 0, this, SLOT( requestCollectionsExpansion() ) );
}

void
CollectionTreeItemModel::collectionRemoved( const QString &collectionId )
{
    DEBUG_BLOCK

    debug() << "Removed collection id:" << collectionId;

    int count = m_rootItem->childCount();
    for( int i = 0; i < count; i++ )
    {
        CollectionTreeItem *item = m_rootItem->child( i );
        if( item && !item->isDataItem() && item->parentCollection()->collectionId() == collectionId )
        {
            beginRemoveRows( QModelIndex(), i, i );
            m_rootItem->removeChild( i );
            d->collections.remove( collectionId );
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
        //CollectionTreeItem *item = m_rootItem->child( i );
        //if( item )
        //    item->setChildrenLoaded( false );
        markSubTreeAsDirty( m_rootItem->child( i ) );
        ensureChildrenLoaded( m_rootItem->child( i ) );
    }
}

void
CollectionTreeItemModel::requestCollectionsExpansion()
{
    DEBUG_BLOCK
    for( int i = 0, count = m_rootItem->childCount(); i < count; i++ )
    {
        emit expandIndex( createIndex( i, 0, m_rootItem->child( i ) ) );
    }
}

void CollectionTreeItemModel::update()
{
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        markSubTreeAsDirty( m_rootItem->child( i ) );
        ensureChildrenLoaded( m_rootItem->child( i ) );
    }

}

#include "CollectionTreeItemModel.moc"

