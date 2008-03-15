/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007  Ian Alexander Monroe <ian@monroe.nu>                  *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#include "CollectionTreeView.h"

#include "amarok.h"
#include "debug.h"
#include "collectionbrowser/CollectionTreeItemModel.h"
#include "collectionbrowser/OrganizeCollectionDialog.h"
#include "context/ContextView.h"
#include "mediabrowser.h"
#include "Meta.h"
#include "MetaQueryBuilder.h"
#include "meta/CustomActionsCapability.h"
#include "playlist/PlaylistModel.h"
#include "QueryMaker.h"
#include "tagdialog.h"
#include "TheInstances.h"

#include <QContextMenuEvent>
#include <QSet>

#include <kconfig.h>
#include <KIcon>
#include <KLineEdit>
#include <KMenu>
#include <KSharedPtr>

CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
    , m_dragStartPosition()
{
    setRootIsDecorated( true );
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setDragDropMode( QAbstractItemView::DragOnly ); // implement drop when time allows

    setAnimated( true );
    setAlternatingRowColors( true );

    m_treeModel = 0;
    m_filterModel = 0;

    connect( this, SIGNAL( collapsed( const QModelIndex & ) ), SLOT( slotCollapsed( const QModelIndex & ) ) );

   
}


void CollectionTreeView::setModel(QAbstractItemModel * model)
{
    m_treeModel = static_cast<CollectionTreeItemModelBase *> ( model );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), m_treeModel, SLOT( slotFilter() ) );
    connect( m_treeModel, SIGNAL( expandIndex( const QModelIndex ) ), SLOT( slotExpand( const QModelIndex ) ) );

    m_filterModel = new CollectionSortFilterProxyModel( this );
    m_filterModel->setSortRole( CustomRoles::SortRole );
    m_filterModel->setFilterRole( CustomRoles::FilterRole );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setSourceModel( model );

    QTreeView::setModel( m_filterModel );
    //QTreeView::setModel( model );

}



CollectionTreeView::~CollectionTreeView() {

    //we dont know what collection this is as this class is used with many different collections...
    //KConfigGroup config = Amarok::config( "Collection Browser" );
    //config.writeEntry( "TreeCategory", m_treeModel->levels() );
    delete m_treeModel;
    delete m_filterModel;
}

void
CollectionTreeView::setLevels( const QList<int> &levels ) {
    m_treeModel->setLevels( levels );
}


void
CollectionTreeView::setLevel( int level, int type ) {
    QList<int> levels = m_treeModel->levels();
    if ( type == CategoryId::None ) {
        while( levels.count() >= level )
            levels.removeLast();
    }
    else {
        levels.removeAll( type );
        levels[level] = type;
    }
    setLevels( levels );
}

QSortFilterProxyModel*
CollectionTreeView::filterModel() const
{
    return m_filterModel;
}


void
CollectionTreeView::contextMenuEvent(QContextMenuEvent* event)
{

    QModelIndexList indices = selectedIndexes();
    if( m_filterModel )
    {
        QModelIndexList tmp;
        foreach( const QModelIndex &idx, indices )
        {
            tmp.append( m_filterModel->mapToSource( idx ) );
        }
        indices = tmp;
    }

    //if( index.isValid() && index.internalPointer()  )
    if( !indices.isEmpty() )
    {
        //CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

        KMenu menu;
        QAction* loadAction = new QAction( KIcon("file_open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), &menu );
        QAction* appendAction = new QAction( KIcon( "list-add-amarok" ), i18n( "&Append to Playlist" ), &menu);
        QAction* editAction = new QAction( i18n( "Edit Track Information" ), &menu );
        QAction* organizeAction = new QAction( i18n( "Organize Files" ), &menu );
        menu.addAction( loadAction );
        menu.addAction( appendAction );
        menu.addSeparator();
        menu.addAction( editAction );
        menu.addAction( organizeAction );

        if( indices.count() == 1 )
        {
            if( indices.first().isValid() && indices.first().internalPointer() )
            {
                Meta::DataPtr data = static_cast<CollectionTreeItem*>( indices.first().internalPointer() )->data();
                if( data )
                {
                    Meta::CustomActionsCapability *cac = data->as<Meta::CustomActionsCapability>();
                    if( cac )
                    {
                        QList<QAction*> actions = cac->customActions();
                        if( actions.count() )
                            menu.addSeparator();
                        foreach( QAction *action, actions )
                            menu.addAction( action );
                        delete cac;
                    }
                }
            }
        }
        QAction* result =  menu.exec( event->globalPos() );
        QSet<CollectionTreeItem*> items;
        foreach( const QModelIndex &index, indices )
        {
            if( index.isValid() && index.internalPointer() )
                items.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
        }
        if( result == loadAction )
            playChildTracks( items, Playlist::Replace );
        else if( result == appendAction )
            playChildTracks( items, Playlist::Append );
        else if( result == editAction )
        {
            editTracks( items );
        }
        else if( result == organizeAction )
        {
            organizeTracks( items );
        }
    }
    else
        debug() << "invalid index or null internalPointer";
}

void CollectionTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index;
    if( m_filterModel )
        index = m_filterModel->mapToSource( indexAt( event->pos() ) );
    else
        index = indexAt( event->pos() );

    if( index.isValid() && index.internalPointer()  /*&& index.parent().isValid()*/ )
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

        playChildTracks( item, Playlist::Append );
    }
}

