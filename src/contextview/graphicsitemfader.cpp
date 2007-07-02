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

#include "contextbox.h"
#include "graphicsitemfader.h"

#include "debug.h"

#include <QPen>

using namespace Context;


GraphicsItemFader::GraphicsItemFader( ContextBox *item )
    : QObject()
    , m_startAlpha( 0 )
    , m_targetAlpha( 255 )
    , m_fps ( 30 )
    , m_duration( 5000 )
{
    m_contentItem = item;

    //m_fadeColor = QColor ( 255, 255, 255, 0 );
    m_fadeColor = m_contentItem->brush().color();

    m_timeLine = new QTimeLine( m_duration, this );
    m_timeLine->setCurveShape( QTimeLine::LinearCurve );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( fadeSlot( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), this, SLOT( fadeFinished() ) );
}

GraphicsItemFader::~GraphicsItemFader()
{
    m_timeLine->stop();
    fadeSlot( m_animationSteps );
}

void GraphicsItemFader::setStartAlpha(int alpha)
{
    m_startAlpha = alpha;
    m_fadeColor.setAlpha( m_startAlpha );
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

void GraphicsItemFader::fadeSlot(int step)
{
    debug() << "step # " << step << " of " << m_animationSteps << endl;
    qreal newAlpha = m_startAlpha + ( step * m_alphaStep );
    m_fadeColor.setAlpha( (int)newAlpha );

    QPen fadePen = m_contentItem->pen();
    QColor penColor = fadePen.color();
    penColor.setAlpha( (int)newAlpha );
    fadePen.setColor( penColor );
    m_contentItem->setPen( fadePen );

    m_contentItem->titleItem()->setDefaultTextColor( QColor( 255, 255, 255, (int)newAlpha ) );

    QLinearGradient titleBarRectGradient( QPointF( 0, 0 ), QPointF( 0, m_contentItem->titleBarRect()->boundingRect().height() ) );
    titleBarRectGradient.setColorAt( 0, QColor( 200, 200, 255, (int)newAlpha ) );
    titleBarRectGradient.setColorAt( 1, QColor( 50, 50, 255, (int)newAlpha ) );
    m_contentItem->titleBarRect()->setBrush( QBrush( titleBarRectGradient ) );

    QLinearGradient contentRectGradient( QPointF( 0, 0 ), QPointF( 0, 10) );
    contentRectGradient.setColorAt( 0, QColor( 150, 150, 150, (int)newAlpha ) );
    contentRectGradient.setColorAt( 1, QColor( 255, 255, 255, (int)newAlpha ) );
    m_contentItem->contentRect()->setBrush( QBrush( contentRectGradient ) );

    m_contentItem->update();
}

void GraphicsItemFader::fadeFinished()
{
    DEBUG_BLOCK
    emit( animationComplete() );
}

void GraphicsItemFader::startFading()
{
    if( m_timeLine->state() != QTimeLine::NotRunning )
        m_timeLine->stop();
    //total number of animation steps;
    m_animationSteps = (int) ( m_fps * ( ( qreal ) m_duration / 1000.0 ) );

    //how much should alpha change each step
    m_alphaStep = ( ( qreal ) ( m_targetAlpha - m_startAlpha ) ) / ( qreal ) m_animationSteps;

    debug() << "Start fading, animationSteps = " << m_animationSteps << " over " << m_duration << " mseconds, alphaStep = " <<  m_alphaStep << endl;

    m_timeLine->setDuration( m_duration );
    m_timeLine->setFrameRange( 0, m_animationSteps );
    m_timeLine->start();
}

#include "graphicsitemfader.moc"
