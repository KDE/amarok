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

#include "PopupDropperBaseItem.h"

#include "debug.h"
#include "PopupDropper.h"
#include "TheInstances.h"

#include <QBrush>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>

using namespace PopupDropperNS;

PopupDropperBaseItem::PopupDropperBaseItem( int whichami, int total, QGraphicsItem* parent )
                                    : QObject( 0 )
                                    , QGraphicsItem( parent )
                                    , m_scaledPercent( 0.0 )
                                    , m_whichami( whichami )
                                    , m_totalEntries( total )
{
    DEBUG_BLOCK
    setAcceptDrops( true );
}

PopupDropperBaseItem::~PopupDropperBaseItem()
{
    DEBUG_BLOCK
}

QRectF
PopupDropperBaseItem::boundingRect() const
{
    QRectF sceneRect = The::PopupDropper()->sceneRect();
    qreal height = ( sceneRect.height() / m_totalEntries ) * 0.8;
    qreal width = ( sceneRect.width() / m_totalEntries ) * 0.8;
    return QRectF( 0, 0, width, height );
}

void
PopupDropperBaseItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    QRectF sceneRect = The::PopupDropper()->sceneRect();
    qreal height = ( sceneRect.height() / m_totalEntries ) * 0.8;
    qreal width = ( sceneRect.width() / m_totalEntries ) * 0.8;
    QPen pen;
    pen.setWidth( 10 );
    pen.setColor( Qt::GlobalColor( Qt::lightGray + m_whichami ));
    painter->setPen( pen );
    painter->fillRect( QRect( 0, 0, (int)width, (int)height ), QBrush( Qt::white ) );
    painter->drawRect( QRect( 0, 0, (int)width, (int)height ) );
}

void
PopupDropperBaseItem::dropEvent( QGraphicsSceneDragDropEvent *e )
{
    DEBUG_BLOCK
    QGraphicsItem::dropEvent( e );
}

#include "PopupDropperBaseItem.moc"
