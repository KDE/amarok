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
 
#include "EditDeleteDelegate.h"

#include "Debug.h"

#include <KIcon>

#include <QPainter>
#include <QPixmap>

#define ICON_WIDTH 16
#define MARGIN 3


EditDeleteDelegate::EditDeleteDelegate( QObject * parent )
    : QStyledItemDelegate( parent )
{
}

void EditDeleteDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    int y = option.rect.y();

    //use normal painting, sizeHint has ensured that we have enough room for both text and icons.
    QStyledItemDelegate::paint( painter, option, index );

    int iconOffset = option.rect.width() - ( MARGIN * 3 + ICON_WIDTH * 2 );

    if ( option.state & QStyle::State_Selected )
    {
        //paint our custom stuff in the leftover space
        //but only if this is the item that the mouse is over...

        const KIcon editIcon( "configure" );
        const KIcon deleteIcon( "edit-delete" );

        QPixmap editPixmap = editIcon.pixmap( ICON_WIDTH, ICON_WIDTH );
        QPixmap deletePixmap = deleteIcon.pixmap( ICON_WIDTH, ICON_WIDTH );

        painter->drawPixmap( iconOffset + MARGIN, y, ICON_WIDTH, ICON_WIDTH, editPixmap );
        painter->drawPixmap( iconOffset + MARGIN *2 + ICON_WIDTH, y, ICON_WIDTH, ICON_WIDTH, deletePixmap );
    }
}

QSize EditDeleteDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    const QSize orgSize = QStyledItemDelegate::sizeHint( option, index );
    const QSize addSize( MARGIN * 3 + ICON_WIDTH * 2, 0 );

    return orgSize + addSize;
}

bool EditDeleteDelegate::hitsEdit( const QPoint &point, const QRect &rect )
{
    DEBUG_BLOCK
    //we considder the icon to be full height, so we just count from the right edge.
    int right = ( rect.x() + rect.width() ) - ( MARGIN * 2 + ICON_WIDTH );
    int left = right - ICON_WIDTH;
    return ( point.x() > left ) && ( point.x() < right );
}

bool EditDeleteDelegate::hitsDelete( const QPoint &point, const QRect &rect )
{
    DEBUG_BLOCK
    //we considder the icon to be full height, so we just count from the right edge.
    int right = ( rect.x() + rect.width() ) - MARGIN;
    int left = right - ICON_WIDTH;
    return ( point.x() > left ) && ( point.x() < right );
}

