/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "PlaylistBrowserView"

#include "PlaylistBrowserView.h"

#include "core/support/Debug.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistController.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "PlaylistBrowserModel.h"
#include "PaletteHandler.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "PlaylistsInFoldersProxy.h"
#include "PlaylistsByProviderProxy.h"
#include "widgets/PrettyTreeDelegate.h"
#include "widgets/PrettyTreeRoles.h"

#include <KAction>
#include <KGlobalSettings>
#include <KMenu>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>
#include <QToolTip>

PlaylistBrowserNS::PlaylistBrowserView::PlaylistBrowserView( QAbstractItemModel *model,
                                                             QWidget *parent )
    : Amarok::PrettyTreeView( parent )
    , m_pd( 0 )
    , m_addFolderAction( 0 )
    , m_ongoingDrag( false )
    , m_dragMutex()
    , m_expandToggledWhenPressed( false )
{
    DEBUG_BLOCK
    setModel( model );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setDragDropMode( QAbstractItemView::DragDrop );
    setAcceptDrops( true );
    setEditTriggers( QAbstractItemView::EditKeyPressed );
    setMouseTracking( true ); // needed for highlighting provider action icons
}

PlaylistBrowserNS::PlaylistBrowserView::~PlaylistBrowserView()
{
}

void
PlaylistBrowserNS::PlaylistBrowserView::setModel( QAbstractItemModel *model )
{
    if( this->model() )
        disconnect( this->model(), 0, this, 0 );
    Amarok::PrettyTreeView::setModel( model );

    connect( this->model(), SIGNAL(renameIndex(QModelIndex)), SLOT(edit(QModelIndex)) );
}

void
PlaylistBrowserNS::PlaylistBrowserView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    // Only forward the press event if we aren't on an action (which gets triggered on a release)
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        decoratorActionAt( index, event->pos() ) )
    {
        event->accept();
        return;
    }

    bool prevExpandState = isExpanded( index );

    // This will toggle the expansion of the current item when clicking
    // on the fold marker but not on the item itself. Required here to
    // enable dragging.
    PrettyTreeView::mousePressEvent( event );

    if( index.isValid() )
        m_expandToggledWhenPressed = ( prevExpandState != isExpanded( index ) );
}

QAction *
PlaylistBrowserNS::PlaylistBrowserView::decoratorActionAt( const QModelIndex &index, const QPoint pos )
{
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount <= 0 )
        return 0;

    PrettyTreeDelegate* ptd = qobject_cast<PrettyTreeDelegate*>( itemDelegate( index ) );
    if( !ptd )
        return 0;

    QActionList actions = index.data( PrettyTreeRoles::DecoratorRole ).value<QActionList>();
    QRect rect = visualRect( index );

    for( int i = 0; i < actions.count(); i++ )
        if( ptd->decoratorRect( rect, i ).contains( pos ) )
            return actions.at( i );

    return 0;
}

void
PlaylistBrowserNS::PlaylistBrowserView::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
        m_pd = 0;
    }

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        PrettyTreeView::mouseReleaseEvent( event );
        return;
    }

    if( event->button() == Qt::MidButton )
    {
        appendAndPlay( index );
        event->accept();
        return;
    }

    // if root is decorated, it doesn't show any actions
    QAction *action = rootIsDecorated() ? 0 : decoratorActionAt( index, event->pos() );
    if( action &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier )
    {
        action->trigger();
        event->accept();
        return;
    }

    if( !m_expandToggledWhenPressed &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        KGlobalSettings::singleClick() &&
        model()->hasChildren( index ) )
    {
        m_expandToggledWhenPressed = !m_expandToggledWhenPressed;
        setExpanded( index, !isExpanded( index ) );
        event->accept();
        return;
    }

    PrettyTreeView::mouseReleaseEvent( event );
}

void
PlaylistBrowserNS::PlaylistBrowserView::mouseMoveEvent( QMouseEvent *event )
{
    PrettyTreeView::mouseMoveEvent( event );

    // Make sure we repaint the item for the collection action buttons
    const QModelIndex index = indexAt( event->pos() );
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount )
        update( index );
}

