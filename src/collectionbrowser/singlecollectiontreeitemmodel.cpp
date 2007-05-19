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

struct SingleCollectionTreeItemModel::Private
{
    QMap<QueryMaker* , CollectionTreeItem* > m_childQueries;
};


SingleCollectionTreeItemModel::SingleCollectionTreeItemModel( Collection * collection, const QList<int> &levelType )
    :QAbstractItemModel()
    , m_rootItem( 0 )
    , d( new Private ) 
{
   /* CollectionManager* collMgr = CollectionManager::instance();
    connect( collMgr, SIGNAL( collectionAdded( Collection* ) ), this, SLOT( collectionAdded( Collection* ) ) );
    connect( collMgr, SIGNAL( collectionRemoved( QString ) ), this, SLOT( collectionRemoved( QString ) ) );
    setLevels( levelType );
    debug() << "Collection root has " << m_rootItem->childCount() << " childrens" << endl;*/

    m_collection = collection;
    //m_rootItem = new CollectionTreeItem( m_collection, 0 );
    //m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    setLevels( levelType );

}


SingleCollectionTreeItemModel::~SingleCollectionTreeItemModel() {
    //delete m_rootItem;
    //delete d;
}


void
SingleCollectionTreeItemModel::setLevels( const QList<int> &levelType ) {
    delete m_rootItem; //clears the whole tree!
    m_levelType = levelType;
    
    //m_rootItem = new CollectionTreeItem( Meta::DataPtr(0), 0 );
    m_rootItem = new CollectionTreeItem( m_collection, 0 );
    //populate root:
    //listForLevel( m_levelType[0], m_collection->queryBuilder(), m_rootItem );
    updateHeaderText();
    reset(); //resets the whole model, as the data changed
}

QModelIndex
SingleCollectionTreeItemModel::index(int row, int column, const QModelIndex &parent) const
{
    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

   //if ( parentItem->childrenLoaded() )
   //{
        CollectionTreeItem *childItem = parentItem->child(row);
        if (childItem) {
            return createIndex(row, column, childItem);
        } else {
            return QModelIndex();
        }
    //}
    //else
      // return QModelIndex();
}

QModelIndex
SingleCollectionTreeItemModel::parent( const QModelIndex &index ) const
{

    // DEBUG_BLOCK
     if ( !index.isValid() ) {
         debug() << "Invalid index" << endl;
         return QModelIndex();
     }
     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();


     if ( parentItem == 0 ) {
         debug() << "No parent, index points at m_rootItem" << endl;
         return QModelIndex();
     }

     if ( parentItem == m_rootItem ) {
         return QModelIndex();
     }

     return createIndex( parentItem->row(), 0, parentItem );
}

int
SingleCollectionTreeItemModel::rowCount(const QModelIndex &parent) const
{
    DEBUG_BLOCK
    CollectionTreeItem *parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    }
    else {
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());
    }
    if (parentItem == m_rootItem ) 
       debug() << "    m_rootItem with " << parentItem->childCount() <<  " children" << endl;
    else 
       debug() << "    something else with " << parentItem->childCount() <<  " children" << endl;

    //ensureChildrenLoaded( parentItem );
    //return parentItem->childCount();
   if ( parentItem->childrenLoaded() )
        return parentItem->childCount();
    else
        return 0; //hack!
}

int
SingleCollectionTreeItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED( parent )
    return 1;
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
            //don't substract one here like in colelctintreeitemmodel because
            //there is no collectin level here
            int level = item->level();
            if ( level < m_levelType.count() )
                return iconForLevel( level );
        }
    }

    return item->data( role );
}

Qt::ItemFlags
SingleCollectionTreeItemModel::flags(const QModelIndex &index) const
{
    //DEBUG_BLOCK
    if ( !index.isValid() || !index.parent().isValid() )
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant
SingleCollectionTreeItemModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        if (section == 0)
            return m_headerText;
    return QVariant();
}

QMimeData*
SingleCollectionTreeItemModel::mimeData( const QModelIndexList &indices ) const {
    if ( indices.isEmpty() )
        return 0;

    KUrl::List urls;

    foreach( QModelIndex index, indices ) {
        if (index.isValid()) {
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());
            ensureChildrenLoaded( item );
            urls += item->urls();
        }
    }

    QMimeData *mimeData = new QMimeData();
    urls.populateMimeData(mimeData);

    return mimeData;
}

void
SingleCollectionTreeItemModel::populateChildren(const DataList &dataList, CollectionTreeItem *parent) const {
    DEBUG_BLOCK
    foreach( Meta::DataPtr data, dataList ) {
        new CollectionTreeItem( data, parent );
    }
    if (parent == m_rootItem )
        debug() << "root item sucesfully populated, now has " << parent->childCount() << " children" << endl;
    parent->setChildrenLoaded( true );
}

