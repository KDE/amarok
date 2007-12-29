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

#include "VerticalLayout.h"

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
    Private() : columnWidth( -1 )
     {}

    
    // even out columns. this checks if any one column can be made shorter by
    // moving the last applet to another column
    void balanceColumns()
    {
        //     DEBUG_BLOCK
        int numColumns = columns.size();
        if( numColumns == 1 ) // no balancing to do :)
            return;

        bool found = true;
        while( found )
        {
            qreal maxHeight  = -1; int maxColumn = -1;
            for( int i = 0; i < numColumns; i++ )
            {
                if( columns[ i ]->sizeHint().height() > maxHeight )
                {
                    maxHeight = columns[ i ]->sizeHint().height();
                    maxColumn = i;
                }
            }

            if( maxHeight == 0 ) // no applets
                return;
            if( columns[ maxColumn ]->count() == 1 ) // if the largest column only has
                return; // one applet, we can't do anything more

                qreal maxAppletHeight = columns[ maxColumn ]->itemAt( columns[ maxColumn ]->count() - 1 )->sizeHint().height() + 10;
            // HACK!
            // adding 10 is needed to cover the borders/padding... i can't get the exact
            // value from Plasma::VBoxLayout because it's not exposed. arg!


//             kDebug() << "found maxHeight:" << maxHeight << "and maxColumn:" << maxColumn << "and maxAppletHeight" << maxAppletHeight;
            found = false;
            for( int i = 0; i < numColumns; i++ )
            {
                if( i == maxColumn ) continue; // don't bother
//                     kDebug() << "checking for column" << i << "of" << numColumns - 1;
//                 kDebug() << "doing math:" << columns[ i ]->sizeHint().height() << "+" << maxAppletHeight;
                qreal newColHeight = columns[ i ]->sizeHint().height() + maxAppletHeight;
//                 kDebug() << "checking if newColHeight:" << newColHeight << "is less than:" << maxHeight;
                if( newColHeight < maxHeight ) // found a new place for this applet
                {
//                     kDebug() << "found new place for an applet: column" << i;
                    columns[ i ]->addItem( columns[ maxColumn ]->takeAt( columns[ maxColumn ]->count() - 1 ) );
                    found = true;
                    break;
                }
            }
        }
    }

    QList< VerticalLayout* > columns;
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
    int total = 0;
    for( int i = 0; i < d->columns.count(); i++ )
        total += d->columns[ i ]->count();
    
    return total;
}

void ContextLayout::addItem(LayoutItem* item)
{
    if( d->columns.size() == 0 )
        d->columns << new VerticalLayout( this );
    
    int smallestColumn = 0, min = (int)d->columns[ 0 ]->sizeHint().height();
    for( int i = 1; i < d->columns.size(); i++ ) // find shortest column to put
    {                                           // the applet in
//         kDebug() << "comparing this column, size:" << d->columns[ i ]->sizeHint().height();
        if( d->columns[ i ]->sizeHint().height() < min )
            smallestColumn = i;
    }
//     kDebug() << "smallest column is" << smallestColumn << "th (" << min << ")" << "of" << d->columns.size();
    
//     kDebug() << "found" << d->columns.size() << " column, adding applet to column:" << smallestColumn;
    d->columns[ smallestColumn ]->addItem( item );
    if (animator()) {
        animator()->setCurrentState(item,Plasma::LayoutAnimator::InsertedState);
    }
    item->setManagingLayout( d->columns[ smallestColumn ] );

    relayout();
}

void ContextLayout::removeItem(LayoutItem* item)
{
    if(!item) {
        return;
    }

    item->unsetManagingLayout(this);

    if (animator()) {
        animator()->setCurrentState(item,Plasma::LayoutAnimator::RemovedState);
    }

    // find item and remove it
    for( int i = 0; i < d->columns.size(); i++ )
    {
        if( d->columns[ i ]->indexOf( item ) != 0 )
            d->columns[ i ]->removeItem( item );
    }
    
}
int ContextLayout::indexOf(LayoutItem* item) const
{
    int count = 0;
    for( int i = 0; i < d->columns.size(); i++ )
    {
        for( int k = 0; k < d->columns[ i ]->count(); k++ )
        {
            if( item == d->columns[ i ]->itemAt( k ) )
                return count;
            count++;
        }
    }
    return 0;
}

Plasma::LayoutItem* ContextLayout::itemAt(int at) const
{
    int count = 0;
    for( int i = 0; i < d->columns.size(); i++ )
    {
        for( int k = 0; k < d->columns[ i ]->count(); k++ )
        {
            if( count == at)
                return d->columns[ i ]->itemAt( k );
            count++;
        }
    }
    return 0;
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

Plasma::LayoutItem* ContextLayout::takeAt(int at)
{
    int count = 0;
    for( int i = 0; i < d->columns.size(); i++ )
    {
        for( int k = 0; k < d->columns[ i ]->count(); k++ )
        {
            if( count == at )
                return d->columns[ i ]->takeAt( k );
            count++;
        }
    }
    return 0;
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
    const int numColumns = qMax( (int)(rect.width() / d->columnWidth), 1 );    //use at least one column

    if( numColumns > d->columns.size() ) // need to make more columns
    {
        for( int i = d->columns.size(); i < numColumns; i++ )
            d->columns << new VerticalLayout( this );
    } if( numColumns < d->columns.size() ) // view was shrunk
    {
//         kDebug() << "gotta shrink!";
        VerticalLayout* column = d->columns[ d->columns.size() - 1 ];
        d->columns.removeAt( d->columns.size() - 1 );
        for( int i = 0; i < column->count() ; i++ )
        {
//             kDebug() << "trying to put away an item";
            LayoutItem* applet = column->takeAt( i );
            int smallestColumn = 0, min = (int)d->columns[ 0 ]->sizeHint().height();
            for( int i = 1; i < d->columns.size(); i++ ) // find shortest column to put
            {                                           // the applet in
            if( d->columns[ i ]->sizeHint().height() < min )
                    smallestColumn = i;
            }
//             kDebug() << "found column for item:" << smallestColumn;
            d->columns[ smallestColumn ]->addItem( applet );
        }
    }
    
    qreal columnWidth = rect.width() / numColumns;
//     columnWidth -= ( numColumns - 1 ) * margin( Plasma::Layout::LeftMargin ); // make room between columns

    for( int i = 0; i < numColumns; i++ ) // lay out columns
    {
//         kDebug() << "setting columns to width:" << columnWidth;
        QPointF pos( ( ( i + 1 ) * margin( Plasma::Layout::LeftMargin  ) ) + ( i * columnWidth ), margin( Plasma::Layout::LeftMargin ) );
        QSizeF size( columnWidth, qMax( d->columns[ i ]->sizeHint().height(),
                                                         rect.height() ) );
         d->columns[ i ]->setGeometry( QRectF( pos, size ) );
    }
//     kDebug() << "columns laid out, now balancing";
    d->balanceColumns();
//     foreach( VerticalLayout* column, d->columns )
//         column->setGeometry( column->geometry() );
//     kDebug() << "result is we have:" << d->columns.size() << "columns:";
//     foreach( VerticalLayout* column, d->columns )
//         kDebug() << "column rect:" << column->geometry() <<  "# of children:" << column->count();
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

