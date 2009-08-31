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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_VERTICAL_LAYOUT_H
#define AMAROK_VERTICAL_LAYOUT_H

#include "amarok_export.h"

#include <QGraphicsLayout>
#include <QtCore/QList>

namespace Context
{

/**
* This layout is kind of like a vertical BoxLayout, but is different in a few
* key ways: it does not take a fixed height, but rather calculates it based on the
* sizeHints of the items, and it uses getHeightForWidth in order to calculate this
* height.
*/

class AMAROK_EXPORT VerticalLayout : public QGraphicsLayout
{
public:
    
    explicit VerticalLayout( QGraphicsLayoutItem *parent = 0 );
    ~VerticalLayout();

    // reimplemented from Layout
    virtual void addItem( QGraphicsLayoutItem *l );
    virtual void removeItem( QGraphicsLayoutItem *l );
    virtual int indexOf( QGraphicsLayoutItem *l ) const;
    virtual QGraphicsLayoutItem *itemAt( int i ) const;
    virtual QGraphicsLayoutItem *takeAt( int i );
    virtual void removeAt( int i );
    virtual void setGeometry( const QRectF &geometry );
    virtual QRectF geometry() const;
    virtual int count() const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const;

    void relayout();
        
private:
    class Private;
    Private *const d;
};

}

#endif
