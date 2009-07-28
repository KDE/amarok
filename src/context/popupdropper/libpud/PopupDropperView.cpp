/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "PopupDropper.h"
#include "PopupDropper_p.h"
#include "PopupDropperItem.h"
#include "PopupDropperView.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

#include <QtDebug>

class PopupDropperViewPrivate
{
public:
    PopupDropperViewPrivate( PopupDropperView* parent, PopupDropper* pd )
        : pd( pd )
        , lastItem( 0 )
        , entered( false )
        , q( parent )
        {}

    PopupDropper *pd;
    PopupDropperItem *lastItem;
    bool entered;
    
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

    #define svgitem(x) dynamic_cast<QGraphicsSvgItem*>(x)
    #define textitem(x) dynamic_cast<QGraphicsTextItem*>(x)
    #define borderitem(x) dynamic_cast<QGraphicsRectItem*>(x)

    if( !svgitem(item) && !textitem(item) && !borderitem(item) )
    {
        if( d->lastItem )
            d->lastItem->hoverLeft();
        d->lastItem = 0;
    }
    else if( svgitem(item) &&
            d->lastItem != dynamic_cast<PopupDropperItem*>( svgitem(item)->parentItem() ) )
    {
        //qDebug() << "svg item";
        if( d->lastItem )
            d->lastItem->hoverLeft();
        static_cast<PopupDropperItem*>( svgitem(item)->parentItem() )->hoverEntered();
        d->lastItem = static_cast<PopupDropperItem*>( svgitem(item)->parentItem() );
    }
    else if( textitem(item) && 
             d->lastItem != dynamic_cast<PopupDropperItem*>( textitem(item)->parentItem() ) )
    {
        //qDebug() << "text item";
        if( d->lastItem )
            d->lastItem->hoverLeft();
        static_cast<PopupDropperItem*>( textitem(item)->parentItem() )->hoverEntered();
        d->lastItem = static_cast<PopupDropperItem*>( textitem(item)->parentItem() );
    }
    else if( borderitem(item) && 
             d->lastItem != dynamic_cast<PopupDropperItem*>( borderitem(item)->parentItem() ) )
    {
        //qDebug() << "border item";
        if( d->lastItem )
            d->lastItem->hoverLeft();
        static_cast<PopupDropperItem*>( borderitem(item)->parentItem() )->hoverEntered();
        d->lastItem = static_cast<PopupDropperItem*>( borderitem(item)->parentItem() );
    }
    
    #undef borderitem
    #undef textitem
    #undef pditem

    event->accept();
}

void PopupDropperView::dragEnterEvent( QDragEnterEvent *event )
{
    //qDebug() << "PopupDropperView::dragEnterEvent";
    event->accept();
    d->entered = true;
    d->pd->d->dragEntered();
}

void PopupDropperView::dragLeaveEvent( QDragLeaveEvent *event )
{
    //qDebug() << "PopupDropperView::dragLeaveEvent";
    event->accept();
    if( d->lastItem )
    {
        d->lastItem->hoverLeft();
        d->lastItem = 0;
    }
    d->pd->d->dragLeft();
}

void PopupDropperView::dropEvent( QDropEvent *event )
{
    //qDebug() << "PopupDropperView::dropEvent";

    if( !d->pd->d->amIOnTop( this ) )
    {
        event->accept();
        return;
    }

    QGraphicsItem* item = itemAt( event->pos() );

    if( QGraphicsSvgItem *svgItem = dynamic_cast<QGraphicsSvgItem*>(item) )
    {
        //qDebug() << "It's a svg item";
        if( PopupDropperItem *pdi = dynamic_cast<PopupDropperItem*>( svgItem->parentItem() ) )
            pdi->dropped( event );
    }
    else if( QGraphicsTextItem *textItem = dynamic_cast<QGraphicsTextItem*>(item) )
    {
        //qDebug() << "It's a text item";
        if( PopupDropperItem *pdi = dynamic_cast<PopupDropperItem*>( textItem->parentItem() ) )
            pdi->dropped( event );
    }
    else if( QGraphicsRectItem *borderItem = dynamic_cast<QGraphicsRectItem*>(item) )
    {
        //qDebug() << "It's a border item";
        if( PopupDropperItem *pdi = dynamic_cast<PopupDropperItem*>( borderItem->parentItem() ) )
            pdi->dropped( event );
    }
    event->accept();
    //qDebug() << "Leaving dropEvent";
}

void PopupDropperView::resetView()
{
    d->lastItem = 0;
    d->entered = false;
    setAcceptDrops( true );
}

void PopupDropperView::deactivateHover()
{
    if( d->lastItem )
        d->lastItem->hoverLeft();
    d->lastItem = 0;
}

bool PopupDropperView::entered() const
{
    return d->entered;
}

void PopupDropperView::setEntered( bool entered )
{
    d->entered = entered;
}

#include "PopupDropperView.moc"

