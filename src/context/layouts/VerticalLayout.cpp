/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "VerticalLayout.h"

#include "debug.h"

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
    debug() << "help help, I'm being repressed: " << this;
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
    if (!item) {
        return;
    }

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
    DEBUG_BLOCK

    QRectF rect = geometry().adjusted( 0, 0, 0, 0 );

    debug() << "VerticalLayout::relayout laying out column in rect" << rect;
    qreal top = 10.0;
    qreal left = 10.0; //Plasma::Layout::margin( Plasma::LeftMargin );

    foreach( QGraphicsLayoutItem *child , d->children )
    {
        qreal height = 0.0;

        if( Plasma::Applet *a = dynamic_cast<Plasma::Applet *>(child) )
        {
            if( a )
                 height = a->effectiveSizeHint( Qt::PreferredSize, QSizeF( rect.width(), -1 ) ).height();
        }
        else
            height = effectiveSizeHint( Qt::PreferredSize ).height();

        const QRectF newgeom( rect.topLeft().x() + left,
                              rect.topLeft().y() + top,
                                           rect.width() - 6,
                                            height );

        top += height /*+ spacing()*/;

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
    qreal height = 0.0;
    foreach( QGraphicsLayoutItem* child, d->children )
        height += child->effectiveSizeHint( which ).height();
    return QSizeF( geometry().width(), height );
}

}
