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

#define DEBUG_PREFIX "UserPlaylistTreeView"

#include "UserPlaylistTreeView.h"

#include "core/support/Debug.h"
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
#include "UserPlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"

#include <KAction>
#include <KGlobalSettings>
#include <KMenu>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>

PlaylistBrowserNS::UserPlaylistTreeView::UserPlaylistTreeView( QAbstractItemModel *model, QWidget *parent )
    : Amarok::PrettyTreeView( parent )
    , m_pd( 0 )
    , m_addGroupAction( 0 )
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
    setAnimated( true );
    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );

    The::paletteHandler()->updateItemView( this );

    //Give line edits a solid background color as any edit delegates will otherwise inherit the transparent base color,
    //which is bad as the line edit is drawn on top of the original name, leading to double text while editing....
    QPalette p = The::paletteHandler()->palette();
    QColor c = p.color( QPalette::Base );
    setStyleSheet("QLineEdit { background-color: " + c.name() + " }");
}


PlaylistBrowserNS::UserPlaylistTreeView::~UserPlaylistTreeView()
{
}

void
PlaylistBrowserNS::UserPlaylistTreeView::setModel( QAbstractItemModel *model )
{
    connect( model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( edit( QModelIndex ) ) );
    Amarok::PrettyTreeView::setModel( model );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
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

    bool prevExpandState = isExpanded( index );

    // This will toggle the expansion of the current item when clicking
    // on the fold marker but not on the item itself. Required here to
    // enable dragging.
    Amarok::PrettyTreeView::mousePressEvent( event );

    m_expandToggledWhenPressed = ( prevExpandState != isExpanded(index) );
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
        m_pd = 0;
    }

    if( !m_expandToggledWhenPressed &&
        event->button() != Amarok::contextMouseButton() &&
        event->modifiers() == Qt::NoModifier &&
        KGlobalSettings::singleClick() &&
        model()->hasChildren( index ) )
    {
        m_expandToggledWhenPressed = !m_expandToggledWhenPressed;
        setCurrentIndex( index );
        setExpanded( index, !isExpanded( index ) );
        event->accept();
        return;
    }
    Amarok::PrettyTreeView::mouseReleaseEvent( event );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() || event->modifiers() )
    {
        Amarok::PrettyTreeView::mouseMoveEvent( event );
        return;
    }
    event->accept();
}

void
PlaylistBrowserNS::UserPlaylistTreeView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    if( !model()->hasChildren( index ) )
    {
        QList<QAction *> actions =
            index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionRole ).value<QList<QAction *> >();
        if( actions.count() > 0 )
        {
            //HACK execute the first action assuming it's load
            actions.first()->trigger();
            actions.first()->setData( QVariant() );
        }
    }
    Amarok::PrettyTreeView::mouseDoubleClickEvent( event );
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
PlaylistBrowserNS::UserPlaylistTreeView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
        {
            foreach( const QModelIndex &selectedIdx, selectedIndexes() )
                model()->removeRow( selectedIdx.row(), selectedIdx.parent() );
            return;
        }
     }
     Amarok::PrettyTreeView::keyPressEvent( event );
}

void PlaylistBrowserNS::UserPlaylistTreeView::contextMenuEvent( QContextMenuEvent * event )
{
    QModelIndexList indices = selectionModel()->selectedIndexes();

    QList<QAction *> actions = actionsFor( indices );

    if( actions.isEmpty() )
        return;

    KMenu menu;
    foreach( QAction *action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    if( indices.count() == 0 )
        menu.addAction( m_addGroupAction );

    menu.exec( mapToGlobal( event->pos() ) );

    //We keep the items that the actions need to be applied to in the actions private data.
    //Clear the data from all actions now that the context menu has executed.
    foreach( QAction *action, actions )
        action->setData( QVariant() );
}

QList<QAction *>
PlaylistBrowserNS::UserPlaylistTreeView::actionsFor( QModelIndexList indexes )
{
    QList<QAction *> actions;
    foreach( QModelIndex idx, indexes )
    {
        QList<QAction *> idxActions =
         idx.data( PlaylistBrowserNS::MetaPlaylistModel::ActionRole ).value<QList<QAction *> >();
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
PlaylistBrowserNS::UserPlaylistTreeView::setNewGroupAction( KAction * action )
{
    m_addGroupAction = action;
}

#include "UserPlaylistTreeView.moc"
