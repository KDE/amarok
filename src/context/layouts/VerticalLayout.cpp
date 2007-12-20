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
    
    QRectF rect = geometry().adjusted(2*margin(Plasma::Layout::LeftMargin), 2*margin(Plasma::Layout::TopMargin), -.38*margin(Plasma::Layout::RightMargin), -.8*margin(Plasma::Layout::BottomMargin));

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
                                            
        debug() << "laying out child item with geometry:" << newgeom;
                                            
        topleft += height + spacing();;
        
        if ( animator() )
            animator()->setGeometry( child , newgeom );
        else
            child->setGeometry( newgeom );
        
    }

}

QRectF VerticalLayout::geometry() const
{
    if (parent()) {
        return parent()->geometry();
    }

    return QRectF(QPointF(0, 0), maximumSize());
}

QSizeF VerticalLayout::sizeHint() const
{
    if (parent()) {
        //kDebug() << "returning size hint from freelayout of" <<  parent()->geometry().size();
        return parent()->geometry().size();
    }

    //kDebug() << "returning size hint from freelayout of" << maximumSize();
    return maximumSize();
}

}
