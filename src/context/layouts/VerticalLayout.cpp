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

#include "plasma/layouts/layoutitem.h"
#include "plasma/layouts/layoutanimator.h"

namespace Context
{


class VerticalLayout::Private
{
public:
    QList<LayoutItem*> children;
    QRectF geometry;
};

VerticalLayout::VerticalLayout(LayoutItem *parent)
    : Layout(parent),
      d(new Private)
{
}

VerticalLayout::~VerticalLayout()
{
    delete d;
}

Qt::Orientations VerticalLayout::expandingDirections() const
{
    return Qt::Vertical;
}

void VerticalLayout::addItem(LayoutItem *item)
{
    if (d->children.contains(item)) {
        return;
    }

    d->children << item;
    item->setManagingLayout(this);
    relayout();
}

void VerticalLayout::removeItem(LayoutItem *item)
{
    if (!item) {
        return;
    }

    d->children.removeAll(item);
    item->unsetManagingLayout(this);
    relayout();
}

int VerticalLayout::indexOf(LayoutItem *item) const
{
    return d->children.indexOf(item);
}

Plasma::LayoutItem* VerticalLayout::itemAt(int i) const
{
    return d->children[i];
}

int VerticalLayout::count() const
{
    return d->children.count();
}

Plasma::LayoutItem* VerticalLayout::takeAt(int i)
{
    Plasma::LayoutItem* item = d->children.takeAt(i);
    relayout();
    return item;
}

void VerticalLayout::relayout()
{
    
    QRectF rect = geometry().adjusted(margin(Plasma::LeftMargin), margin(Plasma::TopMargin), margin(Plasma::RightMargin), margin(Plasma::BottomMargin));

//     debug() << "laying out column in rect::" << rect;

    qreal topleft = 0.0;

    foreach (LayoutItem *child , d->children) {
        qreal height = 0.0;
        
        if( child->hasHeightForWidth() )
            height = child->heightForWidth( rect.width() );
        else
            height = sizeHint().height();
        
        const QRectF newgeom( rect.topLeft().x(),
                                            rect.topLeft().y() + topleft,
                                            rect.width(),
                                            height );
                                            
//         debug() << "laying out child item with geometry:" << newgeom;
                                            
        topleft += height + spacing();;
        
        if ( animator() )
            animator()->setGeometry( child , newgeom );
        else
            child->setGeometry( newgeom );
        
    }

}

void VerticalLayout::setGeometry(const QRectF &geometry)
{
    d->geometry = geometry;
    relayout();
}

QRectF VerticalLayout::geometry() const
{
    return d->geometry;
}

QSizeF VerticalLayout::sizeHint() const
{
    qreal height = 0.0;
    foreach( LayoutItem* child, d->children )
        height += child->sizeHint().height();
    return QSizeF( geometry().width(), height );
}

}

void Context::VerticalLayout::releaseManagedItems()
{
    //FIXME!!
}
