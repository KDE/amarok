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
#include "AmarokMimeData.h"
#include "Debug.h"
#include "CollectionLocation.h"
#include "CollectionManager.h"
#include "browsers/collectionbrowser/CollectionTreeItemModel.h"
#include "context/ContextView.h"
#include "GlobalCollectionActions.h"
#include "Meta.h"
#include "MetaQueryMaker.h"
#include "meta/CollectionCapability.h"
#include "meta/CustomActionsCapability.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistController.h"
#include "PopupDropperFactory.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "QueryMaker.h"
#include "SvgHandler.h"
#include "TagDialog.h"

#include <QContextMenuEvent>
#include <QHash>
#include <QSet>

#include <KAction>
#include <KIcon>
#include <KLineEdit>
#include <KMenu>
#include <KMessageBox> // NOTE: for delete dialog, will move to CollectionCapability later
#include <KSharedPtr>

CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
    , m_filterModel( 0 )
    , m_treeModel( 0 )
    , m_pd( 0 )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_editAction( 0 )
    , m_organizeAction( 0 )
    , m_caSeperator( 0 )
    , m_cmSeperator( 0 )
    , m_dragMutex()
    , m_ongoingDrag( false )
{
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible

    setDragDropMode( QAbstractItemView::DragOnly ); // implement drop when time allows

    //setAnimated( true );

    //even though we paint the rows ourselves, we need this to be true
    //for the option to tell us about it
    setAlternatingRowColors( true );

    The::paletteHandler()->updateItemView( this );
    
    setStyleSheet("QTreeView::item { margin-top: 1px; margin-bottom: 1px; }"); //ensure a bit of space around the cover icons



    connect( this, SIGNAL( collapsed( const QModelIndex & ) ), SLOT( slotCollapsed( const QModelIndex & ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );
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

    connect( m_treeModel, SIGNAL( expandIndex( const QModelIndex & ) ), SLOT( slotExpand( const QModelIndex & ) ) );
}


CollectionTreeView::~CollectionTreeView()
{
    DEBUG_BLOCK

    delete m_treeModel;
    delete m_filterModel;
}

void
CollectionTreeView::setLevels( const QList<int> &levels )
{
    m_treeModel->setLevels( levels );
}


void
CollectionTreeView::setLevel( int level, int type )
{
    QList<int> levels = m_treeModel->levels();
    if ( type == CategoryId::None )
    {
        while( levels.count() >= level )
            levels.removeLast();
    }
    else
    {
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
CollectionTreeView::contextMenuEvent( QContextMenuEvent* event )
{

    KAction separator( this );
    separator.setSeparator( true );

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

    // Abort if nothing is selected
    if( indices.isEmpty() )
        return;

    m_currentItems.clear();
    foreach( const QModelIndex &index, indices )
    {
        if( index.isValid() && index.internalPointer() )
            m_currentItems.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
    }

    PopupDropperActionList actions = createBasicActions( indices );
    actions += createExtendedActions( indices );
    actions += createCollectionActions( indices );

    KMenu menu;

    int count = 0;
    foreach( PopupDropperAction * action, actions )
    {
        menu.addAction( action );

        //HACK this is a really ugly hack, find a better solution for the separator position
        count++;
        if( count == 2 )
            menu.addAction( &separator );
    }

    m_currentCopyDestination = getCopyActions( indices );
    m_currentMoveDestination = getMoveActions( indices );

    if ( !( m_currentCopyDestination.empty() && m_currentMoveDestination.empty() ) )
    {
        if ( m_cmSeperator == 0 )
        {
            m_cmSeperator = new PopupDropperAction( this );
            m_cmSeperator->setSeparator ( true );
        }
        menu.addAction( m_cmSeperator );
    }


    if ( !m_currentCopyDestination.empty() )
    {
        KMenu *copyMenu = new KMenu( i18n( "Copy to Collection" ), &menu );
        foreach( PopupDropperAction *action, m_currentCopyDestination.keys() ) 
        {
            action->setParent( copyMenu );
            copyMenu->addAction( action );
        }
        menu.addMenu( copyMenu );
    }

    if ( !m_currentMoveDestination.empty() )
    {
        KMenu *moveMenu = new KMenu( i18n( "Move to Collection" ), &menu );
        foreach( PopupDropperAction * action, m_currentCopyDestination.keys() )
        {
            action->setParent( moveMenu );
            moveMenu->addAction( action );
        }
        menu.addMenu( moveMenu );
    }



    menu.exec( event->globalPos() );
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
    QTreeView::mousePressEvent( e );
}

void CollectionTreeView::keyPressEvent( QKeyEvent * event )
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

    m_currentItems.clear();
    foreach( const QModelIndex &index, indices )
    {
        if( index.isValid() && index.internalPointer() )
            m_currentItems.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
    }

    if( indices.isEmpty() ) 
    {
        QTreeView::keyPressEvent( event );
        return;
    }
    QModelIndex current = currentIndex();
    switch( event->key() ) 
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            slotAppendChildTracks();
            return;
        case Qt::Key_Up:
        case Qt::Key_Down:
            QAbstractItemView::keyPressEvent( event );
            return;
        // L and R should magically work when we get a patched version of qt
        case Qt::Key_Right:
        case Qt::Key_Direction_R:
            expand( current );
            break;
        case Qt::Key_Left:
        case Qt::Key_Direction_L:
            collapse( current );
            break;
        default:
            break;
    }
    QTreeView::keyPressEvent( event );
}

void
CollectionTreeView::startDrag(Qt::DropActions supportedActions)
{
    DEBUG_BLOCK

    //setSelectionMode( QAbstractItemView::NoSelection );

    // When a parent item is dragged, startDrag() is called a bunch of times. Here we prevent that:
    m_dragMutex.lock();
    if( m_ongoingDrag )
    {
        m_dragMutex.unlock();
        return;
    }
    m_ongoingDrag = true;
    m_dragMutex.unlock();

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
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

        PopupDropperActionList actions = createBasicActions( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        foreach( PopupDropperAction * action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ), false );

        m_currentCopyDestination = getCopyActions( indices );
        m_currentMoveDestination = getMoveActions( indices );

        m_currentItems.clear();
        foreach( const QModelIndex &index, indices )
        {
            if( index.isValid() && index.internalPointer() )
                m_currentItems.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
        }

        PopupDropperItem* subItem;

        actions = createExtendedActions( indices );

        PopupDropper * morePud = 0;
        if ( actions.count() > 1 )
        {
            morePud = The::popupDropperFactory()->createPopupDropper( 0 );

            foreach( PopupDropperAction * action, actions )
                morePud->addItem( The::popupDropperFactory()->createItem( action ), false );
        }
        else
            m_pd->addItem( The::popupDropperFactory()->createItem( actions[0] ), false );

        //TODO: Keep bugging i18n team about problems with 3 dots
        if ( actions.count() > 1 )
        {
            subItem = m_pd->addSubmenu( &morePud, The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "more",  i18n( "More..." )  );
            The::popupDropperFactory()->adjustSubmenuItem( subItem );
        }
        
        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }

    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
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
    DEBUG_BLOCK
    debug() << "modelindex = " << index;
    debug() << "m_filterModel ? " << (m_filterModel ? "true" : "false");
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
    QSet<CollectionTreeItem*> items;
    items.insert( item );

    playChildTracks( items, insertMode );
}

void
CollectionTreeView::playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode )
{
    //Ensure that if a parent and child are both selected we ignore the child
    QSet<CollectionTreeItem*> parents( cleanItemSet( items ) );

    //Store the type of playlist insert to be done and cause a slot to be invoked when the tracklist has been generated.
    AmarokMimeData *mime = dynamic_cast<AmarokMimeData*>( m_treeModel->mimeData( QList<CollectionTreeItem*>::fromSet( parents ) ) );
    m_playChildTracksMode.insert( mime, insertMode );
    connect( mime, SIGNAL( trackListSignal( Meta::TrackList ) ), this, SLOT( playChildTracksSlot( Meta::TrackList) ) );
    mime->getTrackListSignal();
}

