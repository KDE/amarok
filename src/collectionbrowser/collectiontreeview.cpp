/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007  Ian Alexander Monroe <ian@monroe.nu>                  *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#include "collectiontreeview.h"

#include "amarok.h"
#include "debug.h"
#include "collectionbrowser/collectiontreeitemmodel.h"
#include "context/ContextView.h"
#include "TheInstances.h"

#include <QContextMenuEvent>

#include <kconfig.h>
#include <KIcon>
#include <KLineEdit>
#include <KMenu>
#include <KSharedPtr>

CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
    , m_dragStartPosition()
{
    KConfigGroup config = Amarok::config( "Collection Browser" );
    QList<int> cats = config.readEntry( "TreeCategory", QList<int>() );
    if ( cats.isEmpty() )
        cats << QueryBuilder::tabArtist << QueryBuilder::tabAlbum;

    m_treeModel = new CollectionTreeItemModel( cats );
    setModel( m_treeModel );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setDragDropMode( QAbstractItemView::DragOnly ); // implement drop when time allows

    //setAnimated( true );
    setAlternatingRowColors( true );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), m_treeModel, SLOT( slotFilter() ) );
    connect( this, SIGNAL( collapsed( const QModelIndex ) ), SLOT( slotCollapsed( const QModelIndex ) ) );

    connect( m_treeModel, SIGNAL( expandIndex( const QModelIndex ) ), SLOT( slotExpand( const QModelIndex ) ) );
}


void CollectionTreeView::setModel(QAbstractItemModel * model)
{
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
    KConfigGroup config = Amarok::config( "Collection Browser" );
    config.writeEntry( "TreeCategory", m_treeModel->levels() );
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

    QModelIndex index;
    if( m_filterModel )
        index = m_filterModel->mapToSource( indexAt( event->pos() ) );
    else
        index = indexAt( event->pos() );

    if( index.isValid() && index.internalPointer()  )
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

        KMenu menu;
        QAction* appendAction = new QAction( KIcon( Amarok::icon( "add_playlist") ), i18n( "&Append to Playlist" ), &menu);
        QAction* loadAction = new QAction( KIcon(Amarok::icon( "file_open" ) ), i18n( "&Load" ), &menu );
        menu.addAction( loadAction );
        menu.addAction( appendAction );
        QAction* result =  menu.exec( mapToGlobal( event->pos() ) );
        if( result == loadAction )
            playChildTracks( item, Playlist::Replace );
        else if( result == appendAction )
            playChildTracks( item, Playlist::Append );
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

    if( index.isValid() && index.internalPointer()  )
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
CollectionTreeView::playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode)
{
    if( !item->allDescendentTracksLoaded() )
    {
        QueryMaker *qm = item->queryMaker();
        CollectionTreeItem *tmp = item;
        while( tmp->isDataItem() )
        {
            qm->addMatch( tmp->data() );
            tmp = tmp->parent();
        }
        m_treeModel->addFilters( qm );
        The::playlistModel()->insertOptioned( qm, insertMode );
    }
    else
    {
        Meta::TrackList tracks = item->descendentTracks();
        The::playlistModel()->insertOptioned( tracks, insertMode );
    }
}

#include "collectiontreeview.moc"
