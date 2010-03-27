/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#include "VerticalLayout.h"

#include "core/support/Debug.h"

#include <plasma/applet.h>

namespace Context
{


class VerticalLayout::Private
{
public:
    QList<QGraphicsLayoutItem*> children;
    QRectF geometry;
};

VerticalLayout::VerticalLayout(QGraphicsLayoutItem *parent)
    : QGraphicsLayout(parent),
      d(new Private)
{
}

VerticalLayout::~VerticalLayout()
{
    debug() << "help help, I am being repressed: " << this;
    delete d;
}

void
VerticalLayout::addItem( QGraphicsLayoutItem *item )
{
    if (d->children.contains(item)) {
        return;
    }

    d->children << item;
    relayout();
}

void
VerticalLayout::removeItem( QGraphicsLayoutItem *item )
{
    DEBUG_BLOCK
    if (!item) {
        return;
    }

    debug() << "GOT CHILD ITEM TO REMOVE";
    d->children.removeAll( item );
    relayout();
}

int
VerticalLayout::indexOf( QGraphicsLayoutItem *item ) const
{
    return d->children.indexOf( item );
}

QGraphicsLayoutItem*
VerticalLayout::itemAt( int i ) const
{
    return d->children[ i ];
}

int
VerticalLayout::count() const
{
    return d->children.count();
}

QGraphicsLayoutItem*
VerticalLayout::takeAt( int i )
{
    QGraphicsLayoutItem* item = d->children.takeAt(i);
    relayout();
    return item;
}

void
VerticalLayout::removeAt( int i )
{
    d->children.removeAt(i);
}

void VerticalLayout::relayout()
{

    QRectF rect = geometry().adjusted( 0, 0, 0, 0 );

//    debug() << "VerticalLayout::relayout laying out column in rect" << rect;
    qreal top = 0.0;
    qreal left = 5.0; //Plasma::Layout::margin( Plasma::LeftMargin );

    foreach( QGraphicsLayoutItem *child , d->children )
    {
        qreal height = 0.0;
        Plasma::Applet* a = dynamic_cast< Plasma::Applet * >(child);
        if( a )
            height = a->effectiveSizeHint( Qt::PreferredSize, QSizeF( rect.width(), -1 ) ).height();
        else
        {
            debug() << "BAD BAD BAD Vertical Layout is managing a non-Plasma::Applet!!!";
            height = effectiveSizeHint( Qt::PreferredSize ).height();
        }

        const QRectF newgeom( rect.topLeft().x() + left,
                              rect.topLeft().y() + top,
                                           rect.width() - left * 2,
                                            height );
                                                              
                                        top += height;

        debug() << "setting child geometry to" << newgeom;
        child->setGeometry( newgeom );
    }

}

void
VerticalLayout::setGeometry( const QRectF &geometry )
{
    d->geometry = geometry;
    relayout();
}

QRectF
VerticalLayout::geometry() const
{
    return d->geometry;
}

QSizeF
VerticalLayout::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_UNUSED(constraint);
    qreal height = 0.0;
    foreach( QGraphicsLayoutItem* child, d->children )
        height += child->effectiveSizeHint( which ).height();
    return QSizeF( geometry().width(), height );
}

}