void CollectionTreeView::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton )
        m_dragStartPosition = e->pos();

    QTreeView::mousePressEvent( e );
}

void CollectionTreeView::mouseMoveEvent( QMouseEvent *e )
{
    if( !( e->buttons() & Qt::LeftButton ) )
        return;
    if( ( e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance() )
        return;

    // TODO port....
    //ContextView::instance()->showPopupDropper();

    QTreeView::mouseMoveEvent( e );
}


void CollectionTreeView::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    Q_UNUSED( deselected )
    QModelIndexList indexes = selected.indexes();
    if ( indexes.count() < 1 )
        return;

    QModelIndex index;
    if ( m_filterModel )
        index = m_filterModel->mapToSource( indexes[0] );
    else
        index = indexes[0];


    CollectionTreeItem * item = static_cast<CollectionTreeItem *>( index.internalPointer() );

    emit( itemSelected ( item ) );

}

void
CollectionTreeView::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_treeModel->setCurrentFilter( lineEdit->text() );
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void
CollectionTreeView::slotExpand( const QModelIndex &index )
{
    if( m_filterModel )
        expand( m_filterModel->mapFromSource( index ) );
    else
        expand( index );
}

void
CollectionTreeView::slotCollapsed( const QModelIndex &index )
{
    if( m_filterModel )
        m_treeModel->slotCollapsed( m_filterModel->mapToSource( index ) );
    else
        m_treeModel->slotCollapsed( index );
}

void
CollectionTreeView::playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode) const
{
    DEBUG_BLOCK
    if( !item->allDescendentTracksLoaded() )
    {
        QueryMaker *qm = item->queryMaker();
        CollectionTreeItem *tmp = item;
        while( tmp->isDataItem() )
        {
            if ( tmp->data() )
                qm->addMatch( tmp->data() );
            else
                qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
            tmp = tmp->parent();
        }
        m_treeModel->addFilters( qm );
        The::playlistModel()->insertOptioned( qm, insertMode );
    }
    else
    {
        Meta::TrackList tracks = item->descendentTracks();
        qStableSort( tracks.begin(), tracks.end(), Amarok::trackNumberLessThan);
        The::playlistModel()->insertOptioned( tracks, insertMode );
    }
}

void
CollectionTreeView::playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode ) const
{
    //find all selected parents in the list and ignore the rest
    QSet<CollectionTreeItem*> parents;
    foreach( CollectionTreeItem *item, items )
    {
        CollectionTreeItem *tmpItem = item;
        while( tmpItem )
        {
            if( items.contains( tmpItem->parent() ) )
            {
                tmpItem = tmpItem->parent();
            }
            else
            {
                parents.insert( tmpItem );
                break;
            }
        }
    }
    bool first = true;
    foreach( CollectionTreeItem *item, parents )
    {
        //FIXME:we are ignoring the order of the sleected items
        playChildTracks( item, first ? insertMode : Playlist::Append );
        first = false;
    }
}

void
CollectionTreeView::organizeTracks( const QSet<CollectionTreeItem*> &items ) const
{
    //find all selected parents in the list and ignore the rest
    QSet<CollectionTreeItem*> parents;
    foreach( CollectionTreeItem *item, items )
    {
        CollectionTreeItem *tmpItem = item;
        while( tmpItem )
        {
            if( items.contains( tmpItem->parent() ) )
            {
                tmpItem = tmpItem->parent();
            }
            else
            {
                parents.insert( tmpItem );
                break;
            }
        }
    }
    QList<QueryMaker*> queryMakers;
    foreach( CollectionTreeItem *item, parents )
    {
        QueryMaker *qm = item->queryMaker();
        CollectionTreeItem *tmp = item;
        while( tmp->isDataItem() )
        {
            if ( tmp->data() )
                qm->addMatch( tmp->data() );
            else
                qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
            tmp = tmp->parent();
        }
        m_treeModel->addFilters( qm );
        queryMakers.append( qm );
    }
    QueryMaker *qm = new MetaQueryBuilder( queryMakers );
    OrganizeCollectionDialog *dialog = new OrganizeCollectionDialog( qm ); //the dialog will show itself automatically as soon as it is ready
}

void
CollectionTreeView::editTracks( const QSet<CollectionTreeItem*> &items ) const
{
    //find all selected parents in the list and ignore the rest
    QSet<CollectionTreeItem*> parents;
    foreach( CollectionTreeItem *item, items )
    {
        CollectionTreeItem *tmpItem = item;
        while( tmpItem )
        {
            if( items.contains( tmpItem->parent() ) )
            {
                tmpItem = tmpItem->parent();
            }
            else
            {
                parents.insert( tmpItem );
                break;
            }
        }
    }
    QList<QueryMaker*> queryMakers;
    foreach( CollectionTreeItem *item, parents )
    {
        QueryMaker *qm = item->queryMaker();
        CollectionTreeItem *tmp = item;
        while( tmp->isDataItem() )
        {
            if ( tmp->data() )
                qm->addMatch( tmp->data() );
            else
                qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
            tmp = tmp->parent();
        }
        m_treeModel->addFilters( qm );
        queryMakers.append( qm );
    }
    QueryMaker *qm = new MetaQueryBuilder( queryMakers );
    TagDialog *dialog = new TagDialog( qm ); //the dialog will show itself automatically as soon as it is ready
}

#include "CollectionTreeView.moc"
