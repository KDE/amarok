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
#include "PopupDropper.h"
#include "PopupDropperBaseItem.h"
#include "TheInstances.h"

#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPainter>
#include <QRectF>

using namespace PopupDropperNS;

PopupDropperBaseItem::PopupDropperBaseItem( int whichami, int total, QGraphicsItem* parent )
                                    : QObject( 0 )
                                    , QGraphicsItem( parent )
                                    , m_scalingPercent( 0.0 )
                                    , m_whichami( whichami )
                                    , m_totalEntries( total )
{
    DEBUG_BLOCK
}

PopupDropperBaseItem::~PopupDropperBaseItem()
{
    DEBUG_BLOCK
}

QRectF
PopupDropperBaseItem::boundingRect() const
{
    DEBUG_BLOCK
    QRectF sceneRect = The::PopupDropper()->sceneRect();
    qreal scenePct = m_whichami * 1.0 / m_totalEntries;
    debug() << "scenePct of this item: " << scenePct << endl;
    qreal height = sceneRect.height() / m_totalEntries;
    debug() << "height of this item: " << height << endl;
    qreal width = sceneRect.width() / m_totalEntries;
    debug() << "width of this item: " << width << endl;
    return QRectF( width * (m_whichami - 1), height * (m_whichami - 1), width, height );
}

void
PopupDropperBaseItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    QRectF sceneRect = The::PopupDropper()->sceneRect();
    qreal scenePct = m_whichami * 1.0 / m_totalEntries;
    qreal height = sceneRect.height() / m_totalEntries;
    qreal width = sceneRect.width() / m_totalEntries;
    painter->setFont( QFont("Times", 96, QFont::Bold) );
    painter->setPen( Qt::white );
    painter->drawText( width * (m_whichami - 1), height * (m_whichami - 1), width, height, Qt::AlignHCenter | Qt::AlignVCenter, QChar( m_whichami + 48 ) );
}

#include "PopupDropperBaseItem.moc"
