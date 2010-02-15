/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "UserPlaylistTreeView.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistController.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "MetaPlaylistModel.h"
#include "PaletteHandler.h"
#include "PopupDropperFactory.h"
#include "PlaylistTreeItemDelegate.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "UserPlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"

#include <KAction>
#include <KGlobalSettings>
#include <KMenu>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>

#include <typeinfo>

PlaylistBrowserNS::UserPlaylistTreeView::UserPlaylistTreeView( QAbstractItemModel *model, QWidget *parent )
    : Amarok::PrettyTreeView( parent )
    , m_model( model )
    , m_pd( 0 )
    , m_addGroupAction( 0 )
    , m_ongoingDrag( false )
    , m_dragMutex()
    , m_justDoubleClicked( false )
{
    DEBUG_BLOCK
    setModel( model );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setDragDropMode( QAbstractItemView::DragDrop );
    setAcceptDrops( true );
    setAnimated( true );
    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );

    The::paletteHandler()->updateItemView( this );

    //Give line edits a solid background color as any edit delegates will otherwise inherit the transparent base color,
    //which is bad as the line edit is drawn on top of the original name, leading to double text while editing....
    QPalette p = The::paletteHandler()->palette();
    QColor c = p.color( QPalette::Base );
    setStyleSheet("QLineEdit { background-color: " + c.name() + " }");

    connect( m_model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( edit( QModelIndex ) ) );

    connect( &m_clickTimer, SIGNAL( timeout() ), this, SLOT( slotClickTimeout() ) );
}


PlaylistBrowserNS::UserPlaylistTreeView::~UserPlaylistTreeView()
{
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mousePressEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( KGlobalSettings::singleClick() )
        setItemsExpandable( false );
    if( !index.parent().isValid() ) //not a root element, don't bother checking actions
    {
        Amarok::PrettyTreeView::mousePressEvent( event );
        return;
    }

    const int actionCount =
            index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole ).toInt();
    if( actionCount > 0 )
    {
        const QRect rect = PlaylistTreeItemDelegate::actionsRect( index );
        if( rect.contains( event->pos() ) )
            return;
    }

    Amarok::PrettyTreeView::mousePressEvent( event );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mouseReleaseEvent( QMouseEvent * event )
{
    const QModelIndex index = indexAt( event->pos() );
    if( !index.parent().isValid() ) // not a root element, don't bother checking actions
    {
        const int actionCount =
            index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole ).toInt();
        if( actionCount > 0 )
        {
            const QRect rect = PlaylistTreeItemDelegate::actionsRect( index );
            if( rect.contains( event->pos() ) )
            {
                QVariantList variantList =
                        index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionRole ).toList();
                if( variantList.isEmpty() )
                    return;

                QList<QAction*> actions = variantList.first().value<QList<QAction*> >();
                //hack: rect height == the width of one action's area.
                int indexOfActionToTrigger
                    = ( event->pos().x() - rect.left() ) / rect.height();
                debug() << "triggering action " << indexOfActionToTrigger;
                if( indexOfActionToTrigger >= actions.count() )
                {
                    debug() << "no such action";
                    return;
                }
                QAction *action = actions.value( indexOfActionToTrigger );
                if( action )
                    action->trigger();
                return;
            }
        }
    }

    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
    }
    m_pd = 0;

    setItemsExpandable( true );

    if( m_clickTimer.isActive() || m_justDoubleClicked )
    {
        //it's a double-click...so ignore it
        m_clickTimer.stop();
        m_justDoubleClicked = false;
        m_savedClickIndex = QModelIndex();
        event->accept();
        return;
    }

    m_savedClickIndex = indexAt( event->pos() );
    KConfigGroup cg( KGlobal::config(), "KDE" );
    m_clickTimer.start( cg.readEntry( "DoubleClickInterval", 400 ) );
    m_clickLocation = event->pos();
    Amarok::PrettyTreeView::mouseReleaseEvent( event );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() || event->modifiers() )
    {
        Amarok::PrettyTreeView::mouseMoveEvent( event );
        update();
        return;
    }
    QPoint point = event->pos() - m_clickLocation;
    KConfigGroup cg( KGlobal::config(), "KDE" );
    if( point.manhattanLength() > cg.readEntry( "StartDragDistance", 4 ) )
    {
        m_clickTimer.stop();
        slotClickTimeout();
        event->accept();
    }
    else
        Amarok::PrettyTreeView::mouseMoveEvent( event );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QModelIndex index = indexAt( event->pos() );

    if( index.isValid() )
    {
        QModelIndexList list;
        list << index;
        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
        if( mpm == 0 )
            return;
        mpm->loadItems( list, Playlist::LoadAndPlay );
        event->accept();
    }

    m_clickTimer.stop();
    //m_justDoubleClicked is necessary because the mouseReleaseEvent still
    //comes through, but after the mouseDoubleClickEvent, so we need to tell
    //mouseReleaseEvent to ignore that one event
    m_justDoubleClicked = true;
    setExpanded( index, !isExpanded( index ) );

    event->accept();
}

void PlaylistBrowserNS::UserPlaylistTreeView::startDrag( Qt::DropActions supportedActions )
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

    if( m_pd && m_pd->isHidden() )
    {

        QModelIndexList indices = selectedIndexes();

        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
        if( mpm == 0 )
            return;
        QList<QAction*> actions = mpm->actionsFor( indices );

        foreach( QAction * action, actions )
        {
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );
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

void
PlaylistBrowserNS::UserPlaylistTreeView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
        {
            foreach( const QModelIndex &selectedIdx, selectedIndexes() )
                m_model->removeRow( selectedIdx.row(), selectedIdx.parent() );
            return;
        }
     }
     Amarok::PrettyTreeView::keyPressEvent( event );
}

void PlaylistBrowserNS::UserPlaylistTreeView::contextMenuEvent( QContextMenuEvent * event )
{
    QModelIndexList indices = selectionModel()->selectedIndexes();

    KMenu menu;

    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
    if( mpm == 0 )
        return;
    QList<QAction *> actions = mpm->actionsFor( indices );

    if( actions.isEmpty() )
        return;

    foreach( QAction *action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    if( indices.count() == 0 )
        menu.addAction( m_addGroupAction );

    menu.exec( mapToGlobal( event->pos() ) );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::slotClickTimeout()
{
    m_clickTimer.stop();
    if( m_savedClickIndex.isValid() && KGlobalSettings::singleClick() )
    {
        setExpanded( m_savedClickIndex, !isExpanded( m_savedClickIndex ) );
    }
    m_savedClickIndex = QModelIndex();
}

void
PlaylistBrowserNS::UserPlaylistTreeView::setNewGroupAction( KAction * action )
{
    m_addGroupAction = action;
}

void
PlaylistBrowserNS::UserPlaylistTreeView::createNewGroup()
{
    PlaylistsInGroupsProxy *pigp = qobject_cast<PlaylistsInGroupsProxy *>( model() );
    if( pigp == 0 )
        return;
    QModelIndex idx = pigp->createNewGroup( i18np( "New Folder", "New Folder (%1)", 1 ) );
    edit( idx );
}

#include "UserPlaylistTreeView.moc"
