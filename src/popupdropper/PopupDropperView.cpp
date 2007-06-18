/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "debug.h"
#include "PopupDropperView.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QRectF>

using namespace PopupDropperNS;

PopupDropperView::PopupDropperView( QGraphicsScene* scene, QWidget* parent )
                                    : QGraphicsView( scene, parent  )
{
    DEBUG_BLOCK
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    resize( parent->size() + QSize(2, 2) );
    setBackgroundRole( QPalette::Base );
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, 0) );
    setPalette( p );
    setAutoFillBackground( true );
}

PopupDropperView::~PopupDropperView()
{
}

//SLOT
void PopupDropperView::setTransInValue( int value )
{
    DEBUG_BLOCK
    debug() << "value: " << value << endl;
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, value*4) );
    setPalette( p );
}

//SLOT
void PopupDropperView::setTransOutValue( int value )
{
    DEBUG_BLOCK
    debug() << "value: " << value << endl;
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, 120 - value*4) );
    setPalette( p );
    if( 120 - value*4 == 0 )
    {
        emit destroying();
        delete this;
    }
}



#include "PopupDropperView.moc"