void
CollectionTreeView::playChildTracksSlot( Meta::TrackList list ) //slot
{
    AmarokMimeData *mime = dynamic_cast<AmarokMimeData*>( sender() );

    Playlist::AddOptions insertMode = m_playChildTracksMode.take( mime );

    qStableSort( list.begin(), list.end(), Meta::Track::lessThan );
    The::playlistController()->insertOptioned( list, insertMode );

    mime->deleteLater();
}


void
CollectionTreeView::organizeTracks( const QSet<CollectionTreeItem*> &items ) const
{
    DEBUG_BLOCK
    if( !items.count() )
    {
        return;
    }

    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    QueryMaker *qm = createMetaQueryFromItems( items, true );

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
    DEBUG_BLOCK
    if( !destination->isWritable() )
    {
        return;
    }
    //copied from organizeTracks. create a method for this somewhere
    if( !items.count() )
    {
        return;
    }

    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    QueryMaker *qm = createMetaQueryFromItems( items, true );

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
        if( !source->isWritable() ) //error
        {
            warning() << "We can not write to ze source!!! OMGooses!";
            delete dest;
            delete source;
            delete qm;
            return;
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
    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    QueryMaker *qm = createMetaQueryFromItems( items, true );

    (void)new TagDialog( qm ); //the dialog will show itself automatically as soon as it is ready
}

void CollectionTreeView::slotFilterNow()
{
    m_treeModel->slotFilter();
    setFocus( Qt::OtherFocusReason );
}

PopupDropperActionList CollectionTreeView::createBasicActions( const QModelIndexList & indices )
{
    PopupDropperActionList actions;

    if( !indices.isEmpty() )
    {
        if( m_appendAction == 0 )
        {
            m_appendAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "append", KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this );
            connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendChildTracks() ) );
        }

        actions.append( m_appendAction );

        if( m_loadAction == 0 )
        {
            m_loadAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "load", KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
            connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotPlayChildTracks() ) );
        }

        actions.append( m_loadAction );
    }

    return actions;
}