void PlaylistBrowserNS::PlaylistBrowserView::startDrag( Qt::DropActions supportedActions )
{
    //Waah? when a parent item is dragged, startDrag is called a bunch of times
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

    QList<QAction *> actions;

    if( m_pd && m_pd->isHidden() )
    {
        actions = actionsFor( selectedIndexes() );

        foreach( QAction *action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    //We keep the items that the actions need to be applied to in the actions private data.
    //Clear the data from all actions now that the PUD has executed.
    foreach( QAction *action, actions )
        action->setData( QVariant() );

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

void
PlaylistBrowserNS::PlaylistBrowserView::keyPressEvent( QKeyEvent *event )
{
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
    {
        Amarok::PrettyTreeView::keyPressEvent( event );
        return;
    }

    switch( event->key() )
    {
        //activated() only works for current index, not all selected
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if( !state() == EditingState )
            {
                //Why do we even get in this state? Shouldn't the editor consume the
                //keypress? The delete works. see bug 305203
                appendAndPlay( indices );
                return;
            }
            break;
        case Qt::Key_Delete:
            deletePlaylistsTracks( indices );
            return;
        default:
            break;
     }
    Amarok::PrettyTreeView::keyPressEvent( event );
}

void
PlaylistBrowserNS::PlaylistBrowserView::mouseDoubleClickEvent( QMouseEvent *event )
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

    // code copied in src/browser/CollectionTreeView.cpp
    bool isExpandable = model()->hasChildren( index );
    bool wouldExpand = isExpandable && !KGlobalSettings::singleClick(); // we're in doubleClick
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        !wouldExpand )
    {
        appendAndPlay( index );
        event->accept();
        return;
    }
    Amarok::PrettyTreeView::mouseDoubleClickEvent( event );
}

void PlaylistBrowserNS::PlaylistBrowserView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex clickedIdx = indexAt( event->pos() );

    QModelIndexList indices;
    if( selectedIndexes().contains( clickedIdx ) )
        indices << selectedIndexes();
    else
        indices << clickedIdx;

    QActionList actions = actionsFor( indices );

    if( actions.isEmpty() )
        return;

    KMenu menu;
    foreach( QAction *action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    if( indices.count() == 0 )
        menu.addAction( m_addFolderAction );

    menu.exec( mapToGlobal( event->pos() ) );

    //We keep the items that the action need to be applied to in the action's private data.
    //Clear the data from all actions now that the context menu has executed.
    foreach( QAction *action, actions )
        action->setData( QVariant() );
}

bool
PlaylistBrowserNS::PlaylistBrowserView::viewportEvent( QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>( event );
        if( !rootIsDecorated() )
        {
            QAction *action = decoratorActionAt( indexAt( helpEvent->pos() ), helpEvent->pos() );
            if( action )
            {
                QToolTip::showText( helpEvent->globalPos(), action->toolTip() );
                event->accept();
                return true;
            }
        }
    }

    return PrettyTreeView::viewportEvent( event );
}

QList<QAction *>
PlaylistBrowserNS::PlaylistBrowserView::actionsFor( QModelIndexList indexes )
{
    QActionList actions;
    foreach( QModelIndex idx, indexes )
    {
        QActionList idxActions = model()->data( idx,
                PrettyTreeRoles::DecoratorRole ).value<QActionList>();
        //only add unique actions model is responsible for making them unique
        foreach( QAction *action, idxActions )
        {
            if( !actions.contains( action ) )
                actions << action;
        }
    }
    return actions;
}

void
PlaylistBrowserNS::PlaylistBrowserView::setNewFolderAction( KAction *action )
{
    m_addFolderAction = action;
}

void
PlaylistBrowserNS::PlaylistBrowserView::currentChanged( const QModelIndex &current,
                                                        const QModelIndex &previous )
{
    Q_UNUSED( previous )
    emit currentItemChanged( current );
    Amarok::PrettyTreeView::currentChanged( current, previous );
}

void
PlaylistBrowserNS::PlaylistBrowserView::appendAndPlay( const QModelIndex &index )
{
    appendAndPlay( QModelIndexList() << index );
}

void
PlaylistBrowserNS::PlaylistBrowserView::appendAndPlay( const QModelIndexList &list )
{
    performActionNamed( "appendAction", list );
}

void
PlaylistBrowserNS::PlaylistBrowserView::deletePlaylistsTracks( const QModelIndexList &list )
{
    performActionNamed( "deleteAction", list );
}

void
PlaylistBrowserNS::PlaylistBrowserView::performActionNamed( const QString &name,
                                                            const QModelIndexList &list )
{
    QActionList actions = actionsFor( list );

    foreach( QAction *action, actions )
    {
        if( action->objectName() == name )
            action->trigger();
        action->setData( QVariant() );  // reset data of all actions
    }
}

#include "PlaylistBrowserView.moc"
