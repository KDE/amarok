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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

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
    m_contentItem->setZValue ( 1 );
    m_contentItem->setPos( 1, 1 );

    m_shadeRectItem = new QGraphicsRectItem( this );

    m_width = m_contentItem->boundingRect().width() + 2;
    m_height = m_contentItem->boundingRect().height() + 2;



    m_shadeRectItem->setRect( 0, 0, m_width, m_height );
    m_shadeRectItem->setPos( 0, 0 ); // needs a slight offset to cover frames on m_contentItem
    m_shadeRectItem->setPen( Qt::NoPen );
    m_shadeRectItem->setZValue ( 2 );

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
    return QRect( x(), y(), m_width, m_height );
}


void GraphicsItemFader::setFadeColor(const QColor & color)
{
    m_fadeColor = color;
    m_fadeColor.setAlpha( m_startAlpha );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );
}

void GraphicsItemFader::fadeSlot(int step)
{
    int newAlpha = m_startAlpha + ( step * m_alphaStep );
    m_fadeColor.setAlpha( newAlpha );
    m_shadeRectItem->setBrush( QBrush ( m_fadeColor ) );

    if ( newAlpha == m_targetAlpha )
        emit( animationComplete() );

    //debug() << "fading, new alpha = " << newAlpha <<  endl;
}

void GraphicsItemFader::startFading()
{


    //total number of animation steps;
    m_animationSteps = m_fps * ( ( float ) m_duration / 1000.0 );

    //how much should alpha change each step
    m_alphaStep = ( ( float ) ( m_targetAlpha - m_startAlpha ) ) / ( float ) m_animationSteps;

    //debug() << "Start fading, animationSteps = " << m_animationSteps << ", alphaStep = " <<  m_alphaStep << endl;

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
