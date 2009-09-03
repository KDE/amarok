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

#include "SingleCollectionTreeItemModel.h"

#include "Amarok.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "CollectionTreeItem.h"
#include "Debug.h"
#include "QueryMaker.h"

#include <KLocale>


SingleCollectionTreeItemModel::SingleCollectionTreeItemModel( Amarok::Collection * collection, const QList<int> &levelType )
    :CollectionTreeItemModelBase( )
{
    m_collection = collection;
    //we only have one collection that, by its very nature, is always expanded
    m_expandedCollections.insert( m_collection );
    setLevels( levelType );

    connect( collection, SIGNAL( updated() ), this, SLOT( slotFilter() ) ) ;
}

void
SingleCollectionTreeItemModel::setLevels( const QList<int> &levelType )
{
    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    m_rootItem = new CollectionTreeItem( m_collection, 0 );

    d->m_collections.insert( m_collection->collectionId(), CollectionRoot( m_collection, new CollectionTreeItem( Meta::DataPtr(0), 0 ) ) );

    updateHeaderText();
    m_expandedItems.clear();
    m_expandedVariousArtistsNodes.clear();
    reset(); //resets the whole model, as the data changed
}

QVariant
SingleCollectionTreeItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

    if ( item->isDataItem() )
    {
        if ( role == Qt::DecorationRole ) {
            //don't subtract one here like in collectiontreeitemmodel because
            //there is no collection level here

            //check if the item being queried is currently being populated

            const int level = item->level();

            if ( d->m_childQueries.values().contains( item ) )
            {
                if ( level < m_levelType.count() )
                    return m_currentAnimPixmap;
            }

            if ( level < m_levelType.count() )
            {
                if (  m_levelType[level] == CategoryId::Album )
                {
                    Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( item->data() );
                    if( album)
                        return album->imageWithBorder( 32, 2 );
                    else
                        return iconForLevel( level );
                }
                else
                    return iconForLevel( level );
            }
        } else if ( role == AlternateCollectionRowRole )
            return ( index.row() % 2 == 1 );
    }

    return item->data( role );
}

Qt::ItemFlags
SingleCollectionTreeItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = CollectionTreeItemModelBase::flags( index );
    return ( f &= ~Qt::ItemIsEditable );
}

bool
SingleCollectionTreeItemModel::hasChildren ( const QModelIndex & parent ) const
{
    CollectionTreeItem *item;
    if ( !parent.isValid() )
        item = m_rootItem;  // must be root item!
    else
        item = static_cast<CollectionTreeItem*>(parent.internalPointer());

    //we added the collection level so we have to be careful with the item level
    //return item->childrenLoaded() || item->level() == m_levelType.count();  //that's track level
    return item->level() < m_levelType.count();
}

void
SingleCollectionTreeItemModel::ensureChildrenLoaded( CollectionTreeItem *item ) const
{
    if ( !item->childrenLoaded() )
        listForLevel( item->level() +1, item->queryMaker(), item );
}

bool
SingleCollectionTreeItemModel::canFetchMore( const QModelIndex &parent ) const
{
    if ( !parent.isValid() )
       return !m_rootItem->childrenLoaded();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() < m_levelType.count() && !item->childrenLoaded();
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
    m_rootItem->setChildrenLoaded( false );
}

#include "SingleCollectionTreeItemModel.moc"

