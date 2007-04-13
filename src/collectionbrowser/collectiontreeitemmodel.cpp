 /*
  Copyright (c) 2007  Alexandre Pereira de Oliveira <aleprj@gmail.com>
  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

struct CollectionTreeItemModel::Private
{
    QMap<QString, CollectionRoot > m_collections;

    QMap<QueryMaker* , CollectionTreeItem* > m_childQueries;
};

CollectionTreeItemModel::CollectionTreeItemModel( const QList<int> &levelType )
    :QAbstractItemModel()
    , m_rootItem( 0 )
    , d( new Private )
{
    CollectionManager* collMgr = CollectionManager::instance();
    connect( collMgr, SIGNAL( collectionAdded( Collection* ) ), this, SLOT( collectionAdded( Collection* ) ) );
    connect( collMgr, SIGNAL( collectionRemoved( QString ) ), this, SLOT( collectionRemoved( QString ) ) );
    setLevels( levelType );
    debug() << "Collection root has " << m_rootItem->childCount() << " childrens" << endl;
}


CollectionTreeItemModel::~CollectionTreeItemModel() {
    delete m_rootItem;
    delete d;
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

QModelIndex
CollectionTreeItemModel::index(int row, int column, const QModelIndex &parent) const
{
    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    if ( parentItem->childrenLoaded() )
    {
        CollectionTreeItem *childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        else
            return QModelIndex();
    }
    else
        return QModelIndex();
}

QModelIndex
CollectionTreeItemModel::parent(const QModelIndex &index) const
{
     if (!index.isValid())
         return QModelIndex();

     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();

     if (parentItem == m_rootItem)
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);
}

int
CollectionTreeItemModel::rowCount(const QModelIndex &parent) const
{
    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    //ensureChildrenLoaded( parentItem );
    //return parentItem->childCount();
    if ( parentItem->childrenLoaded() )
        return parentItem->childCount();
    else
        return 1; //hack!
}

int
CollectionTreeItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED( parent )
    return 1;
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
            int level = item->level();
            if ( level < m_levelType.count() )
                return iconForLevel( item->level() - 1 );
        }
    }

    return item->data( role );
}

Qt::ItemFlags
CollectionTreeItemModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() || !index.parent().isValid() )
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant
CollectionTreeItemModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        if (section == 0)
            return m_headerText;
    return QVariant();
}

QMimeData*
CollectionTreeItemModel::mimeData( const QModelIndexList &indices ) const {
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
CollectionTreeItemModel::populateChildren(const DataList &dataList, CollectionTreeItem *parent) const {
    foreach( Meta::DataPtr data, dataList ) {
        new CollectionTreeItem( data, parent );
    }
    parent->setChildrenLoaded( true );
}

void
CollectionTreeItemModel::listForLevel( int level, QueryMaker *qm, CollectionTreeItem* parent ) const {
    DEBUG_BLOCK
    if ( qm ) {
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
                    break;
            }
        }
        CollectionTreeItem *tmpItem = parent;
        while ( tmpItem->isDataItem() ) {
            qm->addMatch( tmpItem->data() );
            tmpItem = tmpItem->parent();
        }
        qm->returnResultAsDataPtrs( true );
        connect( qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), SLOT( newResultReady( QString, Meta::DataList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::QueuedConnection );
        debug() << "Running querymaker" << endl;
        d->m_childQueries.insert( qm, parent );
        qm->run();
    }
}

void
CollectionTreeItemModel::updateHeaderText() {
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i ) {
        m_headerText += nameForLevel( i ) + " / ";
    }
    m_headerText.chop( 3 );
}

QString
CollectionTreeItemModel::nameForLevel( int level ) const {
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
CollectionTreeItemModel::iconForLevel( int level ) const {
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
        return false;
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
CollectionTreeItemModel::collectionRemoved( QString collectionId ) {
    d->m_collections.remove( collectionId );
}

void
CollectionTreeItemModel::queryDone() {
    DEBUG_BLOCK
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    d->m_childQueries.remove( qm );
    QTimer::singleShot( 0, qm, SLOT( deleteLater() ) );
}

void
CollectionTreeItemModel::newResultReady( QString collectionId, Meta::DataList data ) {
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    debug() << "Received " << data.count() << " new data values" << endl;
    //if we are expanding an item, we'll find the sender in m_childQueries
    //otherwise we are filtering all collections
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    if ( d->m_childQueries.contains( qm ) ) {
        CollectionTreeItem *parent = d->m_childQueries.value( qm );
        QModelIndex parentIndex = createIndex( parent->row(), 0, parent );
        //remove dummy row
        beginRemoveRows( parentIndex, 0, 0 );
        //the row was never actually there, but we had to return 1 in rowCount to get the +
        endRemoveRows();
        beginInsertRows( parentIndex, 0, data.count() -1 );
        populateChildren( data, parent );
        /*int rowCount = parent->childCount();
        QModelIndex topLeft = createIndex( 0, 0, parent->child( 0 ) );
        QModelIndex bottomRight = createIndex( rowCount - 1, 0, parent->child( rowCount -1 ) );
        emit dataChanged( topLeft, bottomRight );*/
        //emit dataChanged( parentIndex, parentIndex );
        endInsertRows();
    }
}

#include "collectiontreeitemmodel.moc"
