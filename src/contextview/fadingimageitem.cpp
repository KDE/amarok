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

#include "fadingimageitem.h"

#include "debug.h"

#include <QPen>

using namespace Context;


FadingImageItem::FadingImageItem( const QPixmap & pixmap, QGraphicsItem * parent ) 
: QGraphicsPixmapItem ( pixmap, parent )
, QObject()
{

    m_shadeRectItem = new QGraphicsRectItem( this );
    m_shadeRectItem->setRect( 0, 0, boundingRect().width(), boundingRect().height() );
    m_shadeRectItem->setPen( Qt::NoPen );
    m_shadeRectItem->setBrush( QBrush ( QColor ( 255, 255, 255, 0 )  ) );

    // 10 secs at 25.5 fps
    m_animationSteps = 255;

     m_timeLine = new QTimeLine( 10000, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( fadeSlot( int ) ) );




}

void Context::FadingImageItem::setFadeColor(const QColor & color)
{
    m_fadeColor = color;
    m_fadeColor.setAlpha( 0 );

}

void Context::FadingImageItem::setTargetAlpha(int alpha)
{
    m_targetAlpha = alpha;

}

void Context::FadingImageItem::fadeSlot(int step)
{

    m_fadeColor.setAlpha( m_targetAlpha - ( m_animationSteps -  step ) );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );

}

void Context::FadingImageItem::startFading()
{
     m_timeLine->setFrameRange(0, m_animationSteps);
     m_timeLine->start();
}


#include "fadingimageitem.moc"





