/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "CollectionTreeView"

#include "CollectionTreeView.h"

#include "AmarokMimeData.h"
#include "GlobalCollectionActions.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "browsers/CollectionSortFilterProxyModel.h"
#include "browsers/CollectionTreeItemModel.h"
#include "context/ContextView.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/collections/CollectionLocation.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/TextualQueryFilter.h"
#include "core-impl/collections/support/TrashCollectionLocation.h"
#include "dialogs/TagDialog.h"
#include "playlist/PlaylistModelStack.h"
#include "scripting/scriptengine/AmarokCollectionViewScript.h"

#include <QAction>
#include <QIcon>
#include <QComboBox>
#include <QMenu>

#include <QContextMenuEvent>
#include <QHash>
#include <QMouseEvent>
#include <QQueue>
#include <QSortFilterProxyModel>
#include <QScrollBar>

#include <algorithm>

using namespace Collections;

/**
 * RAII class to perform restoring of the scroll position once all queries are
 * finished. DelayedScroller auto-deletes itself once its job is over (ot if it finds
 * it is useless).
 */
class DelayedScroller : public QObject
{
    Q_OBJECT

    public:
        DelayedScroller( CollectionTreeView *treeView,
                         CollectionTreeItemModelBase *treeModel,
                         const QModelIndex &treeModelScrollToIndex, int topOffset )
            : QObject( treeView )
            , m_treeView( treeView )
            , m_treeModel( treeModel )
            , m_topOffset( topOffset )
        {
            connect( treeModel, &CollectionTreeItemModelBase::destroyed,
                     this, &DelayedScroller::deleteLater );
            connect( treeModel, &CollectionTreeItemModelBase::allQueriesFinished,
                     this, &DelayedScroller::slotScroll );

            m_scrollToItem = m_treeModel->treeItem( treeModelScrollToIndex );
            if( m_scrollToItem )
                connect( m_scrollToItem, &CollectionTreeItem::destroyed, this, &DelayedScroller::deleteLater );
            else
                deleteLater(); // nothing to do
        }

    private Q_SLOTS:
        void slotScroll()
        {
            deleteLater();
            QModelIndex idx = m_treeModel->itemIndex( m_scrollToItem );
            QSortFilterProxyModel *filterModel = m_treeView->filterModel();
            idx = filterModel ? filterModel->mapFromSource( idx ) : QModelIndex();
            QScrollBar *scrollBar = m_treeView->verticalScrollBar();
            if( !idx.isValid() || !scrollBar )
                return;

            int newTopOffset = m_treeView->visualRect( idx ).top();
            scrollBar->setValue( scrollBar->value() + (newTopOffset - m_topOffset) );
        }

    private:
        CollectionTreeView *m_treeView;
        CollectionTreeItemModelBase *m_treeModel;
        CollectionTreeItem *m_scrollToItem;
        int m_topOffset;
};

/**
 * RAII class to auto-expand collection tree entries after filtering.
 * AutoExpander auto-deletes itself once its job is over (or if it finds
 * it is useless).
 */
class AutoExpander : public QObject
{
    Q_OBJECT

    public:
        AutoExpander( CollectionTreeView *treeView,
                      CollectionTreeItemModelBase *treeModel,
                      QAbstractItemModel *filterModel)
            : QObject( treeView )
            , m_treeView( treeView )
            , m_filterModel( filterModel )
        {
            connect( filterModel, &QObject::destroyed, this, &QObject::deleteLater );
            connect( treeModel, &CollectionTreeItemModelBase::allQueriesFinished, this, &AutoExpander::slotExpandMore );

            // start with the root index
            m_indicesToCheck.enqueue( QModelIndex() );
            slotExpandMore();
        }

    private Q_SLOTS:
        void slotExpandMore()
        {
            const int maxChildrenToExpand = 3;

            QQueue<QModelIndex> pendingIndices;
            while( !m_indicesToCheck.isEmpty() )
            {
                if( !m_filterModel )
                    return;

                QModelIndex current = m_indicesToCheck.dequeue();

                if( m_filterModel->canFetchMore( current ) )
                {
                    m_filterModel->fetchMore( current );
                    pendingIndices.enqueue( current );
                    continue;
                }

                if( m_filterModel->rowCount( current ) <= maxChildrenToExpand )
                {
                    m_treeView->expand( current );
                    for( int i = 0; i < m_filterModel->rowCount( current ); i++ )
                        m_indicesToCheck.enqueue( m_filterModel->index( i, 0, current ) );
                }
            }

            if( pendingIndices.isEmpty() )
                // nothing left to do
                deleteLater();
            else
                // process pending indices when queries finish
                m_indicesToCheck.swap( pendingIndices );
        }

    private:
        Q_DISABLE_COPY(AutoExpander)

        CollectionTreeView *m_treeView;
        QPointer<QAbstractItemModel> m_filterModel;
        QQueue<QModelIndex> m_indicesToCheck;
};

