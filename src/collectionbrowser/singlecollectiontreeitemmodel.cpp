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
#include <KStandardDirs>
#include <QMapIterator>
#include <QMimeData>
#include <QPixmap>
#include <QTimer>


SingleCollectionTreeItemModel::SingleCollectionTreeItemModel( Collection * collection, const QList<int> &levelType )
    :CollectionTreeItemModelBase( ) 
    , m_animFrame( 0 )
    , m_loading1( QPixmap( KStandardDirs::locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( QPixmap( KStandardDirs::locate("data", "amarok/images/loading2.png" ) ) )
    , m_currentAnimPixmap( m_loading1 )

{
    m_collection = collection;
    setLevels( levelType );

    m_timeLine = new QTimeLine( 10000, this );

    //m_timeLine->setDuration( 500 );
    m_timeLine->setFrameRange( 0, 20 );
    m_timeLine->setLoopCount ( 0 );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( loadingAnimationTick() ) );
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
        //if (m_timeLine->state() != QTimeLine::Running) 
            m_timeLine->start();
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

void SingleCollectionTreeItemModel::loadingAnimationTick()
{

    DEBUG_BLOCK
    if ( m_animFrame == 0 ) 
        m_currentAnimPixmap = m_loading2;
    else
        m_currentAnimPixmap = m_loading1;

    m_animFrame = 1 - m_animFrame;


    //trigger an update of all items being populated at the moment;
    QList<CollectionTreeItem* > items = d->m_childQueries.values();

    foreach (CollectionTreeItem* item, items) {

        emit ( dataChanged ( createIndex(item->row(), 0, item), createIndex(item->row(), 0, item) ) );
    }

    

}


#include "singlecollectiontreeitemmodel.moc"
