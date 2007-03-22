/*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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

#include "cloudbox.h"

#include "debug.h"

#include <QFont>
#include <QGraphicsTextItem>

using namespace Context;


CloudTextItem::CloudTextItem(QString text, QGraphicsItem * parent, QGraphicsScene * scene)
: QGraphicsTextItem ( text, parent, scene )
{

    setAcceptsHoverEvents( true );

    m_timeLine = new QTimeLine( 1000, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( colorFadeSlot( int ) ) );

}

void CloudTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    
    m_timeLine->stop();
    
    setDefaultTextColor( QColor( 255, 255, 255 ) );


}

void CloudTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{

    //setDefaultTextColor( QColor( 0, 0, 0 ) );



     // Construct a 1-second timeline with a frame range of 0 - 100
     m_timeLine->setFrameRange(0, 30);
     m_timeLine->start();

}

void CloudTextItem::colorFadeSlot( int step ) {

    int colorValue = 255 - step * 8.5;
    if ( step == 100 ) colorValue = 0;

     setDefaultTextColor( QColor( colorValue, colorValue, colorValue ) );
     update();
}






Context::CloudBox::CloudBox( QGraphicsItem *parent, QGraphicsScene *scene )
: ContextBox( parent, scene )
{
   m_maxFontSize = 20;
   m_minFontSize = 4;

   m_runningX = 0.0;
   m_runningY = 0.0;

   m_currentLineMaxHeight = 0.0;

    for ( int i = 0; i < 20; i++) {

        int random = ( rand() % (m_maxFontSize - m_minFontSize) ) + m_minFontSize + 1;

        debug() << "randum font size: " << random << endl;

        addText( "Amarok", random);

    }


}


void CloudBox::addText(QString text, int weight)
{
    CloudTextItem * item = new CloudTextItem ( text, this, scene() );

    QFont font = item->font();

    font.setPointSize( weight );
    item->setFont( font );
    
    QRectF itemRect = item->boundingRect();
    QRectF parentRect = boundingRect();

    if (itemRect.height() > m_currentLineMaxHeight)
        m_currentLineMaxHeight = itemRect.height();

    if ( ( itemRect.width() + m_runningX ) > parentRect.width() ) {
        m_runningY += m_currentLineMaxHeight;
        m_runningX = 0;
        m_currentLineMaxHeight = 0;
    }

    item->setPos( QPointF( m_runningX, m_runningY + m_maxFontSize - weight ) );
    m_runningX += itemRect.width(); 


}

#include "cloudbox.moc"