CollectionTreeView::CollectionTreeView( QWidget *parent)
    : Amarok::PrettyTreeView( parent )
    , m_filterModel( nullptr )
    , m_treeModel( nullptr )
    , m_pd( nullptr )
    , m_appendAction( nullptr )
    , m_loadAction( nullptr )
    , m_editAction( nullptr )
    , m_organizeAction( nullptr )
    , m_ongoingDrag( false )
{
    setSortingEnabled( true );
    setFocusPolicy( Qt::StrongFocus );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setEditTriggers( EditKeyPressed );

    setDragDropMode( QAbstractItemView::DragDrop );

    connect( this, &CollectionTreeView::collapsed,
             this, &CollectionTreeView::slotCollapsed );
    connect( this, &CollectionTreeView::expanded,
             this, &CollectionTreeView::slotExpanded );
}

void
CollectionTreeView::setModel( QAbstractItemModel *model )
{
    if( m_treeModel )
        disconnect( m_treeModel, nullptr, this, nullptr);

    m_treeModel = qobject_cast<CollectionTreeItemModelBase *>( model );
    if( !m_treeModel )
        return;

    connect( m_treeModel, &CollectionTreeItemModelBase::allQueriesFinished,
             this, &CollectionTreeView::slotCheckAutoExpand );
    connect( m_treeModel, &CollectionTreeItemModelBase::expandIndex,
             this, &CollectionTreeView::slotExpandIndex );

    if( m_filterModel )
        m_filterModel->deleteLater();
    m_filterModel = new CollectionSortFilterProxyModel( this );
    m_filterModel->setSourceModel( model );

    QTreeView::setModel( m_filterModel );

    QTimer::singleShot( 0, this, &CollectionTreeView::slotCheckAutoExpandReally );
}

CollectionTreeView::~CollectionTreeView()
{
    // we don't own m_treeModel pointer
    // m_filterModel will get deleted by QObject parentship
}

void
CollectionTreeView::setLevels( const QList<CategoryId::CatMenuId> &levels )
{
    if( m_treeModel )
        m_treeModel->setLevels( levels );
}

QList<CategoryId::CatMenuId>
CollectionTreeView::levels() const
{
    if( m_treeModel )
        return m_treeModel->levels();
    return QList<CategoryId::CatMenuId>();
}

void
CollectionTreeView::setLevel( int level, CategoryId::CatMenuId type )
{
    if( !m_treeModel )
        return;
    QList<CategoryId::CatMenuId> levels = m_treeModel->levels();
    if( type == CategoryId::None )
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

QSortFilterProxyModel *
CollectionTreeView::filterModel() const
{
    return m_filterModel;
}

void
CollectionTreeView::contextMenuEvent( QContextMenuEvent *event )
{
    if( !m_treeModel )
        return;

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        Amarok::PrettyTreeView::contextMenuEvent( event );
        return;
    }

    QModelIndexList indices = selectedIndexes();

    // if previously selected indices do not contain the index of the item
    // currently under the mouse when context menu is invoked.
    if( !indices.contains( index ) )
    {
        indices.clear();
        indices << index;
        setCurrentIndex( index );
    }

    //TODO: get rid of this, it's a hack.
    // Put remove actions in model so we don't need access to the internal pointer in view
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
            m_currentItems.insert(
                            static_cast<CollectionTreeItem *>( index.internalPointer() )
                        );
    }

    QMenu menu( this );

    // Destroy the menu when the model is reset (collection update), so that we don't
    // operate on invalid data. see BUG 190056
    connect( m_treeModel, &CollectionTreeItemModelBase::modelReset, &menu, &QMenu::deleteLater );

    // create basic actions
    QActionList actions = createBasicActions( indices );
    foreach( QAction *action, actions ) {
        menu.addAction( action );
    }
    menu.addSeparator();
    actions.clear();

    QActionList albumActions = createCustomActions( indices );
    QMenu menuAlbum( i18n( "Album" )  );
    foreach( QAction *action, albumActions )
    {
        if( !action->parent() )
            action->setParent( &menuAlbum );
    }

    if( albumActions.count() > 1 )
    {
        menuAlbum.addActions( albumActions );
        menuAlbum.setIcon( QIcon::fromTheme( QStringLiteral("filename-album-amarok") ) );
        menu.addMenu( &menuAlbum );
        menu.addSeparator();
    }
    else if( albumActions.count() == 1 )
    {
        menu.addActions( albumActions );
    }

    QActionList collectionActions = createCollectionActions( indices );
    QMenu menuCollection( i18n( "Collection" ) );
    foreach( QAction *action, collectionActions )
    {
        if( !action->parent() )
            action->setParent( &menuCollection );
    }

    if( collectionActions.count() > 1 )
    {
        menuCollection.setIcon( QIcon::fromTheme( QStringLiteral("drive-harddisk") ) );
        menuCollection.addActions( collectionActions );
        menu.addMenu( &menuCollection );
        menu.addSeparator();
    }
    else if( collectionActions.count() == 1 )
    {
        menu.addActions( collectionActions );
    }

    m_currentCopyDestination = getCopyActions( indices );
    m_currentMoveDestination = getMoveActions( indices );

    if( !m_currentCopyDestination.empty() )
    {
        QMenu *copyMenu = new QMenu( i18n( "Copy to Collection" ), &menu );
        copyMenu->setIcon( QIcon::fromTheme( QStringLiteral("edit-copy") ) );
        copyMenu->addActions( m_currentCopyDestination.keys() );
        menu.addMenu( copyMenu );
    }

    //Move = copy + delete from source
    if( !m_currentMoveDestination.empty() )
    {
        QMenu *moveMenu = new QMenu( i18n( "Move to Collection" ), &menu );
        moveMenu->setIcon( QIcon::fromTheme( QStringLiteral("go-jump") ) );
        moveMenu->addActions( m_currentMoveDestination.keys() );
        menu.addMenu( moveMenu );
    }

    // create trash and delete actions
    if( onlyOneCollection( indices ) )
    {
        Collection *collection = getCollection( indices.first() );
        if( collection && collection->isWritable() )
        {
            //TODO: don't recreate action
            QAction *trashAction = new QAction( QIcon::fromTheme( QStringLiteral("user-trash") ),
                                                i18n( "Move Tracks to Trash" ),
                                                &menu );
            trashAction->setProperty( "popupdropper_svg_id", "delete" );
            // key shortcut is only for display purposes here, actual one is
            // determined by View in Model/View classes
            trashAction->setShortcut( Qt::Key_Delete );
            connect( trashAction, &QAction::triggered,
                     this, &CollectionTreeView::slotTrashTracks );
            menu.addAction( trashAction );

            QAction *deleteAction = new QAction( QIcon::fromTheme( QStringLiteral("remove-amarok") ),
                                                 i18n( "Delete Tracks" ),
                                                 &menu );
            deleteAction->setProperty( "popupdropper_svg_id", "delete" );
            // key shortcut is only for display purposes here, actual one is
            // determined by View in Model/View classes
            deleteAction->setShortcut( Qt::SHIFT + Qt::Key_Delete );
            connect( deleteAction, &QAction::triggered, this, &CollectionTreeView::slotDeleteTracks );
            menu.addAction( deleteAction );
        }
    }

    // add extended actions
    menu.addSeparator();
    actions += createExtendedActions( indices );
    foreach( QAction *action, actions ) {
        menu.addAction( action );
    }
    AmarokScript::AmarokCollectionViewScript::createScriptedActions( menu, indices );

    menu.exec( event->globalPos() );
}

