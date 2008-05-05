/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (c) 2007 Ian Alexander Monroe <ian@monroe.nu>                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License or (at your option) version 3 or any later version             *
 * accepted by the membership of KDE e.V. (or its successor approved          *
 * by the membership of KDE e.V.), which shall act as a proxy                 *
 * defined in Section 14 of version 3 of the license.                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
 
#include "CollectionTreeView.h"

#include "Amarok.h"
#include "debug.h"
#include "CollectionLocation.h"
#include "CollectionManager.h"
#include "collectionbrowser/CollectionTreeItemModel.h"
#include "context/ContextView.h"
#include "mediabrowser.h"
#include "Meta.h"
#include "MetaQueryMaker.h"
#include "meta/CustomActionsCapability.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistGraphicsView.h"
#include "popupdropper/PopupDropper.h"
#include "popupdropper/PopupDropperAction.h"
#include "popupdropper/PopupDropperItem.h"
#include "QueryMaker.h"
#include "SvgHandler.h"
#include "tagdialog.h"
#include "TheInstances.h"

#include <QContextMenuEvent>
#include <QHash>
#include <QSet>

#include <kconfig.h>
#include <KIcon>
#include <KLineEdit>
#include <KMenu>
#include <KSharedPtr>

CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
    , m_dragStartPosition()
    , m_pd( 0 )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_editAction( 0 )
    , m_organizeAction( 0 )
    , m_caSeperator( 0 )
    , m_cmSeperator( 0 )
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

    m_filterModel = new CollectionSortFilterProxyModel( this );
    m_filterModel->setSortRole( CustomRoles::SortRole );
    m_filterModel->setFilterRole( CustomRoles::FilterRole );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setSourceModel( model );

    QTreeView::setModel( m_filterModel );
//     QTreeView::setModel( model );

    connect( m_treeModel, SIGNAL( expandIndex( const QModelIndex & ) ), SLOT( slotExpand( const QModelIndex & ) ) );
}



CollectionTreeView::~CollectionTreeView() {

    //we don't know what collection this is as this class is used with many different collections...
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

    PopupDropperActionList actions = getActions( indices );

    KMenu menu;

    foreach( PopupDropperAction * action, actions ) {
        debug() << "adding: " << action->text();
        menu.addAction( action );
    }
    
    QAction *organizeAction = 0;
    if( !indices.isEmpty() )
    {
        {   //keep the scope of item minimal
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
            while( item->isDataItem() )
            {
                item = item->parent();
            }
            Collection *collection = item->parentCollection();
            
            if( collection->isOrganizable() && onlyOneCollection( indices) )
            {
                organizeAction = new QAction( i18n( "Organize Files" ), &menu );
                menu.addAction( organizeAction );
            }


        }
    }


    QHash<PopupDropperAction*, Collection*> copyDestination = getCopyActions( indices );
    QHash<PopupDropperAction*, Collection*> moveDestination = getMoveActions( indices );

    if ( !( copyDestination.empty() && moveDestination.empty() ) ) {
        if ( m_cmSeperator == 0 ) {
            m_cmSeperator = new PopupDropperAction( this );
            m_cmSeperator->setSeparator ( true );
        }
        menu.addAction( m_cmSeperator );
    }

    
    if ( !copyDestination.empty() ) {
        debug() << "got copy actions";
        KMenu *copyMenu = new KMenu( i18n( "Copy to Collection" ), &menu );
        foreach( PopupDropperAction * action, copyDestination.keys() ) {
            action->setParent( copyMenu );
            copyMenu->addAction( action );
        }
        menu.addMenu( copyMenu );
    }

    if ( !moveDestination.empty() ) {
        debug() << "got move actions";
        KMenu *moveMenu = new KMenu( i18n( "Move to Collection" ), &menu );
        foreach( PopupDropperAction * action, copyDestination.keys() ) {
            action->setParent( moveMenu );
            moveMenu->addAction( action );
        }
        menu.addMenu( moveMenu );
    }

    
    PopupDropperAction* result = dynamic_cast< PopupDropperAction* > ( menu.exec( event->globalPos() ) );
    if ( result == 0 ) return;
    
    QSet<CollectionTreeItem*> items;
    foreach( const QModelIndex &index, indices )
    {
        if( index.isValid() && index.internalPointer() )
            items.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
    }
    if( result == dynamic_cast<QAction*>(m_loadAction) )
        playChildTracks( items, Playlist::Replace );
    else if( result == dynamic_cast<QAction*>(m_appendAction) )
        playChildTracks( items, Playlist::Append );
    else if( result == m_editAction )
    {
        editTracks( items );
    }
    else if( result == organizeAction )
    {
        organizeTracks( items );
    }
    else if( copyDestination.contains( result ) )
    {
        copyTracks( items, copyDestination[ result ], false );
    }
    else if( moveDestination.contains( result ) )
    {
        copyTracks( items, moveDestination[ result ], true );
    }

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

        playChildTracks( item, Playlist::AppendAndPlay );
    }
}

