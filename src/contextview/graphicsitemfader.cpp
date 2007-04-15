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

#include "graphicsitemfader.h"

#include "debug.h"

#include <QPen>

using namespace Context;


GraphicsItemFader::GraphicsItemFader( QGraphicsItem * item, QGraphicsItem * parent )
    : QGraphicsItem ( parent )
    , QObject()
    , m_startAlpha( 0 )
    , m_targetAlpha( 255 )
    , m_fps ( 30 )
    , m_duration( 5000 )
{
    m_contentItem = item;
    m_contentItem->setParentItem( this );

    m_shadeRectItem = new QGraphicsRectItem( this );
    m_shadeRectItem->setRect( 0, 0, m_contentItem->boundingRect().width() + 2, m_contentItem->boundingRect().height() + 2 );
    m_shadeRectItem->setPos( -1, -1 ); // needs a slight offset to cover frames on m_contentItem
    m_shadeRectItem->setPen( Qt::NoPen );

    m_fadeColor = QColor ( 255, 255, 255, 0 );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );

    m_timeLine = new QTimeLine( 0, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( fadeSlot( int ) ) );
}

void GraphicsItemFader::setStartAlpha(int alpha)
{
    m_startAlpha = alpha;
    m_fadeColor.setAlpha( m_startAlpha );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );
}

void GraphicsItemFader::setTargetAlpha(int alpha)
{
    m_targetAlpha = alpha;
}

void GraphicsItemFader::setDuration(int ms)
{
    m_duration = ms;
}

void GraphicsItemFader::setFPS(int fps)
{
    m_fps = fps;
}


QRectF Context::GraphicsItemFader::boundingRect() const
{
    m_contentItem->boundingRect();
}


void GraphicsItemFader::setFadeColor(const QColor & color)
{
    m_fadeColor = color;
    m_fadeColor.setAlpha( m_startAlpha );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );
}

void GraphicsItemFader::fadeSlot(int step)
{
    m_fadeColor.setAlpha( m_startAlpha + ( step * m_alphaStep ) );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );
}

void GraphicsItemFader::startFading()
{
    //total number of animation steps;
    m_animationSteps = m_fps * ( ( float ) m_duration / 1000.0 );

    //how much should alpha change each step
    m_alphaStep = ( ( float ) ( m_targetAlpha - m_startAlpha ) ) / ( float ) m_animationSteps;

    m_timeLine->setDuration( m_duration );
    m_timeLine->setFrameRange( 0, m_animationSteps );
    m_timeLine->start();
}

void GraphicsItemFader::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    Q_UNUSED( painter );
    Q_UNUSED( option );
    Q_UNUSED( widget );
}

#include "graphicsitemfader.moc"