void
CollectionTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    if( event->button() == Qt::MidButton )
    {
        event->accept();
        return;
    }

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    // code copied in PlaylistBrowserView::mouseDoubleClickEvent(), keep in sync
    // mind bug 279513
    bool isExpandable = model()->hasChildren( index );
    bool wouldExpand = !visualRect( index ).contains( event->pos() ) || // clicked outside item, perhaps on expander icon
                       ( isExpandable && !style()->styleHint( QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, this ) ); // we're in doubleClick
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        !wouldExpand )
    {
        CollectionTreeItem *item = getItemFromIndex( index );
        playChildTracks( item, Playlist::OnDoubleClickOnSelectedItems );
        event->accept();
        return;
    }

    PrettyTreeView::mouseDoubleClickEvent( event );
}

void
CollectionTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pd )
    {
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::deleteLater );
        m_pd->hide();
        m_pd = nullptr;
    }

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        PrettyTreeView::mouseReleaseEvent( event );
        return;
    }

    if( event->button() == Qt::MidButton )
    {
        CollectionTreeItem *item = getItemFromIndex( index );
        playChildTracks( item, Playlist::OnMiddleClickOnSelectedItems );
        event->accept();
        return;
    }

    PrettyTreeView::mouseReleaseEvent( event );
}

CollectionTreeItem *
CollectionTreeView::getItemFromIndex( QModelIndex &index )
{
    QModelIndex filteredIndex;
    if( m_filterModel )
        filteredIndex = m_filterModel->mapToSource( index );
    else
        filteredIndex = index;

    if( !filteredIndex.isValid() )
    {
        return nullptr;
    }

    return static_cast<CollectionTreeItem *>( filteredIndex.internalPointer() );
}

void
CollectionTreeView::keyPressEvent( QKeyEvent *event )
{
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
    {
        Amarok::PrettyTreeView::keyPressEvent( event );
        return;
    }

    if( m_filterModel )
    {
        QModelIndexList tmp;
        foreach( const QModelIndex &idx, indices )
            tmp.append( m_filterModel->mapToSource( idx ) );
        indices = tmp;
    }

    m_currentItems.clear();
    foreach( const QModelIndex &index, indices )
    {
        if( index.isValid() && index.internalPointer() )
        {
            m_currentItems.insert(
                        static_cast<CollectionTreeItem *>( index.internalPointer() ) );
        }
    }

    QModelIndex current = currentIndex();
    switch( event->key() )
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            playChildTracks( m_currentItems, Playlist::OnReturnPressedOnSelectedItems );
            return;
        case Qt::Key_Delete:
            if( !onlyOneCollection( indices ) )
                break;
            removeTracks( m_currentItems, !( event->modifiers() & Qt::ShiftModifier ) );
            return;
        case Qt::Key_Up:
            if( current.parent() == QModelIndex() && current.row() == 0 )
            {
                Q_EMIT leavingTree();
                return;
            }
            break;
        case Qt::Key_Down:
            break;
        default:
            break;
    }
    Amarok::PrettyTreeView::keyPressEvent( event );
}

