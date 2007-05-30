/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007  Ian Alexander Monroe <ian@monroe.nu>                  *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#include "collectiontreeview.h"

#include "amarok.h"
#include "collectionbrowser/collectiontreeitemmodel.h"
#include "playlist/PlaylistModel.h"

#include <QContextMenuEvent>

#include <kconfig.h>
#include <KIcon>
#include <KMenu>
#include <KSharedPtr>


CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
{
    KConfigGroup config = Amarok::config( "Collection Browser" );
    QList<int> cats = config.readEntry( "TreeCategory", QList<int>() );
    if ( cats.isEmpty() )
        cats << QueryBuilder::tabArtist << QueryBuilder::tabAlbum;

    m_treeModel = new CollectionTreeItemModel( cats );
    setModel( m_treeModel );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setDragDropMode( QAbstractItemView::DragOnly ); // implement drop when time allows

    //setAnimated( true );
    setAlternatingRowColors( true );
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
        menu.addAction( appendAction );
        QAction* result =  menu.exec( mapToGlobal( event->pos() ) );
        if( result == appendAction )
        {
                debug() << "row# " << item->row() << endl;
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
                    PlaylistNS::Model::instance()->insertTracks( PlaylistNS::Model::instance()->rowCount(), qm );
                }
                else
                {
                    QList< Meta::TrackPtr > tracks = item->descendentTracks();
                    PlaylistNS::Model::instance()->insertTracks( PlaylistNS::Model::instance()->rowCount(),
                        tracks );
                }
        }
    }
    else
        debug() << "invalid index or null internalPointer" << endl;
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

#include "collectiontreeview.moc"
