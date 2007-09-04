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
#include "ServiceListModel.h"

#include <QIcon>
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

    painter->setRenderHint ( QPainter::Antialiasing );


    painter->drawRoundRect( option.rect.topLeft().x(), option.rect.topLeft().y() ,250,100, 10 ,10 );

    if (option.state & QStyle::State_Selected)
        painter->setPen(Qt::blue);
    else 
        painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 16));

    painter->drawPixmap( option.rect.topLeft() + QPoint( 2, 2 ) , index.data( Qt::DecorationRole ).value<QIcon>().pixmap( 50, 50 ) );

    painter->drawText( option.rect.topLeft() + QPoint( 53, 33 ) , index.data( Qt::DisplayRole ).toString() );

    painter->setFont(QFont("Arial", 12));
    
    QRectF textRect;

    textRect.setLeft( option.rect.topLeft().x() + 4 );
    textRect.setTop( option.rect.topLeft().y() + 46 );
    textRect.setWidth( 250 );
    textRect.setHeight( 100 - textRect.top() );

    painter->drawText ( textRect, Qt::TextWordWrap, index.data( ShortDescriptionRole ).toString() );


    painter->restore();

}

QSize ServiceListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    DEBUG_BLOCK

    return QSize ( 250, 100 );

    

}