void
CollectionTreeView::dragEnterEvent( QDragEnterEvent *event )
{
    // We want to indicate to the user that dropping to the same collection is not possible.
    // CollectionTreeItemModel therefore needs to know what collection the drag originated
    // so that is can play with Qt::ItemIsDropEnabled in flags()
    const AmarokMimeData *mimeData =
            qobject_cast<const AmarokMimeData *>( event->mimeData() );
    if( mimeData ) // drag from within Amarok
    {
        QSet<Collection *> srcCollections;
        foreach( Meta::TrackPtr track, mimeData->tracks() )
        {
            srcCollections.insert( track->collection() );
        }
        m_treeModel->setDragSourceCollections( srcCollections );
    }
    QAbstractItemView::dragEnterEvent( event );
}

void
CollectionTreeView::dragMoveEvent( QDragMoveEvent *event )
{
    // this mangling is not needed for Copy/Move distinction to work, it is only needed
    // for mouse cursor changing to work
    if( (event->keyboardModifiers() & Qt::ShiftModifier)
        && (event->possibleActions() & Qt::MoveAction) )
    {
        event->setDropAction( Qt::MoveAction );
    }
    else if( event->possibleActions() & Qt::CopyAction )
    {
        event->setDropAction( Qt::CopyAction );
    }

    QTreeView::dragMoveEvent( event );
}

void
CollectionTreeView::startDrag(Qt::DropActions supportedActions)
{
    DEBUG_BLOCK

    // Make sure that the left mouse button is actually pressed. Otherwise we're prone to
    // mis-detecting clicks as dragging
    if( !( QApplication::mouseButtons() & Qt::LeftButton ) )
        return;

    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return;

    // When a parent item is dragged, startDrag() is called a bunch of times. Here we
    // prevent that:
    if( m_ongoingDrag )
        return;
    m_ongoingDrag = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        if( m_filterModel )
        {
            QModelIndexList tmp;
            foreach( const QModelIndex &idx, indices )
            {
                tmp.append( m_filterModel->mapToSource( idx ) );
            }
            indices = tmp;
        }

        QActionList actions = createBasicActions( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        foreach( QAction * action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_currentCopyDestination = getCopyActions( indices );
        m_currentMoveDestination = getMoveActions( indices );

        m_currentItems.clear();
        foreach( const QModelIndex &index, indices )
        {
            if( index.isValid() && index.internalPointer() )
            {
                m_currentItems.insert(
                        static_cast<CollectionTreeItem *>( index.internalPointer() ) );
            }
        }

        PopupDropperItem *subItem;

        actions = createExtendedActions( indices );

        PopupDropper *morePud = nullptr;
        if( actions.count() > 1 )
        {
            morePud = The::popupDropperFactory()->createPopupDropper( nullptr, true );

            foreach( QAction *action, actions )
                morePud->addItem( The::popupDropperFactory()->createItem( action ) );
        }
        else
            m_pd->addItem( The::popupDropperFactory()->createItem( actions[0] ) );

        //TODO: Keep bugging i18n team about problems with 3 dots
        if ( actions.count() > 1 )
        {
            subItem = m_pd->addSubmenu( &morePud, i18n( "More..." )  );
            The::popupDropperFactory()->adjustItem( subItem );
        }

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::clear );
        m_pd->hide();
    }

    m_ongoingDrag = false;
}

void
CollectionTreeView::selectionChanged( const QItemSelection &selected,
                                      const QItemSelection &deselected )
{
    QModelIndexList indexes = selected.indexes();

    QModelIndexList changedIndexes = indexes;
    changedIndexes << deselected.indexes();
    foreach( const QModelIndex &index, changedIndexes )
        update( index );

    if( indexes.count() < 1 )
        return;

    QModelIndex index;
    if( m_filterModel )
        index = m_filterModel->mapToSource( indexes[0] );
    else
        index = indexes[0];

    CollectionTreeItem *item =
            static_cast<CollectionTreeItem *>( index.internalPointer() );
    Q_EMIT( itemSelected ( item ) );
}

void
CollectionTreeView::slotCollapsed( const QModelIndex &index )
{
    if( !m_treeModel )
        return;
    if( m_filterModel )
        m_treeModel->slotCollapsed( m_filterModel->mapToSource( index ) );
    else
        m_treeModel->slotCollapsed( index );
}