void CollectionTreeView::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton )
        m_dragStartPosition = e->pos();

    QTreeView::mousePressEvent( e );
}

void
CollectionTreeView::startDrag(Qt::DropActions supportedActions)
{
    DEBUG_BLOCK

    //Waah? when a parent item is dragged, startDrag is called a bunch of times
    static bool ongoingDrags = false;
    if( ongoingDrags )
        return;
    ongoingDrags = true;

    if( !m_pd )
        m_pd = createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isValid() && m_pd->isHidden() )
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

        PopupDropperActionList actions = getActions( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        foreach( PopupDropperAction * action, actions ) {
            
            PopupDropperItem* pdi = new PopupDropperItem();
            pdi->setAction( action );
            pdi->setFont( font );
            m_pd->addItem( pdi, false );
        }

       /* QHash<PopupDropperAction*, Collection*> copyDestination = getCopyActions( indices );
        QHash<PopupDropperAction*, Collection*> moveDestination = getMoveActions( indices );

        if ( !( copyDestination.empty() && moveDestination.empty() ) ) {
        if ( m_cmSeperator == 0 ) {
        m_cmSeperator = new PopupDropperAction( this );
        m_cmSeperator->setSeparator ( true );
    }
        PopupDropperItem* pdi = new PopupDropperItem();
        pdi->setAction( m_cmSeperator );
        pdi->setFont( font );
        m_pd->addItem( pdi, false );
    }

    
        if ( !copyDestination.empty() ) {
        debug() << "got copy actions";
        KMenu *copyMenu = new KMenu( i18n( "Copy to Collection" ), &menu );
        foreach( PopupDropperAction * action, copyDestination.keys() ) {
        action->setParent( copyMenu );
                
        PopupDropperItem* pdi = new PopupDropperItem();
        pdi->setAction( m_cmSeperator );
        pdi->setFont( font );
        m_pd->addItem( pdi, false );
    }
        menu.addMenu( copyMenu );
    }

        if ( !moveDestination.empty() ) {
        debug() << "got move actions";
        KMenu *moveMenu = new KMenu( i18n( "Move to Collection" ), &menu );
        foreach( PopupDropperAction * action, copyDestination.keys() ) {
        action->setParent( moveMenu );
        moveMenu->addAction( action );
    }
        menu.addMenu( moveMenu );
    }*/

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        m_pd->clear();
    }
    ongoingDrags = false;
}

PopupDropper* CollectionTreeView::createPopupDropper( QWidget *parent )
{
    DEBUG_BLOCK
    PopupDropper* pd = new PopupDropper( parent );
    if( !pd )
        return 0;

    pd->setQuitOnDragLeave( true );
    pd->setFadeInTime( 500 );
    pd->setFadeOutTime( 300 );
    QColor windowColor( Qt::black );
    windowColor.setAlpha( 128 );
    QColor textColor( Qt::white );
    pd->setColors( windowColor, textColor );

    return pd;
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
    if( !items.count() )
    {
        return;
    }
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
    QueryMaker *qm = new MetaQueryMaker( queryMakers );
    CollectionTreeItem *item = items.toList().first();
    while( item->isDataItem() )
    {
        item = item->parent();
    }
    Collection *coll = item->parentCollection();
    CollectionLocation *location = coll->location();
    if( !location->isOrganizable() )
    {
        //how did we get here??
        delete location;
        delete qm;
        return;
    }
    location->prepareMove( qm, coll->location() );
}

void
CollectionTreeView::copyTracks( const QSet<CollectionTreeItem*> &items, Collection *destination, bool removeSources ) const
{
    if( !destination->isWritable() )
    {
        return;
    }
    //copied from organizeTracks. create a method for this somewhere
    if( !items.count() )
    {
        return;
    }
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
    QueryMaker *qm = new MetaQueryMaker( queryMakers );
    CollectionTreeItem *item = items.toList().first();
    while( item->isDataItem() )
    {
        item = item->parent();
    }
    Collection *coll = item->parentCollection();
    CollectionLocation *source = coll->location();
    CollectionLocation *dest = destination->location();
    if( removeSources )
    {
        if( !source->isWriteable() ) //error
        {
            delete dest;
            delete source;
            delete qm;
        }
        source->prepareMove( qm, dest );
    }
    else
    {
        source->prepareCopy( qm, dest );
    }
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
    QueryMaker *qm = new MetaQueryMaker( queryMakers );
    (void)new TagDialog( qm ); //the dialog will show itself automatically as soon as it is ready
}

void CollectionTreeView::slotFilterNow()
{
    m_treeModel->slotFilter();
}

