/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/




#define DEBUG_PREFIX "CollectionTreeItemModelBase"

#include "CollectionTreeItemModelBase.h"

#include "amarok.h"
#include "collection.h"
#include "collectionmanager.h"
#include "collectiontreeitem.h"
#include "debug.h"
#include "querymaker.h"

#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KStandardDirs>
#include <QMimeData>
#include <QPixmap>
#include <QTimer>


CollectionTreeItemModelBase::CollectionTreeItemModelBase( )
    :QAbstractItemModel()
    , m_rootItem( 0 )
    , d( new Private )
    , m_animFrame( 0 )
    , m_loading1( QPixmap( KStandardDirs::locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( QPixmap( KStandardDirs::locate("data", "amarok/images/loading2.png" ) ) )
    , m_currentAnimPixmap( m_loading1 )
{


    m_timeLine = new QTimeLine( 10000, this );
    m_timeLine->setFrameRange( 0, 20 );
    m_timeLine->setLoopCount ( 0 );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( loadingAnimationTick() ) );


}


CollectionTreeItemModelBase::~CollectionTreeItemModelBase()
{
    delete m_rootItem;
    delete d;
}

Qt::ItemFlags CollectionTreeItemModelBase::flags(const QModelIndex & index) const
{
    //DEBUG_BLOCK
    if ( !index.isValid() || !index.parent().isValid() )
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant 
CollectionTreeItemModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        if (section == 0)
            return m_headerText;
    return QVariant();
}

QModelIndex 
CollectionTreeItemModelBase::index(int row, int column, const QModelIndex & parent) const
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
    //   return QModelIndex();
}

QModelIndex 
CollectionTreeItemModelBase::parent(const QModelIndex & index) const
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
CollectionTreeItemModelBase::rowCount(const QModelIndex & parent) const
{

    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    if ( parentItem->childrenLoaded() )
        return parentItem->childCount();
    else
        return 0;

}

int CollectionTreeItemModelBase::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED( parent )
    return 1;
}

QMimeData * CollectionTreeItemModelBase::mimeData(const QModelIndexList & indices) const
{
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

QPixmap 
CollectionTreeItemModelBase::iconForLevel(int level) const
{
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

void CollectionTreeItemModelBase::listForLevel(int level, QueryMaker * qm, CollectionTreeItem * parent) const
{
    //DEBUG_BLOCK
    if ( qm && parent ) {

        //this check should not hurt anyone... needs to check if single... needs it
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
                    break;
            }
        }
        CollectionTreeItem *tmpItem = parent;
        while ( tmpItem->isDataItem()  ) {
            qm->addMatch( tmpItem->data() );
            tmpItem = tmpItem->parent();
        }
        addFilters( qm );
        qm->returnResultAsDataPtrs( true );
        connect( qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), SLOT( newResultReady( QString, Meta::DataList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::QueuedConnection );
        d->m_childQueries.insert( qm, parent );
        qm->run();

       //start animation
       if ( ( m_timeLine->state() != QTimeLine::Running ) && ( parent != m_rootItem ) ) 
           m_timeLine->start();


    }


}


void 
CollectionTreeItemModelBase::addFilters(QueryMaker * qm) const
{
    DEBUG_BLOCK
    //filter string hardcoded for testing purposes
    foreach( int level, m_levelType ) {
        qint64 value;
        switch( level ) {
            case CategoryId::Album:
                value = QueryMaker::valAlbum;
                break;
            case CategoryId::Artist:
                value = QueryMaker::valArtist;
                break;
            case CategoryId::Composer:
                value = QueryMaker::valComposer;
                break;
            case CategoryId::Genre:
                value = QueryMaker::valGenre;
                break;
            case CategoryId::Year:
                value = QueryMaker::valYear;
                break;
            default:
                value = -1;
                break;
        }
        //qm->addFilter( value, "Hero", false, false );
    }
    //qm->addFilter( QueryMaker::valTitle, "Hero", false, false ); //always filter for track title too
}

void 
CollectionTreeItemModelBase::queryDone()
{
    //DEBUG_BLOCK
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    CollectionTreeItem* item = d->m_childQueries.take( qm );

    //reset icon for this item
    if ( item != m_rootItem )
        emit ( dataChanged ( createIndex(item->row(), 0, item), createIndex(item->row(), 0, item) ) );

    //stop timer if there are no more animations active
    if (d->m_childQueries.count() == 0 )
        m_timeLine->stop();

    QTimer::singleShot( 0, qm, SLOT( deleteLater() ) );
}

void 
CollectionTreeItemModelBase::newResultReady(const QString & collectionId, Meta::DataList data)
{
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
        if (parent == m_rootItem ) // will never happen in CollectionTreeItemModel
        {
            parentIndex = QModelIndex();
        }
        else 
        {
            parentIndex = createIndex( parent->row(), 0, parent );
        }

        beginInsertRows( parentIndex, 0, data.count()-1 );
        populateChildren( data, parent ); 
        endInsertRows();
    }
}

void 
CollectionTreeItemModelBase::populateChildren(const DataList & dataList, CollectionTreeItem * parent) const
{
    foreach( Meta::DataPtr data, dataList ) {
        new CollectionTreeItem( data, parent );
    }
    parent->setChildrenLoaded( true );
}

void 
CollectionTreeItemModelBase::updateHeaderText()
{
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i ) {
        m_headerText += nameForLevel( i ) + " / ";
    }
    m_headerText.chop( 3 );
}

QString 
CollectionTreeItemModelBase::nameForLevel(int level) const
{
    switch( m_levelType[level] ) {
        case CategoryId::Album : return i18n( "Album" );
        case CategoryId::Artist : return i18n( "Artist" );
        case CategoryId::Composer : return i18n( "Composer" );
        case CategoryId::Genre : return i18n( "Genre" );
        case CategoryId::Year : return i18n( "Year" );
        default: return QString();
    }
}

void CollectionTreeItemModelBase::loadingAnimationTick()
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


#include "CollectionTreeItemModelBase.moc"