void
CollectionTreeView::slotExpanded( const QModelIndex &index )
{
    if( !m_treeModel )
        return;
    if( m_filterModel )
        m_treeModel->slotExpanded( m_filterModel->mapToSource( index ));
    else
        m_treeModel->slotExpanded( index );
}

void
CollectionTreeView::slotExpandIndex( const QModelIndex &index )
{
    if( !m_treeModel )
        return;
    if( m_filterModel )
        expand( m_filterModel->mapFromSource( index ) );
}

void
CollectionTreeView::slotCheckAutoExpand( bool reallyExpand )
{
    if( !m_filterModel || !reallyExpand )
        return;

    // auto-deletes itself:
    new AutoExpander( this, m_treeModel, m_filterModel );
}

void
CollectionTreeView::playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode )
{
    QSet<CollectionTreeItem*> items;
    items.insert( item );

    playChildTracks( items, insertMode );
}

void
CollectionTreeView::playChildTracks( const QSet<CollectionTreeItem *> &items,
                                     Playlist::AddOptions insertMode )
{
    if( !m_treeModel )
        return;
    //Ensure that if a parent and child are both selected we ignore the child
    QSet<CollectionTreeItem *> parents( cleanItemSet( items ) );

    //Store the type of playlist insert to be done and cause a slot to be invoked when the tracklist has been generated.
    AmarokMimeData *mime = dynamic_cast<AmarokMimeData*>(
                m_treeModel->mimeData( QList<CollectionTreeItem *>( parents.begin(), parents.end() ) ) );
    m_playChildTracksMode.insert( mime, insertMode );
    connect( mime, &AmarokMimeData::trackListSignal,
             this, &CollectionTreeView::playChildTracksSlot );
    mime->getTrackListSignal();
}

void
CollectionTreeView::playChildTracksSlot( Meta::TrackList list ) //slot
{
    AmarokMimeData *mime = dynamic_cast<AmarokMimeData *>( sender() );

    Playlist::AddOptions insertMode = m_playChildTracksMode.take( mime );

    std::stable_sort( list.begin(), list.end(), Meta::Track::lessThan );
    The::playlistController()->insertOptioned( list, insertMode );

    mime->deleteLater();
}

void
CollectionTreeView::organizeTracks( const QSet<CollectionTreeItem *> &items ) const
{
    DEBUG_BLOCK
    if( !items.count() )
        return;

    //Create query based upon items, ensuring that if a parent and child are both
    //selected we ignore the child
    Collections::QueryMaker *qm = createMetaQueryFromItems( items, true );
    if( !qm )
        return;

    CollectionTreeItem *item = items.values().first();
    while( item->isDataItem() )
        item = item->parent();

    Collection *coll = item->parentCollection();
    CollectionLocation *location = coll->location();
    if( !location->isOrganizable() )
    {
        debug() << "Collection not organizable";
        //how did we get here??
        delete location;
        delete qm;
        return;
    }
    location->prepareMove( qm, coll->location() );
}

void
CollectionTreeView::copySelectedToLocalCollection()
{
    DEBUG_BLOCK

    // Get the local collection
    Collections::Collection *collection = nullptr;
    const QList<Collections::Collection*> collections = CollectionManager::instance()->collections().keys();

    foreach( collection, collections )
    {
        if ( collection->collectionId() == QLatin1String("localCollection") )
            break;
    }

    if( !collection )
        return;

    // Get selected items
    QModelIndexList indexes = selectedIndexes();
    if( m_filterModel )
    {
        QModelIndexList tmp;
        foreach( const QModelIndex &idx, indexes )
            tmp.append( m_filterModel->mapToSource( idx ) );
        indexes = tmp;
    }

    m_currentItems.clear();
    foreach( const QModelIndex &index, indexes )
    {
        if( index.isValid() && index.internalPointer() )
            m_currentItems.insert( static_cast<CollectionTreeItem *>( index.internalPointer() ) );
    }

    copyTracks( m_currentItems, collection, false );
}

void
CollectionTreeView::copyTracks( const QSet<CollectionTreeItem *> &items,
                                Collection *destination, bool removeSources ) const
{
    DEBUG_BLOCK

    if( !destination )
    {
        warning() << "collection is not writable (0-pointer)! Aborting";
        return;
    }
    if( !destination->isWritable() )
    {
        warning() << "collection " << destination->prettyName() << " is not writable! Aborting";
        return;
    }
    //copied from organizeTracks. create a method for this somewhere
    if( !items.count() )
    {
        warning() << "No items to copy! Aborting";
        return;
    }

    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    Collections::QueryMaker *qm = createMetaQueryFromItems( items, true );
    if( !qm )
    {
        warning() << "could not get qm!";
        return;
    }

    CollectionTreeItem *item = items.values().first();
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

        debug() << "starting source->prepareMove";
        source->prepareMove( qm, dest );
    }
    else
    {
        debug() << "starting source->prepareCopy";
        source->prepareCopy( qm, dest );
    }
}