PopupDropperActionList CollectionTreeView::getActions( const QModelIndexList & indices )
{
    PopupDropperActionList actions;
    
    if( !indices.isEmpty() )
    {

        if ( m_appendAction == 0 )
            m_appendAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "append_playlist_xlarge", KIcon( "list-add-amarok" ), i18n( "&Append to Playlist" ), this );

        actions.append( m_appendAction );

        if ( m_loadAction == 0 )
            m_loadAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "load_playlist_xlarge", KIcon("file_open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );

        actions.append( m_loadAction );

        if ( m_editAction == 0 )
            m_editAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "load_playlist_xlarge", KIcon("file_open" ), i18n( "&Edit Track Information" ), this );

        actions.append( m_editAction );

        {   //keep the scope of item minimal
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
            while( item->isDataItem() )
            {
                item = item->parent();
            }
            Collection *collection = item->parentCollection();
            if( collection->location()->isOrganizable() )
            {
                bool onlyOneCollection = true;
                foreach( const QModelIndex &index, indices )
                {
                    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
                    while( item->isDataItem() )
                    {
                        item = item->parent();
                    }
                    onlyOneCollection = item->parentCollection() == collection;
                    if( !onlyOneCollection )
                        break;
                }

                if( onlyOneCollection )
                {
                    if ( m_organizeAction == 0 )
                        m_organizeAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "load_playlist_xlarge", KIcon("file_open" ), i18nc( "Organize Files", "Organize Files" ), this );
                    actions.append( m_organizeAction );
                }
            }
        }

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
                        if ( m_caSeperator == 0 ) {
                            m_caSeperator = new PopupDropperAction( this );
                            m_caSeperator->setSeparator ( true );
                        }
                        actions.append( m_caSeperator );
                        
                        PopupDropperActionList cActions = cac->customActions();
                        
                        foreach( PopupDropperAction *action, cActions ) {

                            actions.append( action );
                            debug() << "Got custom action: " << action->text();
                        }
                        delete cac;
                    }
                }
            }
        }
    }

    else
        debug() << "invalid index or null internalPointer";

    return actions;
    
}


QHash<PopupDropperAction*, Collection*> CollectionTreeView::getCopyActions(const QModelIndexList & indices )
{
    DEBUG_BLOCK
    QHash<PopupDropperAction*, Collection*> copyDestination;
    if( !indices.isEmpty() )
    {

        if( onlyOneCollection( indices) )
        {
            Collection *collection = getCollection( indices );
            QList<Collection*> writableCollections;
            foreach( Collection *coll, CollectionManager::instance()->collections() )
            {
                if( coll->isWritable() && coll != collection )
                {
                    debug() << "got writable collection";
                    writableCollections.append( coll );
                }
            }
            if( !writableCollections.isEmpty() )
            {
                foreach( Collection *coll, writableCollections )
                {
                    debug() << "creating action";
                    PopupDropperAction *action = new PopupDropperAction( QIcon(), coll->prettyName(), 0 );
                    copyDestination.insert( action, coll );
                }

            }
        }
    }
    debug() << "returning " << copyDestination.size() << " actions.";
    return copyDestination;
}

QHash<PopupDropperAction*, Collection*> CollectionTreeView::getMoveActions( const QModelIndexList & indices )
{
    DEBUG_BLOCK
    QHash<PopupDropperAction*, Collection*> moveDestination;
    if( !indices.isEmpty() )
    {
        if( onlyOneCollection( indices) )
        {
            Collection *collection = getCollection( indices );
            QList<Collection*> writableCollections;
            foreach( Collection *coll, CollectionManager::instance()->collections() )
            {
                if( coll->isWritable() && coll != collection )
                {
                    writableCollections.append( coll );
                }
            }
            if( !writableCollections.isEmpty() )
            {
                if( collection->isWritable() )
                {
                    foreach( Collection *coll, writableCollections )
                    {
                        PopupDropperAction *action = new PopupDropperAction( QIcon(), coll->prettyName(), 0 );
                        moveDestination.insert( action, coll );
                    }
                }
            }
        }
    }
    debug() << "returning " << moveDestination.size() << " actions.";
    return moveDestination;
}

bool CollectionTreeView::onlyOneCollection( const QModelIndexList & indices )
{
    DEBUG_BLOCK
    bool onlyOneCollection = true;
    if( !indices.isEmpty() )
    {
        Collection *collection = getCollection( indices );
        foreach( const QModelIndex &index, indices )
        {
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
            while( item->isDataItem() )
            {
                item = item->parent();
            }
            onlyOneCollection = item->parentCollection() == collection;
            if( !onlyOneCollection )
                break;
        }
    }

    return onlyOneCollection;
}

Collection * CollectionTreeView::getCollection( const QModelIndexList & indices )
{
    DEBUG_BLOCK
    Collection *collection = 0;
    if( !indices.isEmpty() )
    {

        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
        while( item->isDataItem() )
        {
            item = item->parent();
        }
        collection = item->parentCollection();
    }

    return collection;
}

#include "CollectionTreeView.moc"

