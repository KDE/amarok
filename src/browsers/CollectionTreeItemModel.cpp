/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
#include "CollectionTreeItem.h"
#include "Debug.h"
#include "Amarok.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "covermanager/CoverFetcher.h"
#include "QueryMaker.h"

#include <KLocale>
#include <QTimer>


CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    : CollectionTreeItemModelBase()
{
    CollectionManager* collMgr = CollectionManager::instance();
    connect( collMgr, SIGNAL( collectionAdded( Amarok::Collection* ) ), this, SLOT( collectionAdded( Amarok::Collection* ) ), Qt::QueuedConnection );
    connect( collMgr, SIGNAL( collectionRemoved( QString ) ), this, SLOT( collectionRemoved( QString ) ) );
    //delete m_rootItem; //clears the whole tree!
    m_rootItem = new CollectionTreeItem( this );
    d->m_collections.clear();
    QList<Amarok::Collection*> collections = CollectionManager::instance()->viewableCollections();
    foreach( Amarok::Collection *coll, collections )
    {
        connect( coll, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;
        d->m_collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem, this ) ) );
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
    m_levelType = levelType;
    update();
    if ( d->m_collections.count() == 1 )
        QTimer::singleShot( 0, this, SLOT( requestCollectionsExpansion() ) );
}

QVariant
CollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());

    if ( item->isDataItem() )
    {
        if ( role == Qt::DecorationRole )
        {
            int level = item->level() -1;

            if ( d->m_childQueries.values().contains( item ) )
            {
                if( level < m_levelType.count() )
                    return m_currentAnimPixmap;
            }

            if ( level >= 0 && level < m_levelType.count() ) {
                if (  m_levelType[level] == CategoryId::Album )
                {
                    if( AmarokConfig::showAlbumArt() )
                    {
                        Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( item->data() );
                        if( album )
                            return album->imageWithBorder( 32, 2 );
                        return iconForLevel( level  );
                    }
                }
                return iconForLevel( level );
            }
        } else if ( role == AlternateCollectionRowRole )
            return ( index.row() % 2 == 1 );
    }

    return item->data( role );
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
CollectionTreeItemModel::collectionAdded( Amarok::Collection *newCollection )
{
    DEBUG_BLOCK
    if ( !newCollection )
        return;

    connect( newCollection, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;

    QString collectionId = newCollection->collectionId();
    if ( d->m_collections.contains( collectionId ) )
        return;

    //inserts new collection at the end. sort collection alphabetically?
    beginInsertRows( QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount() );
    d->m_collections.insert( collectionId, CollectionRoot( newCollection, new CollectionTreeItem( newCollection, m_rootItem, this ) ) );
    endInsertRows();

    if( d->m_collections.count() == 1 )
        QTimer::singleShot( 0, this, SLOT( requestCollectionsExpansion() ) );
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
            d->m_collections.remove( collectionId );
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


namespace Amarok {
    // TODO Internationlise
    void manipulateThe( QString &str, bool reverse )
    {
        if( reverse )
        {
            if( !str.startsWith( "the ", Qt::CaseInsensitive ) )
                return;

            QString begin = str.left( 3 );
            str = str.append( ", %1" ).arg( begin );
            str = str.mid( 4 );
            return;
        }

        if( !str.endsWith( ", the", Qt::CaseInsensitive ) )
            return;

        QString end = str.right( 3 );
        str = str.prepend( "%1 " ).arg( end );

        uint newLen = str.length() - end.length() - 2;

        str.truncate( newLen );
    }
}

void CollectionTreeItemModel::update()
{
    /*delete m_rootItem; //clears the whole tree!
    m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    d->m_collections.clear();
    QList<Amarok::Collection*> collections = CollectionManager::instance()->viewableCollections();
    foreach( Amarok::Collection *coll, collections )
    {
        connect( coll, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;
        d->m_collections.insert( coll->collectionId(), CollectionRoot( coll, new CollectionTreeItem( coll, m_rootItem ) ) );
    }
    m_rootItem->setChildrenLoaded( true ); //children of the root item are the collection items
    updateHeaderText();
    m_expandedItems.clear();
    m_expandedVariousArtistsNodes.clear();
    reset();*/
    for( int i = 0; i < m_rootItem->childCount(); i++ )
    {
        markSubTreeAsDirty( m_rootItem->child( i ) );
        ensureChildrenLoaded( m_rootItem->child( i ) );
    }

}

#include "CollectionTreeItemModel.moc"