PopupDropperActionList CollectionTreeView::createExtendedActions( const QModelIndexList & indices )
{
    PopupDropperActionList actions;

    if( !indices.isEmpty() )
    {
        if ( m_editAction == 0 )
        {
            m_editAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "edit", KIcon( "media-track-edit-amarok" ), i18n( "&Edit Track Details" ), this );
            connect( m_editAction, SIGNAL( triggered() ), this, SLOT( slotEditTracks() ) );
        }
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
                    Q_UNUSED( index )
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
                    {
                        m_organizeAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "organize", KIcon("folder-open" ), i18nc( "Organize Files", "Organize Files" ), this );
                        connect( m_organizeAction, SIGNAL( triggered() ), this, SLOT( slotOrganize() ) );
                    }
                    actions.append( m_organizeAction );
                }
            }
        }


        //hmmm... figure out what kind of item we are dealing with....

        if ( indices.size() == 1 )
        {
            debug() << "checking for global actions";
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );

            PopupDropperActionList gActions = The::globalCollectionActions()->actionsFor( item->data() );
            foreach( PopupDropperAction *action, gActions )
            {
                actions.append( action );
                debug() << "Got global action: " << action->text();
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
                        //actions.append( m_caSeperator );

                        PopupDropperActionList cActions = cac->customActions();

                        foreach( PopupDropperAction *action, cActions )
                        {
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

PopupDropperActionList
CollectionTreeView::createCollectionActions( const QModelIndexList & indices )
{
    DEBUG_BLOCK
    PopupDropperActionList actions;

    // Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    // This will fetch TrackList, pass TrackList to appropriate function with done signal

    QueryMaker *qm = createMetaQueryFromItems( m_currentItems, true );

    qm->setQueryType( QueryMaker::Track );


    // Extract collection whose constituent was selected

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( indices.first().internalPointer() );
    while( item->isDataItem() )
    {
        item = item->parent();
    }
    Collection *collection = item->parentCollection();

    // Generate CollectionCapability, test for existence

    Meta::CollectionCapability *cc = collection->as<Meta::CollectionCapability>();

    if( cc )
    {
        debug() << "Has Collection Capability!";
        actions = cc->collectionActions( qm );
    }
    else
    {
        debug() << "Does not have collection capability!";
        qm->deleteLater();
    }

// Sample delete action skeleton, will be replaced
    /*

    PopupDropperAction *deleteAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "delete", KIcon( "amarok_remove" ), i18n( "&Delete Tracks" ), 0 );

    connect( deleteAction, SIGNAL( triggered() ), this, SLOT( slotDeleteTracks() ) );

    actions.append( deleteAction );
    */

    return actions;
}


QHash<PopupDropperAction*, Collection*> CollectionTreeView::getCopyActions(const QModelIndexList & indices )
{
    QHash<PopupDropperAction*, Collection*> m_currentCopyDestination;

    if( onlyOneCollection( indices) )
    {
        Collection *collection = getCollection( indices.first() );
        QList<Collection*> writableCollections;
        foreach( Collection *coll, CollectionManager::instance()->collections().keys() )
        {
            if( coll && coll->isWritable() && coll != collection )
            {
                writableCollections.append( coll );
            }
        }
        if( !writableCollections.isEmpty() )
        {
            foreach( Collection *coll, writableCollections )
            {
                PopupDropperAction *action = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "collection", QIcon(), coll->prettyName(), 0 );

                connect( action, SIGNAL( triggered() ), this, SLOT( slotCopyTracks() ) );

                m_currentCopyDestination.insert( action, coll );
            }
        }
    }
    return m_currentCopyDestination;
}

QHash<PopupDropperAction*, Collection*> CollectionTreeView::getMoveActions( const QModelIndexList & indices )
{
    QHash<PopupDropperAction*, Collection*> m_currentMoveDestination;

    if( onlyOneCollection( indices) )
    {
        Collection *collection = getCollection( indices.first() );
        QList<Collection*> writableCollections;
        QHash<Collection*, CollectionManager::CollectionStatus> hash = CollectionManager::instance()->collections();
        QHash<Collection*, CollectionManager::CollectionStatus>::const_iterator it = hash.constBegin();
        while ( it != hash.constEnd() )
        {
            Collection *coll = it.key();
            if( coll && coll->isWritable() && coll != collection )
            {
                writableCollections.append( coll );
            }
            ++it;
        }
        if( !writableCollections.isEmpty() )
        {
            if( collection->isWritable() )
            {
                foreach( Collection *coll, writableCollections )
                {
                    PopupDropperAction *action = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "collection", QIcon(), coll->prettyName(), 0 );

                    connect( action, SIGNAL( triggered() ), this, SLOT( slotMoveTracks() ) );
                    m_currentMoveDestination.insert( action, coll );
                }
            }
        }
    }
    return m_currentMoveDestination;
}

