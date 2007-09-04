/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ServiceListDelegate.h"

#include "debug.h"
#include "servicebase.h"

#include <QPainter>

ServiceListDelegate::ServiceListDelegate()
 : QItemDelegate()
{
}

ServiceListDelegate::~ServiceListDelegate()
{
}

void ServiceListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    DEBUG_BLOCK

    //ServiceBase * service = static_cast< ServiceBase * >( index.internalPointer() );
    //if ( ! service ) return;

    debug() << "Look ma' I am painting!";
 
    painter->save();
 
    painter->setPen(Qt::gray);

    if (option.state & QStyle::State_Selected)
        painter->drawRoundRect( option.rect.topLeft().x(), option.rect.topLeft().y() ,200,200, 10 ,10 );
    else
        painter->drawRoundRect( option.rect.topLeft().x(), option.rect.topLeft().y() ,200,100, 10 ,10 );

    if (option.state & QStyle::State_Selected)
        painter->setPen(Qt::blue);
    else 
        painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 16));
    painter->drawText( option.rect.topLeft() + QPoint( 0, 16 ) , index.data( Qt::DisplayRole ).toString() );

    painter->restore();

}

QSize ServiceListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    //Q_UNUSED( option );
    Q_UNUSED( index );

    DEBUG_BLOCK

    if (option.state & QStyle::State_Selected)
        return QSize ( 200, 200 );
    else 
        return QSize ( 200, 100 );
    
    

}


