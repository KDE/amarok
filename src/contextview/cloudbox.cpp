/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "cloudbox.h"

#include "debug.h"

#include <QFont>
#include <QGraphicsTextItem>

using namespace Context;


CloudTextItem::CloudTextItem( QString text, QGraphicsItem *parent, QGraphicsScene *scene )
    : QGraphicsTextItem( text, parent, scene )
{
    setAcceptsHoverEvents( true );
    m_timeLine = new QTimeLine( 1000, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( colorFadeSlot( int ) ) );
}

void CloudTextItem::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
    m_timeLine->stop();
    m_timeLine->setCurrentTime ( 0 );

    setDefaultTextColor( QColor( 0, 127, 255 ) );
    update();
}

void CloudTextItem::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
    //setDefaultTextColor( QColor( 0, 0, 0 ) );

    // Construct a 1-second timeline with a frame range of 0 - 30
    m_timeLine->setFrameRange(0, 30);
    m_timeLine->start();
}

void CloudTextItem::colorFadeSlot( int step )
{
    int colorValue = 255 - step * 8.5;
    if( step == 100 )
        colorValue = 0;

    setDefaultTextColor( QColor( 0, colorValue / 2, colorValue ) );
    update();
}

void CloudTextItem::mousePressEvent( QGraphicsSceneMouseEvent * )
{
    emit( clicked( toPlainText() ) );
}



Context::CloudBox::CloudBox( QGraphicsItem *parent, QGraphicsScene *scene )
    : ContextBox( parent, scene )
{
   m_maxFontSize = 24;
   m_minFontSize = 4;

   m_runningX = 0.0;
   m_runningY = 0.0;

   m_currentLineMaxHeight = 0.0;
}

void CloudBox::addText( QString text, int weight, QObject *receiver, const char *slot )
{
    // create the new text item
    CloudTextItem * item = new CloudTextItem ( text, m_contentRect, 0 );
    //item->setParentItem( this );
    if( receiver && slot )
        item->connect ( item, SIGNAL( clicked( QString ) ), receiver, slot );

    QFont font = item->font();
    font.setPointSize( weight );
    item->setFont( font );

    QRectF itemRect = item->boundingRect();
    QRectF parentRect = boundingRect();

    // check if item will fit inside cloud at all... if not, just skip it
    // (Does anyone have a better idea how to handle this? )
    if( itemRect.width() > parentRect.width() )
    {
        delete item;
        return;
    }

    // Check if item will fit on the current line, if not, print current line
    if( ( itemRect.width() + m_runningX ) > parentRect.width() )
    {
        adjustCurrentLinePos();
        m_runningX = 0;
    }

    m_runningX += itemRect.width();
    m_currentLineItems.append( item );
}

void CloudBox::adjustCurrentLinePos()
{
    if ( m_currentLineItems.isEmpty() ) return;

    int totalWidth = 0;
    int maxHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    int currentX = 0;

    CloudTextItem * currentItem;
    QRectF currentItemRect;

    //First we run through the list to get the max height of an item
    // and the total width of all items.
    foreach( currentItem, m_currentLineItems )
    {
        currentItemRect = currentItem->boundingRect();
        totalWidth += currentItemRect.width();
        if ( currentItemRect.height() > maxHeight ) maxHeight = currentItemRect.height();
    }

    //do we have enough vertical space for this line? If not, create some!
    if ( ( m_runningY + maxHeight ) > m_contentRect->boundingRect().height() )
    {
        int missingHeight = ( m_runningY + maxHeight ) -  m_contentRect->boundingRect().height();
       // m_contentRect->setRect(0, 0, boundingRect().width(), boundingRect().height() + missingHeight );
       setContentRectSize( QSize( boundingRect().width(), boundingRect().height() + missingHeight ) );
    }

    //calc the X offset that makes the line centered horizontally
    offsetX = ( boundingRect().width() - totalWidth ) / 2;

    //then remove all items from the list, setting the correct position of each in the process
    while (!m_currentLineItems.isEmpty())
    {
        currentItem = m_currentLineItems.takeFirst();
        currentItemRect = currentItem->boundingRect();
        offsetY = ( (maxHeight - currentItemRect.height()) / 2 );
        currentItem->setPos( QPointF( currentX + offsetX, m_runningY + offsetY ) );
        currentX += currentItemRect.width();
    }

    m_runningY += maxHeight;
}

void Context::CloudBox::done()
{
    adjustCurrentLinePos();
}


#include "cloudbox.moc"