void
CollectionTreeView::removeTracks( const QSet<CollectionTreeItem *> &items,
                                  bool useTrash ) const
{
    DEBUG_BLOCK

    //copied from organizeTracks. create a method for this somewhere
    if( !items.count() )
        return;

    //Create query based upon items, ensuring that if a parent and child are both selected we ignore the child
    Collections::QueryMaker *qm = createMetaQueryFromItems( items, true );
    if( !qm )
        return;

    CollectionTreeItem *item = items.values().first();
    while( item->isDataItem() )
        item = item->parent();
    Collection *coll = item->parentCollection();

    CollectionLocation *source = coll->location();
    if( !source->isWritable() ) //error
    {
        warning() << "We can not write to ze source!!! OMGooses!";
        delete source;
        delete qm;
        return;
    }

    if( useTrash )
    {
        TrashCollectionLocation *trash = new TrashCollectionLocation();
        source->prepareMove( qm, trash );
    }
    else
        source->prepareRemove( qm );
}

void
CollectionTreeView::editTracks( const QSet<CollectionTreeItem *> &items ) const
{
    //Create query based upon items, ensuring that if a parent and child are both
    //selected we ignore the child
    Collections::QueryMaker *qm = createMetaQueryFromItems( items, true );
    if( !qm )
        return;

    (void)new TagDialog( qm ); //the dialog will show itself automatically as soon as it is ready
}

void
CollectionTreeView::slotSetFilter( const QString &filter )
{
    QString currentFilter = m_treeModel ? m_treeModel->currentFilter() : QString();
    if( !m_filterModel || !m_treeModel || filter == currentFilter )
        return;

    // special case: transitioning from non-empty to empty buffer
    // -> trigger later restoring of the scroll position
    if( filter.isEmpty() ) // currentFilter must not be empty then (see earlier check)
    {
        // take first item, descending to leaf ones if expanded. There may be better
        // ways to determine what item should stay "fixed".
        QModelIndex scrollToIndex = m_filterModel->index( 0, 0 );
        while( isExpanded( scrollToIndex ) && m_filterModel->rowCount( scrollToIndex ) > 0 )
            scrollToIndex = m_filterModel->index( 0, 0, scrollToIndex);
        int topOffset = visualRect( scrollToIndex ).top();

        QModelIndex bottomIndex = m_filterModel->mapToSource( scrollToIndex );
        // if we have somewhere to scroll to after filter is cleared...
        if( bottomIndex.isValid() )
            // auto-destroys itself
            new DelayedScroller( this, m_treeModel, bottomIndex, topOffset );
    }
    m_treeModel->setCurrentFilter( filter );
}

void
CollectionTreeView::slotAddFilteredTracksToPlaylist()
{
    if( !m_treeModel )
        return;

    // disconnect any possible earlier connection we've done
    disconnect( m_treeModel, &CollectionTreeItemModelBase::allQueriesFinished,
                this, &CollectionTreeView::slotAddFilteredTracksToPlaylist );

    if( m_treeModel->hasRunningQueries() )
        // wait for the queries to finish
        connect( m_treeModel, &CollectionTreeItemModelBase::allQueriesFinished,
                 this, &CollectionTreeView::slotAddFilteredTracksToPlaylist );
    else
    {
        // yay, we can add the tracks now
        QSet<CollectionTreeItem *> items;
        for( int row = 0; row < m_treeModel->rowCount(); row++ )
        {
            QModelIndex idx = m_treeModel->index( row, 0 );
            CollectionTreeItem *item = idx.isValid()
                    ? static_cast<CollectionTreeItem *>( idx.internalPointer() ) : nullptr;
            if( item )
                items.insert( item );
        }
        if( !items.isEmpty() )
            playChildTracks( items, Playlist::OnAppendToPlaylistAction );
        Q_EMIT addingFilteredTracksDone();
    }
}

QActionList
CollectionTreeView::createBasicActions( const QModelIndexList &indices )
{
    QActionList actions;

    if( !indices.isEmpty() )
    {
        if( m_appendAction == nullptr )
        {
            m_appendAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-add-amarok") ),
                                          i18n( "&Add to Playlist" ), this );
            m_appendAction->setProperty( "popupdropper_svg_id", "append" );
            connect( m_appendAction, &QAction::triggered, this, &CollectionTreeView::slotAppendChildTracks );
        }

        actions.append( m_appendAction );

        if( m_loadAction == nullptr )
        {
            m_loadAction = new QAction(
                        i18nc( "Replace the currently loaded tracks with these",
                               "&Replace Playlist" ), this );
            m_loadAction->setProperty( "popupdropper_svg_id", "load" );
            connect( m_loadAction, &QAction::triggered,
                     this, &CollectionTreeView::slotReplacePlaylistWithChildTracks );
        }

        actions.append( m_loadAction );
    }

    return actions;
}

