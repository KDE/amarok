/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include <QMouseEvent>

#include <KMenu>

// TODO: context menu actions, enable drag and drop to playlist

AmazonItemTreeView::AmazonItemTreeView( QWidget *parent ) :
    Amarok::PrettyTreeView( parent )
{
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

    QAction *addToCartAction = new QAction( QString( i18n( "Add to Cart" ) ), &menu );
    actions.append( addToCartAction );
    connect( addToCartAction, SIGNAL( triggered() ), this, SLOT( addToCartAction() ) );

    AmazonItemTreeModel *amazonModel;
    amazonModel = dynamic_cast<AmazonItemTreeModel*>( model() );

    if( !amazonModel )
    {
        menu.exec( actions, event->globalPos() );
        event->accept();
        return;
    }

    if( amazonModel->isAlbum( index ) )
    {
        QAction *getDetailsAction = new QAction( QString( i18n( "Details..." ) ), &menu );
        actions.append( getDetailsAction );
        connect( getDetailsAction, SIGNAL( triggered() ), this, SLOT( itemActivatedAction() ) );
    }
    else // track
    {
        QAction *addToPlaylistAction = new QAction( QString( i18n( "Add Preview to Playlist" ) ), &menu );
        actions.append( addToPlaylistAction );
        connect( addToPlaylistAction, SIGNAL( triggered() ), this, SLOT( itemActivatedAction() ) );
    }

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

void AmazonItemTreeView::startDrag( Qt::DropActions supportedActions )
{
    // TODO: WIP
    DEBUG_BLOCK
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
    }
}

//CollectionTreeItem* AmazonItemTreeView::getItemFromIndex( QModelIndex &index )
//{
//    if( !index.isValid() )
//    {
//        return 0;
//    }

//    return static_cast<CollectionTreeItem*>( filteredIndex.internalPointer() );
//}

void AmazonItemTreeView::selectionChanged( const QItemSelection &selected, const QItemSelection &deselected )
{
    QTreeView::selectionChanged( selected, deselected ); // to avoid repainting problems
    QModelIndexList indexes = selected.indexes();

    if( indexes.count() < 1 )
        return;

    emit( itemSelected( indexes[0] ) ); // emit the QModelIndex
}

void AmazonItemTreeView::addToCartAction()
{
    emit addToCart();
}

void AmazonItemTreeView::itemActivatedAction()
{
    QModelIndexList indexes = selectedIndexes();

    if( indexes.count() < 1 )
        return;

    emit itemDoubleClicked( indexes[0] ); // same behaviour
}
