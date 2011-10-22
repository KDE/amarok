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

#include "AmazonShoppingCartView.h"

AmazonShoppingCartView::AmazonShoppingCartView( QWidget *parent ) :
    QListView( parent )
{
}

void AmazonShoppingCartView::keyPressEvent( QKeyEvent *event )
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
