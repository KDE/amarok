/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "EditDeleteComboBoxView.h"

#include "Debug.h"
#include "EditDeleteDelegate.h"

#include <QModelIndex>
#include <QMouseEvent>

EditDeleteComboBoxView::EditDeleteComboBoxView( QWidget* parent )
 : QListView( parent )
{
}

void EditDeleteComboBoxView::mousePressEvent( QMouseEvent *event )
{
    DEBUG_BLOCK
    
    QModelIndex index = indexAt( event->pos() );
    QPoint mousePressPos = event->pos();
    mousePressPos.rx() += horizontalOffset();
    mousePressPos.ry() += verticalOffset();

    if ( EditDeleteDelegate::hitsEdit( mousePressPos, rectForIndex( index ) ) )
        emit( editItem( index.data().toString() ) );
    else if ( EditDeleteDelegate::hitsDelete( mousePressPos, rectForIndex( index ) ) )
        emit( deleteItem( index.data().toString() ) );

    QListView::mousePressEvent( event );
}

#include "EditDeleteComboBoxView.moc"
