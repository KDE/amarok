/****************************************************************************************
 * Copyright (c) 2007 Robert Knight <robertknight@gmail.com>                            *
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

#include "ContextLayout.h"

#include "Applet.h"
#include "core/support/Debug.h"
#include "VerticalLayout.h"

#include <limits.h>
#include <math.h>

#include "kdebug.h"
#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QTimeLine>

#include <QtDebug>

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
        int numColumns = columns.size();
        if( numColumns == 1 ) // no balancing to do :)
            return;

        bool found = true;
        while( found )
        {
            qreal maxHeight  = -1; int maxColumn = -1;
            for( int i = 0; i < numColumns; i++ )
            {
                if( columns[ i ]->effectiveSizeHint( Qt::MaximumSize ).height() > maxHeight )
                {
                    maxHeight = columns[ i ]->effectiveSizeHint( Qt::MaximumSize ).height();
                    maxColumn = i;
                }
            }

            if( maxHeight == 0 ) // no applets
                return;
            if( columns[ maxColumn ]->count() == 1 ) // if the largest column only has
                return; // one applet, we can't do anything more

            qreal maxAppletHeight = columns[ maxColumn ]->itemAt( columns[ maxColumn ]->count() - 1 )->effectiveSizeHint( Qt::MaximumSize ).height() + 10;
            // HACK!
            // adding 10 is needed to cover the borders/padding... i can't get the exact
            // value from VerticalLayout because it's not exposed. arg!


            found = false;
            for( int i = 0; i < numColumns; i++ )
            {
                if( i == maxColumn ) continue; // don't bother
                qreal newColHeight = columns[ i ]->effectiveSizeHint( Qt::MaximumSize ).height() + maxAppletHeight;
                if( newColHeight < maxHeight ) // found a new place for this applet
                {
                    QGraphicsLayoutItem *it = columns[ maxColumn ]->itemAt( columns [ maxColumn ]->count() - 1 );
                    columns[ i ]->addItem( it );
                    columns[ i ]->removeItem( it );
                    found = true;
                    break;
                }
            }
        }
    }

    QList< VerticalLayout* > columns;
    qreal columnWidth;
    QRectF geom;
};

ContextLayout::ContextLayout(QGraphicsLayoutItem *parent)
    : QGraphicsLayout(parent)
    , d(new Private)
{
    DEBUG_BLOCK
    relayout();
}

ContextLayout::~ContextLayout()
{
    warning() << "Temporary fix for crash here, uncomment me later.";
    //delete d;
}

int
ContextLayout::count() const
{
    int total = 0;
    for( int i = 0; i < d->columns.count(); i++ )
        total += d->columns[ i ]->count();

    return total;
}

void
ContextLayout::addItem(QGraphicsLayoutItem* item)
{
    DEBUG_BLOCK
    if( d->columns.size() == 0 )
        d->columns << new VerticalLayout( this );

    int smallestColumn = 0, min = (int)d->columns[ 0 ]->effectiveSizeHint( Qt::PreferredSize ).height();
    for( int i = 0; i < d->columns.size(); i++ ) // find shortest column to put
    {                                           // the applet in
        if( d->columns[ i ]->effectiveSizeHint( Qt::PreferredSize ).height() < min )
            smallestColumn = i;
    }
    d->columns[ smallestColumn ]->addItem( item );

    relayout();
}

void
ContextLayout::removeItem(QGraphicsLayoutItem* item)
{
    DEBUG_BLOCK
    if(!item) {
        return;
    }
    debug() << "got item to remove";
    // find item and remove it
    for( int i = 0; i < d->columns.size(); i++ )
    {
//         if( d->columns[ i ]->indexOf( item ) != 0 )
            d->columns[ i ]->removeItem( item );
    }
}

int
ContextLayout::indexOf(QGraphicsLayoutItem* item) const
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

QGraphicsLayoutItem*
ContextLayout::itemAt(int at) const
{
    int count = 0;
    for( int i = 0; i < d->columns.count(); i++ )
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

QSizeF
ContextLayout::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_UNUSED(which);
    Q_UNUSED(constraint);
    // TODO A proper algorithm here
    //
    // Idea:  Return a size hint based on the golden ratio to
    //        make it aesthetically good
    //        eg. Longer side is 1.61x the length of the shorter side
    //

    // testing
    return QSizeF(500,500);
}

QGraphicsLayoutItem*
ContextLayout::takeAt(int at)
{
    int count = 0;
    for( int i = 0; i < d->columns.size(); i++ )
    {
        for( int k = 0; k < d->columns[ i ]->count(); k++ )
        {
            if( count == at )
            {
                QGraphicsLayoutItem *item = d->columns[ i ]->itemAt( k );
                d->columns[ i ]->removeAt( k );
                return item;
            }
            count++;
        }
    }
    return 0;
}

void
ContextLayout::removeAt( int at )
{
    int count = 0;
    for( int i = 0; i < d->columns.size(); i++ )
    {
        for( int k = 0; k < d->columns[ i ]->count(); k++ )
        {
            if( count == at )
            {
                d->columns[ i ]->removeAt( k );
                return;
            }
            count++;
        }
    }
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

void
ContextLayout::relayout()
{
  //  DEBUG_BLOCK
    QRectF rect = d->geom;
    const int numColumns = qMax( (int)(rect.width() / d->columnWidth), 1 );    //use at least one column

//    debug() << "ContextLayout::relayout laying out into:" << rect << "with" << numColumns << " columns";
    if( numColumns > d->columns.size() ) // need to make more columns
    {
        for( int i = d->columns.size(); i < numColumns; i++ ) {
//            debug() << "ContextLayout::relayout making a new column";
            VerticalLayout *newColumn = new VerticalLayout( this );
            d->columns << newColumn;
        }

    }
    else if( numColumns < d->columns.size() ) // view was shrunk
    {
        VerticalLayout* column = d->columns[ d->columns.size() - 1 ];
        d->columns.removeAt( d->columns.size() - 1 );
        for( int i = 0; i < column->count() ; i++ )
        {
            QGraphicsLayoutItem *applet = column->itemAt( i );
            column->removeAt( i );
            int smallestColumn = 0, min = (int)d->columns[ 0 ]->effectiveSizeHint( Qt::PreferredSize ).height();
            for( int i = 1; i < d->columns.size(); i++ ) // find shortest column to put
            {                                           // the applet in
                if( d->columns[ i ]->effectiveSizeHint( Qt::PreferredSize ).height() < min )
                    smallestColumn = i;
            }
            d->columns[ smallestColumn ]->addItem( applet );
//            debug() << "ContextLayout::relayout removing a column, adding an applet to column" << smallestColumn;
        }
    }


    qreal columnWidth = ( rect.width() - 0 ) / d->columns.size();

    for( int i = 0; i < d->columns.size(); i++ ) // lay out columns
    {
        QPointF pos( ( ( i + 1 ) * 0 ) + ( i * columnWidth ), 0 );
        qreal height1 = d->columns[ i ]->effectiveSizeHint( Qt::PreferredSize ).height();
        qreal height2 = rect.height();
        qreal maxHeight = qMax( height1, height2 );
        QSizeF size( columnWidth, maxHeight );
//        debug() << "setting column" << i << " geometry to pos" << pos << " " << size;
        d->columns[ i ]->setGeometry( QRectF( pos, size ) );
    }
    d->balanceColumns();
}


void ContextLayout::setGeometry( const QRectF& geom )
{
 //   DEBUG_BLOCK
 //   debug() << "Setting ContextLayout geometry to" << geom; 
    d->geom = geom;
    relayout();
}

qreal
ContextLayout::columnWidth() const
{
    return d->columnWidth;
}

void
ContextLayout::setColumnWidth( const qreal width )
{
    d->columnWidth = width;
}