QActionList
CollectionTreeView::createExtendedActions( const QModelIndexList &indices )
{
    QActionList actions;

    if( !indices.isEmpty() )
    {
        {   //keep the scope of item minimal
            CollectionTreeItem *item =
                    static_cast<CollectionTreeItem *>( indices.first().internalPointer() );
            while( item->isDataItem() )
                item = item->parent();

            Collection *collection = item->parentCollection();
            CollectionLocation* location = collection->location();

            if( location->isOrganizable() )
            {
                bool onlyOneCollection = true;
                foreach( const QModelIndex &index, indices )
                {
                    Q_UNUSED( index )
                    CollectionTreeItem *item = static_cast<CollectionTreeItem *>(
                                indices.first().internalPointer() );
                    while( item->isDataItem() )
                        item = item->parent();

                    onlyOneCollection = item->parentCollection() == collection;
                    if( !onlyOneCollection )
                        break;
                }

                if( onlyOneCollection )
                {
                    if( m_organizeAction == nullptr )
                    {
                        m_organizeAction = new QAction( QIcon::fromTheme(QStringLiteral("folder-open") ),
                                    i18nc( "Organize Files", "Organize Files" ), this );
                        m_organizeAction->setProperty( "popupdropper_svg_id", "organize" );
                        connect( m_organizeAction, &QAction::triggered,
                                 this, &CollectionTreeView::slotOrganize );
                    }
                    actions.append( m_organizeAction );
                }
            }
            delete location;
        }

        //hmmm... figure out what kind of item we are dealing with....

        if( indices.size() == 1 )
        {
            debug() << "checking for global actions";
            CollectionTreeItem *item = static_cast<CollectionTreeItem *>(
                        indices.first().internalPointer() );

            QActionList gActions = The::globalCollectionActions()->actionsFor( item->data() );
            foreach( QAction *action, gActions )
            {
                if( action ) // Can become 0-pointer, see https://bugs.kde.org/show_bug.cgi?id=183250
                {
                    actions.append( action );
                    debug() << "Got global action: " << action->text();
                }
            }
        }

        if( m_editAction == nullptr )
        {
            m_editAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ),
                                        i18n( "&Edit Track Details" ), this );
            setProperty( "popupdropper_svg_id", "edit" );
            connect( m_editAction, &QAction::triggered, this, &CollectionTreeView::slotEditTracks );
        }
        actions.append( m_editAction );
    }
    else
        debug() << "invalid index or null internalPointer";

    return actions;
}

QActionList
CollectionTreeView::createCustomActions( const QModelIndexList &indices )
{
    QActionList actions;
    if( indices.count() == 1 )
    {
        if( indices.first().isValid() && indices.first().internalPointer() )
        {
            Meta::DataPtr data = static_cast<CollectionTreeItem *>(
                        indices.first().internalPointer() )->data();
            if( data )
            {
                QScopedPointer<Capabilities::ActionsCapability> ac(
                            data->create<Capabilities::ActionsCapability>() );
                if( ac )
                {
                    QActionList cActions = ac->actions();

                    foreach( QAction *action, cActions )
                    {
                        Q_ASSERT( action );
                        actions.append( action );
                        debug() << "Got custom action: " << action->text();
                    }
                }

                //check if this item can be bookmarked...
                QScopedPointer<Capabilities::BookmarkThisCapability> btc(
                            data->create<Capabilities::BookmarkThisCapability>() );
                if( btc && btc->isBookmarkable() && btc->bookmarkAction() )
                    actions.append( btc->bookmarkAction() );
            }
        }
    }
    return actions;
}

QActionList
CollectionTreeView::createCollectionActions( const QModelIndexList &indices )
{
    QActionList actions;
    // Extract collection whose constituent was selected

    CollectionTreeItem *item =
            static_cast<CollectionTreeItem *>( indices.first().internalPointer() );

    // Don't return any collection actions for non collection items
    if( item->isDataItem() )
        return actions;

    Collection *collection = item->parentCollection();

    // Generate CollectionCapability, test for existence

    QScopedPointer<Capabilities::ActionsCapability> cc(
                collection->create<Capabilities::ActionsCapability>() );

    if( cc )
        actions = cc->actions();

    return actions;
}


QHash<QAction *, Collection *>
CollectionTreeView::getCopyActions( const QModelIndexList &indices )
{
    QHash<QAction *, Collection *> currentCopyDestination;

    if( onlyOneCollection( indices ) )
    {
        Collection *collection = getCollection( indices.first() );
        QList<Collection *> writableCollections;
        QHash<Collection *, CollectionManager::CollectionStatus> hash =
                CollectionManager::instance()->collections();
        QHash<Collection *, CollectionManager::CollectionStatus>::const_iterator it =
                hash.constBegin();
        while( it != hash.constEnd() )
        {
            Collection *coll = it.key();
            if( coll && coll->isWritable() && coll != collection )
                writableCollections.append( coll );
            ++it;
        }
        if( !writableCollections.isEmpty() )
        {
            foreach( Collection *coll, writableCollections )
            {
                QAction *action = new QAction( coll->icon(), coll->prettyName(), nullptr );
                action->setProperty( "popupdropper_svg_id", "collection" );
                connect( action, &QAction::triggered, this, &CollectionTreeView::slotCopyTracks );

                currentCopyDestination.insert( action, coll );
            }
        }
    }
    return currentCopyDestination;
}

