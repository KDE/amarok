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

#include "PopupDropperView.h"

#include "debug.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
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
    setAcceptDrops( true );
    setBackgroundRole( QPalette::Base );
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, 0) );
    setPalette( p );
    setFrameShape( QFrame::NoFrame );
}

PopupDropperView::~PopupDropperView()
{
    DEBUG_BLOCK
}

//SLOT
void PopupDropperView::setTransInValue( int value )
{
    //DEBUG_BLOCK
    //debug() << "value: " << value << endl;
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, value*12 ) );
    setPalette( p );
}

//SLOT
void PopupDropperView::setTransOutValue( int value )
{
    //DEBUG_BLOCK
    //debug() << "value: " << value << endl;
    QPalette p = palette();
    p.setColor( QPalette::Base, QColor(0, 0, 0, 120 - value*12) );
    setPalette( p );
}

void PopupDropperView::mouseMoveEvent( QMouseEvent *e )
{
    DEBUG_BLOCK
    if( !( e->buttons() & Qt::LeftButton) )
        emit destroyMe();
    QGraphicsView::mouseMoveEvent( e );
}

void PopupDropperView::mouseReleaseEvent( QMouseEvent *e )
{
    DEBUG_BLOCK
    QGraphicsView::mouseReleaseEvent( e );
}

#include "PopupDropperView.moc"

