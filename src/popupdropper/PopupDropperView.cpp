/* 
   Copyright (C) 2008 Jeff Mitchell <mitchell@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

#include <QtDebug>

#include "PopupDropper.h"
#include "PopupDropperItem.h"
#include "PopupDropperView.h"

class PopupDropperViewPrivate
{
public:
    PopupDropperViewPrivate( PopupDropperView* parent, PopupDropper* pd )
        : pd( pd )
        , lastItem( 0 )
        , quitOnDragLeave( false )
        , q( parent )
        {}

    PopupDropper *pd;
    PopupDropperItem *lastItem;
    bool quitOnDragLeave;
    
private:
    PopupDropperView* q;
};

////////////////////////////////////////////////////////////////////////////

PopupDropperView::PopupDropperView( PopupDropper *pd, QGraphicsScene *scene, QWidget *parent )
    : QGraphicsView( scene, parent ) 
    , d( new PopupDropperViewPrivate( this, pd ) )
{
    setInteractive( true );
    setAcceptDrops( true );
}

PopupDropperView::~PopupDropperView()
{
    delete d;
}

void PopupDropperView::dragMoveEvent( QDragMoveEvent *event )
{
    //qDebug() << "PopupDropperView::dragMoveEvent";
    QGraphicsItem* item = itemAt( event->pos() );

    #define pditem(x) dynamic_cast<PopupDropperItem*>(x)

    if( !pditem(item) )
    {
        if( d->lastItem )
            d->lastItem->stopHoverTimer();
        d->lastItem = 0;
    }
    else if( d->lastItem != item )
    {
        if( d->lastItem )
            d->lastItem->stopHoverTimer();
        pditem(item)->startHoverTimer();
        d->lastItem = pditem(item) ;
    }
    
    #undef pditem
    
    event->accept();
}

void PopupDropperView::dragLeaveEvent( QDragLeaveEvent *event )
{
    qDebug() << "PopupDropperView::dragLeaveEvent";
    event->accept();
    d->pd->hide( PopupDropper::DragLeave );
}

void PopupDropperView::dropEvent( QDropEvent *event )
{
    qDebug() << "PopupDropperView::dropEvent";
    QGraphicsItem* item = itemAt( event->pos() );

    if( PopupDropperItem *pdi = dynamic_cast<PopupDropperItem*>(item) )
        pdi->dropped( event );
}

bool PopupDropperView::quitOnDragLeave() const
{
    return d->quitOnDragLeave;
}

void PopupDropperView::setQuitOnDragLeave( bool quit )
{
    d->quitOnDragLeave = quit;
}

#include "PopupDropperView.moc"

