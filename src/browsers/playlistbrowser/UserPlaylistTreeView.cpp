/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "UserPlaylistTreeView.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistController.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "MetaPlaylistModel.h"
#include "PaletteHandler.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "UserPlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"

#include <KAction>
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
{
    DEBUG_BLOCK
    setModel( model );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragDropMode( QAbstractItemView::DragDrop );
    setAcceptDrops( true );

    The::paletteHandler()->updateItemView( this );

    //Give line edits a solid background color as any edit delegates will otherwise inherit the transparent base color,
    //which is bad as the line edit is drawn on top of the original name, leading to double text while editing....
    QPalette p = The::paletteHandler()->palette();
    QColor c = p.color( QPalette::Base );
    setStyleSheet("QLineEdit { background-color: " + c.name() + " }");

    connect( m_model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( edit( QModelIndex ) ) );
}


PlaylistBrowserNS::UserPlaylistTreeView::~UserPlaylistTreeView()
{
}

void PlaylistBrowserNS::UserPlaylistTreeView::mousePressEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
        m_dragStartPosition = event->pos();

    QTreeView::mousePressEvent( event );
}

void PlaylistBrowserNS::UserPlaylistTreeView::mouseReleaseEvent( QMouseEvent * event )
{
    Q_UNUSED( event )

    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
    }
    m_pd = 0;
}

void PlaylistBrowserNS::UserPlaylistTreeView::mouseDoubleClickEvent( QMouseEvent * event )
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
    }
}

void PlaylistBrowserNS::UserPlaylistTreeView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    //Waah? when a parent item is dragged, startDrag is called a bunch of times
    static bool ongoingDrags = false;
    if( ongoingDrags )
        return;
    ongoingDrags = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {

        QModelIndexList indices = selectedIndexes();

        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
        if( mpm == 0 )
            return;
        QList<PopupDropperAction*> actions = mpm->actionsFor( indices );

        foreach( PopupDropperAction * action, actions ) {
            m_pd->addItem( The::popupDropperFactory()->createItem( action ), false );
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
    ongoingDrags = false;
}

void
PlaylistBrowserNS::UserPlaylistTreeView::keyPressEvent( QKeyEvent *event )
{
    Q_UNUSED( event )

    switch( event->key() )
    {
        case Qt::Key_Delete:
        {
            QModelIndex selectedIdx = selectedIndexes().first();
            m_model->removeRow( selectedIdx.row(), selectedIdx.parent() );
            return;
        }

        case Qt::Key_F2:
        {
            //can only rename if one is selected
            if( selectedIndexes().count() != 1 )
                return;
            event->accept();
            edit( selectedIndexes().first() );
            return;
        }
     }
     QTreeView::keyPressEvent( event );
}

void PlaylistBrowserNS::UserPlaylistTreeView::contextMenuEvent( QContextMenuEvent * event )
{
    QModelIndexList indices = selectionModel()->selectedIndexes();

    KMenu menu;

    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
    if( mpm == 0 )
        return;
    QList<PopupDropperAction *> actions = mpm->actionsFor( indices );

    if( actions.isEmpty() )
        return;

    foreach( PopupDropperAction * action, actions )
        menu.addAction( action );

    if( indices.count() == 0 )
        menu.addAction( m_addGroupAction );

    menu.exec( mapToGlobal( event->pos() ) );
}

void
PlaylistBrowserNS::UserPlaylistTreeView::setNewGroupAction( KAction * action )
{
    m_addGroupAction = action;
}

void
PlaylistBrowserNS::UserPlaylistTreeView::createNewGroup()
{
    PlaylistsInGroupsProxy *pigp = dynamic_cast<PlaylistsInGroupsProxy *>(m_model);
    if( pigp == 0 )
        return;
    QModelIndex idx = pigp->createNewGroup( QString("New Folder") );
    edit( idx );
}

#include "UserPlaylistTreeView.moc"
