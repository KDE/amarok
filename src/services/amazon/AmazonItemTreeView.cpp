/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
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

#include "AmazonItemTreeModel.h"
#include "AmazonItemTreeView.h"

#include "AmarokMimeData.h"

#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "PopupDropperFactory.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"

#include <QHeaderView>
#include <QMouseEvent>

#include <KIcon>
#include <KMenu>

AmazonItemTreeView::AmazonItemTreeView( QWidget *parent ) :
    Amarok::PrettyTreeView( parent )
    , m_pd( 0 )
{
    setDragDropMode( QAbstractItemView::DragOnly );
}

void AmazonItemTreeView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex index = indexAt( event->pos() );

    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    KMenu menu( this );
    QList< QAction * > actions;

    AmazonItemTreeModel *amazonModel;
    amazonModel = dynamic_cast<AmazonItemTreeModel*>( model() );

    if( !amazonModel )
    {
        menu.exec( actions, event->globalPos() );
        event->accept();
        return;
    }

    if( amazonModel->isAlbum( index ) )
        actions.append( createDetailsAction() );
    else // track
        actions.append( createAddToPlaylistAction() ); // this should be the first action

    actions.append( createAddToCartAction() );

    menu.exec( actions, event->globalPos() );
    event->accept();
}

void AmazonItemTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    // for tracks: append them to the playlist
    // for albums: query for the album
    if( event->button() == Qt::MidButton )
    {
        event->accept();
        return;
    }

    QModelIndex index = indexAt( event->pos() );
    if( index.isValid() )
    {
        event->accept();
        emit itemDoubleClicked( index );
    }

}

void AmazonItemTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    Amarok::PrettyTreeView::mousePressEvent( event );
}

void AmazonItemTreeView::mouseMoveEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    // pass event to parent widget
    if( event->buttons() || event->modifiers() )
    {
        Amarok::PrettyTreeView::mouseMoveEvent( event );
        return;
    }
    event->accept();
}

void AmazonItemTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pd )
    {
        m_pd->hide();
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd = 0;
    }

    Amarok::PrettyTreeView::mouseReleaseEvent( event );
}

void AmazonItemTreeView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        AmazonItemTreeModel *amazonModel;
        amazonModel = dynamic_cast<AmazonItemTreeModel*>( model() );

        if( !amazonModel )
            return;

        if( amazonModel->isAlbum( indices.at( 0 ) ) )
        {
            QAction *detailsAction = createDetailsAction();
            // TODO: add correct icon here
            // detailsAction->setProperty( "popupdropper_svg_id", "load" );
            m_pd->addItem( The::popupDropperFactory()->createItem( detailsAction ) );
        }
        else // track
        {
            QAction *addToPlaylistAction = createAddToPlaylistAction();
            addToPlaylistAction->setProperty( "popupdropper_svg_id", "append" );
            m_pd->addItem( The::popupDropperFactory()->createItem( addToPlaylistAction ) );
        }

        m_pd->addItem( The::popupDropperFactory()->createItem( createAddToCartAction() ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );

    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }
}

void AmazonItemTreeView::dataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
    Q_UNUSED( topLeft )
    Q_UNUSED( bottomRight )
    header()->setResizeMode( 1, QHeaderView::ResizeToContents );
    header()->setResizeMode( 0, QHeaderView::Stretch );
}

void AmazonItemTreeView::selectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
    QTreeView::selectionChanged( selected, deselected ); // to avoid repainting problems
    QModelIndexList indexes = selected.indexes();

    if( indexes.count() < 1 )
        return;

    emit( itemSelected( indexes[0] ) ); // emit the QModelIndex
}

void AmazonItemTreeView::itemActivatedAction()
{
    QModelIndexList indexes = selectedIndexes();

    if( indexes.count() < 1 )
        return;

    emit itemDoubleClicked( indexes[0] ); // same behaviour as double click
}

void AmazonItemTreeView::setModel( QAbstractItemModel *model )
{
    Amarok::PrettyTreeView::setModel( model );
    header()->setStretchLastSection( false );
}

// private

QAction*
AmazonItemTreeView::createAddToCartAction()
{
    // TODO: add correct icon here
    QAction *addToCartAction = new QAction( QString( i18n( "Add to Cart" ) ), this );
    connect( addToCartAction, SIGNAL( triggered() ), this, SIGNAL( addToCart() ) );

    return addToCartAction;
}

QAction*
AmazonItemTreeView::createAddToPlaylistAction()
{
    QAction *addToPlaylistAction = new QAction( KIcon( "media-track-add-amarok" ), QString( i18n( "Add Preview to Playlist" ) ), this );
    connect( addToPlaylistAction, SIGNAL( triggered() ), this, SLOT( itemActivatedAction() ) );

    return addToPlaylistAction;
}

QAction*
AmazonItemTreeView::createDetailsAction()
{
    // TODO: add correct icon here
    QAction *getDetailsAction = new QAction( QString( i18n( "Load Details..." ) ), this );
    connect( getDetailsAction, SIGNAL( triggered() ), this, SLOT( itemActivatedAction() ) );

    return getDetailsAction;
}