void
SingleCollectionTreeItemModel::listForLevel( int level, QueryMaker *qm, CollectionTreeItem* parent ) const {
    DEBUG_BLOCK
    debug() << "    level: " << level << ", leveltype count: " << m_levelType.count() << endl;
    if ( qm && parent ) {
        for( QMapIterator<QueryMaker*, CollectionTreeItem*> iter( d->m_childQueries ); iter.hasNext(); ) {
            if( iter.next().value() == parent )
                return;             //we are already querying for children of parent
        }
        if ( level > m_levelType.count() )
            return;
        if ( level == m_levelType.count() ) {
            qm->startTrackQuery();
        }

        else {
            switch( m_levelType[level] ) {
                case CategoryId::Album :
                    qm->startAlbumQuery();
                    break;
                case CategoryId::Artist :
                    qm->startArtistQuery();
                    break;
                case CategoryId::Composer :
                    qm->startComposerQuery();
                    break;
                case CategoryId::Genre :
                    qm->startGenreQuery();
                    break;
                case CategoryId::Year :
                    qm->startYearQuery();
                    break;
                default : //TODO handle error condition. return tracks?
                    debug() << "Error !!?" << endl;
                    qm->startTrackQuery();
                    break;
            }
        }
        CollectionTreeItem *tmpItem = parent;
        while ( tmpItem->isDataItem()  ) {
            debug() << "add match" << endl;
            debug() << "    tmpItem->data() = " <<  tmpItem->data() << endl;
            qm->addMatch( tmpItem->data() );
            tmpItem = tmpItem->parent();
        }
        qm->returnResultAsDataPtrs( true );
        connect( qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), SLOT( newResultReady( QString, Meta::DataList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::QueuedConnection );
        d->m_childQueries.insert( qm, parent );
        qm->run();
    }
}

void
SingleCollectionTreeItemModel::updateHeaderText() {
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i ) {
        m_headerText += nameForLevel( i ) + " / ";
    }
    m_headerText.chop( 3 );
}

QString
SingleCollectionTreeItemModel::nameForLevel( int level ) const {
    switch( m_levelType[level] ) {
        case CategoryId::Album : return i18n( "Album" );
        case CategoryId::Artist : return i18n( "Artist" );
        case CategoryId::Composer : return i18n( "Composer" );
        case CategoryId::Genre : return i18n( "Genre" );
        case CategoryId::Year : return i18n( "Year" );
        default: return QString();
    }
}

QPixmap
SingleCollectionTreeItemModel::iconForLevel( int level ) const {
    QString icon;
        switch( m_levelType[level] ) {
        case CategoryId::Album :
            icon = "album";
            break;
        case CategoryId::Artist :
            icon = "artist";
            break;
        case CategoryId::Composer :
            icon = "artist";
            break;

        case CategoryId::Genre :
            icon = "kfm";
            break;

        case CategoryId::Year :
            icon = "clock";
            break;
    }
    return KIconLoader::global()->loadIcon( Amarok::icon( icon ), K3Icon::Toolbar, K3Icon::SizeSmall );
}


bool
SingleCollectionTreeItemModel::hasChildren ( const QModelIndex & parent ) const {
    DEBUG_BLOCK
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
    DEBUG_BLOCK
    if ( !parent.isValid() )
       return !m_rootItem->childrenLoaded();

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    return item->level() < m_levelType.count() && !item->childrenLoaded();
}

void
SingleCollectionTreeItemModel::fetchMore( const QModelIndex &parent ) {
    DEBUG_BLOCK
    CollectionTreeItem *item;
    if ( parent.isValid() )
        item = static_cast<CollectionTreeItem*>( parent.internalPointer() );
    else
        item = m_rootItem;
    ensureChildrenLoaded( item );
}


void
SingleCollectionTreeItemModel::queryDone() {
    DEBUG_BLOCK
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    d->m_childQueries.remove( qm );
    QTimer::singleShot( 0, qm, SLOT( deleteLater() ) );
}

void
SingleCollectionTreeItemModel::newResultReady( const QString &collectionId, Meta::DataList data ) {
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    debug() << "Received " << data.count() << " new data values" << endl;
    if ( data.count() == 0 )
        return;
    //if we are expanding an item, we'll find the sender in m_childQueries
    //otherwise we are filtering all collections
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    if ( d->m_childQueries.contains( qm ) ) {
        CollectionTreeItem *parent = d->m_childQueries.value( qm );
        QModelIndex parentIndex;
        if (parent == m_rootItem ) 
        {
           debug() << "    m_rootItem with " << parent->childCount() <<  " children" << endl;
            //the root Item *must* have an invalid index
            parentIndex = QModelIndex();
        }
        else 
        {
           debug() << "    something else with " << parent->childCount() <<  " children" << endl;
            parentIndex = createIndex( parent->row(), 0, parent );
        }
        //remove dummy row
       // beginRemoveRows( parentIndex, 0, 0 );
        //the row was never actually there, but we had to return 1 in rowCount to get the +
        //endRemoveRows();
        beginInsertRows( parentIndex, 0, data.count()-1 );
        populateChildren( data, parent ); 
        endInsertRows();
    }
}

#include "singlecollectiontreeitemmodel.moc"