QHash<QAction *, Collection *>
CollectionTreeView::getMoveActions( const QModelIndexList &indices )
{
    QHash<QAction *, Collection *> currentMoveDestination;

    if( onlyOneCollection( indices ) )
    {
        Collection *collection = getCollection( indices.first() );
        QList<Collection *> writableCollections;
        QHash<Collection *, CollectionManager::CollectionStatus> hash =
                CollectionManager::instance()->collections();
        QHash<Collection *, CollectionManager::CollectionStatus>::const_iterator it =
                hash.constBegin();
        while( it != hash.constEnd() )
        {
            Collection *coll = it.key();
            if( coll && coll->isWritable() && coll != collection )
                writableCollections.append( coll );
            ++it;
        }
        if( !writableCollections.isEmpty() )
        {
            if( collection->isWritable() )
            {
                foreach( Collection *coll, writableCollections )
                {
                    QAction *action = new QAction( coll->icon(), coll->prettyName(), nullptr );
                    action->setProperty( "popupdropper_svg_id", "collection" );
                    connect( action, &QAction::triggered, this, &CollectionTreeView::slotMoveTracks );
                    currentMoveDestination.insert( action, coll );
                }
            }
        }
    }
    return currentMoveDestination;
}

bool CollectionTreeView::onlyOneCollection( const QModelIndexList &indices )
{
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

Collection *
CollectionTreeView::getCollection( const QModelIndex &index )
{
    Collection *collection = nullptr;
    if( index.isValid() )
    {
        CollectionTreeItem *item =
                static_cast<CollectionTreeItem *>( index.internalPointer() );
        while( item->isDataItem() )
            item = item->parent();
        collection = item->parentCollection();
    }

    return collection;
}

void
CollectionTreeView::slotReplacePlaylistWithChildTracks()
{
    playChildTracks( m_currentItems, Playlist::OnReplacePlaylistAction );
}

void
CollectionTreeView::slotAppendChildTracks()
{
    playChildTracks( m_currentItems, Playlist::OnAppendToPlaylistAction );
}

void
CollectionTreeView::slotQueueChildTracks()
{
    playChildTracks( m_currentItems, Playlist::OnQueueToPlaylistAction );
}

void
CollectionTreeView::slotEditTracks()
{
    editTracks( m_currentItems );
}

void
CollectionTreeView::slotCopyTracks()
{
    if( !sender() )
        return;
    if( QAction *action = dynamic_cast<QAction *>( sender() ) )
        copyTracks( m_currentItems, m_currentCopyDestination[ action ], false );
}

void
CollectionTreeView::slotMoveTracks()
{
    if( !sender() )
        return;
    if ( QAction *action = dynamic_cast<QAction *>( sender() ) )
        copyTracks( m_currentItems, m_currentMoveDestination[ action ], true );
}

void
CollectionTreeView::slotTrashTracks()
{
    removeTracks( m_currentItems, true );
}

void
CollectionTreeView::slotDeleteTracks()
{
    removeTracks( m_currentItems, false /* do not use trash */ );
}

void
CollectionTreeView::slotOrganize()
{
    if( sender() )
    {
        if( QAction *action = dynamic_cast<QAction *>( sender() ) )
        {
            Q_UNUSED( action )
            organizeTracks( m_currentItems );
        }
    }
}

QSet<CollectionTreeItem *>
CollectionTreeView::cleanItemSet( const QSet<CollectionTreeItem *> &items )
{
    QSet<CollectionTreeItem *> parents;
    foreach( CollectionTreeItem *item, items )
    {
        CollectionTreeItem *tmpItem = item;
        while( tmpItem )
        {
            if( items.contains( tmpItem->parent() ) )
                tmpItem = tmpItem->parent();
            else
            {
                parents.insert( tmpItem );
                break;
            }
        }
    }
    return parents;
}

Collections::QueryMaker *
CollectionTreeView::createMetaQueryFromItems( const QSet<CollectionTreeItem *> &items,
                                              bool cleanItems ) const
{
    if( !m_treeModel )
        return nullptr;

    QSet<CollectionTreeItem*> parents = cleanItems ? cleanItemSet( items ) : items;

    QList<Collections::QueryMaker *> queryMakers;
    foreach( CollectionTreeItem *item, parents )
    {
        Collections::QueryMaker *qm = item->queryMaker();
        for( CollectionTreeItem *tmp = item; tmp; tmp = tmp->parent() )
            tmp->addMatch( qm, m_treeModel->levelCategory( tmp->level() - 1 ) );
        Collections::addTextualFilter( qm, m_treeModel->currentFilter() );
        queryMakers.append( qm );
    }
    return new Collections::MetaQueryMaker( queryMakers );
}

#include "CollectionTreeView.moc"  // Q_OBJECTs defined in CollectionTreeView.cpp
#include "moc_CollectionTreeView.cpp"  // Q_OBJECTs defined in CollectionTreeView.h