bool CollectionTreeView::onlyOneCollection( const QModelIndexList & indices )
{
    DEBUG_BLOCK

    if( !indices.isEmpty() )
    {
        Collection *collection = getCollection( indices.first() );
        foreach( const QModelIndex &index, indices )
        {
            Collection *currentCollection = getCollection( index );
            if( collection != currentCollection )
                return false;
        }
    }

    return true;
}

Collection * CollectionTreeView::getCollection( const QModelIndex & index )
{
    Collection *collection = 0;
    if( index.isValid() )
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );
        while( item->isDataItem() )
        {
            item = item->parent();
        }
        collection = item->parentCollection();
    }

    return collection;
}

void CollectionTreeView::mouseReleaseEvent( QMouseEvent * event )
{
    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
    }
    m_pd = 0;

    QTreeView::mouseReleaseEvent( event );
}

void CollectionTreeView::slotPlayChildTracks()
{
    playChildTracks( m_currentItems, Playlist::LoadAndPlay );
}

void CollectionTreeView::slotAppendChildTracks()
{
    playChildTracks( m_currentItems, Playlist::AppendAndPlay );
}

void CollectionTreeView::slotDeleteTracks()
{
    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    QueryMaker *qm = createMetaQueryFromItems( m_currentItems, true );

    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( deleteResultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( deleteQueryDone() ), Qt::QueuedConnection );
    qm->run();
}

void CollectionTreeView::slotEditTracks()
{
    editTracks( m_currentItems );
}

void CollectionTreeView::slotCopyTracks()
{
    if( sender() ) {
        if ( PopupDropperAction * action = dynamic_cast<PopupDropperAction *>( sender() ) )
            copyTracks( m_currentItems, m_currentCopyDestination[ action ], false );
    }
}

void CollectionTreeView::slotMoveTracks()
{
    if( sender() ) {
        if ( PopupDropperAction * action = dynamic_cast<PopupDropperAction *>( sender() ) )
            copyTracks( m_currentItems, m_currentCopyDestination[ action ], true );
    }
}

void CollectionTreeView::slotOrganize()
{
    if( sender() ) {
        if( PopupDropperAction * action = dynamic_cast<PopupDropperAction *>( sender() ) )
            Q_UNUSED( action )
            organizeTracks( m_currentItems );
    }
}


void CollectionTreeView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( this );
}

void
CollectionTreeView::deleteResultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )

    foreach( Meta::TrackPtr d_track, tracks )
    {
        if ( d_track )
            debug() << "Artist is: " << d_track->artist()->name();
    }

    const QString text( i18nc( "@info", "Do you really want to delete these %1 tracks?", tracks.count() ) );
    const bool del = KMessageBox::warningContinueCancel(this,
            text,
            QString() ) == KMessageBox::Continue;

    debug() << "Wants to delete: " << (del ? "Yes" : "No");
}

void
CollectionTreeView::deleteQueryDone()
{
    DEBUG_BLOCK
}

void CollectionTreeView::drawRow( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QStyleOptionViewItemV4 opt = option;

    const bool alternate = opt.features & QStyleOptionViewItemV2::Alternate;

    const int width = option.rect.width();
    const int height = option.rect.height();

    if( height > 0 )
    {
        painter->save();
        QPixmap background;

        //debug() << "features: " << opt.features;

        if ( !alternate )
            background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );
        else
            background = The::svgHandler()->renderSvgWithDividers( "alt_service_list_item", width, height, "alt_service_list_item" );

        painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );

        painter->restore();
    }
    
    QTreeView::drawRow( painter, option, index ); 
}

QSet<CollectionTreeItem*>
CollectionTreeView::cleanItemSet( const QSet<CollectionTreeItem*> &items )
{
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
    return parents;
}

QueryMaker*
CollectionTreeView::createMetaQueryFromItems( const QSet<CollectionTreeItem*> &items, bool cleanItems ) const
{
    QSet<CollectionTreeItem*> parents = cleanItems ? cleanItemSet( items ) : items;

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
    return new MetaQueryMaker( queryMakers );
}

#include "CollectionTreeView.moc"

