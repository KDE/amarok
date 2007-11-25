/*
*   Copyright 2007 by Robert Knight <robertknight@gmail.com>
*   Copyright 2007 by Leo Franchi <lfranchi@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License
*   as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ContextLayout.h"

#include <limits.h>
#include <math.h>

#include "kdebug.h"
#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QTimeLine>

#include <QtDebug>

#include "plasma/layouts/layoutanimator.h"

using namespace Context;

class ContextLayout::Private
{
public:
    Private() : columnWidth( -1 ) {}
    QList<LayoutItem*> items; 
    qreal columnWidth;
};

ContextLayout::ContextLayout(LayoutItem* parent)
    : Layout(parent)
    , d(new Private)
{
}
ContextLayout::~ContextLayout()
{
    delete d;
}

int ContextLayout::count() const
{
    return d->items.count();
}

void ContextLayout::addItem(LayoutItem* item)
{
    if (d->items.contains(item)) {
        return;
    }

    d->items << item;

    if (animator()) {
        animator()->setCurrentState(item,Plasma::LayoutAnimator::InsertedState);
    }

    item->setManagingLayout(this);
}
void ContextLayout::removeItem(LayoutItem* item)
{
    item->unsetManagingLayout(this);
    d->items.removeAll(item);

    if (animator()) {
        animator()->setCurrentState(item,Plasma::LayoutAnimator::RemovedState);
    }
}
int ContextLayout::indexOf(LayoutItem* item) const
{
    return d->items.indexOf(item);
}

Plasma::LayoutItem* ContextLayout::itemAt(int i) const
{
    return d->items[i];
}
QSizeF ContextLayout::sizeHint() const
{
    // TODO A proper algorithm here
    // 
    // Idea:  Return a size hint based on the golden ratio to
    //        make it aesthetically good
    //        eg. Longer side is 1.61x the length of the shorter side
    //

    // testing
    return QSizeF(500,500);
}

Plasma::LayoutItem* ContextLayout::takeAt(int i)
{
    return d->items.takeAt(i);
}

template <class T>
T qSum(const QList<T>& container) 
{
    T total = 0;
    foreach( const T& item , container ) {
        total += item; 
    }   
    return total;
}

void ContextLayout::relayout()
{
    QRectF rect = geometry().adjusted(margin(LeftMargin), margin(TopMargin), -margin(RightMargin), -margin(BottomMargin));

    qDebug() << "Context layout geometry set to " << geometry() << " using column width: " << d->columnWidth;

    const int columnCount = (int)(rect.width() / d->columnWidth);

    int insertColumn = 0;
    qreal rowPos = 0;
    qreal rowHeight = 0;

    // lay the items out in left-to-right , top-to-bottom order
    foreach( LayoutItem *item , d->items ) {
    
        const QSizeF& itemSize = item->sizeHint();

        int columnSpan = (int)ceil(itemSize.width() / d->columnWidth);

        if ( insertColumn + columnSpan > columnCount ) {
            // start a new row
            insertColumn = 0;
            rowPos += rowHeight + spacing();
        }

       // qDebug() << "Inserting item at column" << insertColumn 
       //          << "spanning" << columnSpan << "columns"
       //          << "with offset" << offset;


        // try to expand the item to fill its allocated number of columns
        qreal itemWidth = itemSize.width(); 
        const qreal idealWidth = columnSpan * d->columnWidth - spacing();
        if ( itemWidth < idealWidth && 
             idealWidth < item->maximumSize().width() ) {
             itemWidth = idealWidth; 
        }
       
        // calculate offset to horizontally center item 
        qreal offset = (columnSpan * d->columnWidth) - itemWidth;
        if ( insertColumn == 0 )
            offset -= spacing();  
        offset /= 2;

        // try to restrict the item width to the available geometry's
        // width
        if ( itemWidth > rect.width() ) {
            itemWidth = qMax(rect.width(),item->minimumSize().width());
            offset = 0;
        }        

        // position the item
        qreal itemHeight;
        if( item->hasHeightForWidth() )
            itemHeight = item->heightForWidth( itemWidth );
        else
            itemHeight = itemSize.height(); // this is not good, applets should provide heightForWidth
            
        const QRectF newGeometry(rect.left() + insertColumn * d->columnWidth + offset,
                                 rect.top() + rowPos,
                                 itemWidth,
                                 itemHeight );

        rowHeight = qMax(rowHeight,itemHeight);
        insertColumn += columnSpan;

        kDebug() << "Setting a child item geometry to:" << newGeometry;
        if ( animator() )
            animator()->setGeometry( item , newGeometry );
        else
            item->setGeometry( newGeometry );
    }

    startAnimation();
}

Qt::Orientations ContextLayout::expandingDirections() const
{
    return Qt::Vertical | Qt::Horizontal;
}

qreal ContextLayout::columnWidth() const
{
    return d->columnWidth;
}

void ContextLayout::setColumnWidth( const qreal width )
{
    d->columnWidth = width;
}
