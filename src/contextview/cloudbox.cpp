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

        debug() << "randum font size: " << random ;

        addText( "Amarok", random);

    }


}


void CloudBox::addText(QString text, int weight)
{
    QGraphicsTextItem * item = new QGraphicsTextItem ( text, this, scene() );
    QFont font("Helvetica", weight, QFont::Bold);
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



