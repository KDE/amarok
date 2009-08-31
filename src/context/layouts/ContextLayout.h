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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CONTEXT_LAYOUT_H
#define CONTEXT_LAYOUT_H

#include "amarok_export.h"

#include <QGraphicsLayout>

namespace Context
{

/**
 * A layout which lays items out left-to-right , top-to-bottom.
 *
 * This is similar to the layout of items in a QListView.
 *
 * Additionally, this class only lays items out width-wise and
 * uses the items' heightForWidth for the heights
 */
class AMAROK_EXPORT ContextLayout : public QGraphicsLayout
{
public:
    /** Construct a new flow layout with the specified parent. */
    explicit ContextLayout( QGraphicsLayoutItem* parent );
    virtual ~ContextLayout();

    // reimplemented
    virtual int count() const;
    virtual void addItem( QGraphicsLayoutItem* item );
    virtual void removeItem( QGraphicsLayoutItem* item );
    virtual int indexOf( QGraphicsLayoutItem* item ) const;
    virtual QGraphicsLayoutItem* itemAt( int i ) const;
    virtual QGraphicsLayoutItem* takeAt( int i );
    virtual void removeAt( int i );

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const;
    virtual void setColumnWidth( const qreal width );
    virtual qreal columnWidth() const;
    
    virtual void setGeometry( const QRectF& geom );

    void relayout();
        
private:
    class Private;
    Private *const d;
};

}

#endif // __FLOWLAYOUT__

