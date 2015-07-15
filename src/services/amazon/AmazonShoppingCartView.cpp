/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#include "AmazonShoppingCartView.h"

#include "klocalizedstring.h"

#include <QIcon>
#include <QMenu>

AmazonShoppingCartView::AmazonShoppingCartView( QWidget *parent ) :
    QListView( parent )
{
    setAlternatingRowColors( true );
    setUniformItemSizes( true );
}

void
AmazonShoppingCartView::keyPressEvent( QKeyEvent *event )
{
    if( event->key() == Qt::Key_Delete )
    {
        QModelIndex index = currentIndex();
        int row = index.row();
        int count = 1;

        model()->removeRows( row, count, index );
        event->accept();

        return;
    }

    QListView::keyPressEvent( event );
}


/* protected */

void
AmazonShoppingCartView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    QMenu menu( this );
    QList< QAction * > actions;

    QAction *removeFromCartAction = new QAction( QIcon::fromTheme( "amarok_cart_remove" ), QString( i18n( "Remove from Cart" ) ), &menu );
    actions.append( removeFromCartAction );
    connect( removeFromCartAction, SIGNAL(triggered()), this, SLOT(removeFromCartAction()) );

    menu.exec( actions, event->globalPos() );
    event->accept();
}


/* protected slots */

void
AmazonShoppingCartView::removeFromCartAction()
{
    QModelIndex index = currentIndex();
    int row = index.row();
    int count = 1;

    model()->removeRows( row, count, index );
}
