/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#include <kstandarddirs.h>

#include <QIcon>
#include <QMenu>

AmazonItemTreeView::AmazonItemTreeView( QWidget *parent ) :
    Amarok::PrettyTreeView( parent )
    , m_pd( 0 )
{
    setDragDropMode( QAbstractItemView::DragOnly );
}


void
AmazonItemTreeView::setModel( QAbstractItemModel *model )
{
    Amarok::PrettyTreeView::setModel( model );
    header()->setStretchLastSection( false );
}


// protected

void
AmazonItemTreeView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex index = indexAt( event->pos() );

    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    QMenu menu( this );
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
    {
        actions.append( createAddToPlaylistAction() ); // this should be the first action
        actions.append( createSearchForAlbumAction() );
    }

    actions.append( createAddToCartAction() );
    actions.append( createDirectCheckoutAction() );

    menu.exec( actions, event->globalPos() );
    event->accept();
}

void
AmazonItemTreeView::mouseDoubleClickEvent( QMouseEvent *event )
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

void
AmazonItemTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pd )
    {
        m_pd->hide();
        connect( m_pd, SIGNAL(fadeHideFinished()), m_pd, SLOT(deleteLater()) );
        m_pd = 0;
    }

    Amarok::PrettyTreeView::mouseReleaseEvent( event );
}

void
AmazonItemTreeView::startDrag( Qt::DropActions supportedActions )
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
            detailsAction->setProperty( "popupdropper_svg_id", "loading" );
            m_pd->addItem( The::popupDropperFactory()->createItem( detailsAction ) );
        }
        else // track
        {
            QAction *addToPlaylistAction = createAddToPlaylistAction();
            addToPlaylistAction->setProperty( "popupdropper_svg_id", "append" );
            m_pd->addItem( The::popupDropperFactory()->createItem( addToPlaylistAction ) );

            QAction *searchForAlbumAction = createSearchForAlbumAction();
            addToPlaylistAction->setProperty( "popupdropper_svg_id", "collection" );
            m_pd->addItem( The::popupDropperFactory()->createItem( searchForAlbumAction ) );
        }

        QAction *addToCartAction = createAddToCartAction();
        addToCartAction->setProperty( "popupdropper_svg_id", "cart_in" );
        m_pd->addItem( The::popupDropperFactory()->createItem( addToCartAction ) );

        QAction *directCheckoutAction = createDirectCheckoutAction();
        directCheckoutAction->setProperty( "popupdropper_svg_id", "download" );
        m_pd->addItem( The::popupDropperFactory()->createItem( directCheckoutAction ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );

    if( m_pd )
    {
        connect( m_pd, SIGNAL(fadeHideFinished()), m_pd, SLOT(clear()) );
        m_pd->hide();
    }
}


// protected slots

void
AmazonItemTreeView::dataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
    Q_UNUSED( topLeft )
    Q_UNUSED( bottomRight )
    header()->setResizeMode( 1, QHeaderView::ResizeToContents );
    header()->setResizeMode( 0, QHeaderView::Stretch );
}

void
AmazonItemTreeView::selectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
    QTreeView::selectionChanged( selected, deselected ); // to avoid repainting problems
    QModelIndexList indexes = selected.indexes();

    if( indexes.count() < 1 )
        return;

    emit( itemSelected( indexes[0] ) ); // emit the QModelIndex
}

void
AmazonItemTreeView::itemActivatedAction()
{
    QModelIndexList indexes = selectedIndexes();

    if( indexes.count() < 1 )
        return;

    emit itemDoubleClicked( indexes[0] ); // same behaviour as double click
}

void
AmazonItemTreeView::searchForAlbumAction()
{
    QModelIndexList indexes = selectedIndexes();

    if( indexes.count() < 1 )
        return;

    // make sure we are working on a track
    AmazonItemTreeModel *amazonModel;
    amazonModel = dynamic_cast<AmazonItemTreeModel*>( model() );

    if( !amazonModel )
        return;

    if( amazonModel->isAlbum( indexes[0] ) )
        return;

    emit searchForAlbum( indexes[0] );
}

// private

QAction*
AmazonItemTreeView::createAddToCartAction()
{
    QAction *addToCartAction = new QAction( QIcon::fromTheme( "amarok_cart_add" ), QString( i18n( "Add to Cart" ) ), this );
    connect( addToCartAction, SIGNAL(triggered()), this, SIGNAL(addToCart()) );

    return addToCartAction;
}

QAction*
AmazonItemTreeView::createAddToPlaylistAction()
{
    QAction *addToPlaylistAction = new QAction( QIcon::fromTheme( "media-track-add-amarok" ), QString( i18n( "Add Preview to Playlist" ) ), this );
    connect( addToPlaylistAction, SIGNAL(triggered()), this, SLOT(itemActivatedAction()) );

    return addToPlaylistAction;
}

QAction*
AmazonItemTreeView::createDetailsAction()
{
    QAction *getDetailsAction = new QAction( QIcon( KStandardDirs::locate( "data", "amarok/images/loading1.png" ) ), QString( i18n( "Load Details..." ) ), this );
    connect( getDetailsAction, SIGNAL(triggered()), this, SLOT(itemActivatedAction()) );

    return getDetailsAction;
}

QAction*
AmazonItemTreeView::createDirectCheckoutAction()
{
    QAction *directCheckoutAction = new QAction( QIcon::fromTheme( "download-amarok" ), QString( i18n( "Direct Checkout" ) ), this );
    connect( directCheckoutAction, SIGNAL(triggered()), this, SIGNAL(directCheckout()) );

    return directCheckoutAction;
}

QAction*
AmazonItemTreeView::createSearchForAlbumAction()
{
    QAction *searchForAlbumAction = new QAction( QIcon::fromTheme( "media-optical-amarok" ), QString( i18n( "Search for Album..." ) ), this );
    connect( searchForAlbumAction, SIGNAL(triggered()), this, SLOT(searchForAlbumAction()) );

    return searchForAlbumAction;
}
