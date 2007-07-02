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
#include "graphicsitemscaler.h"

#include <math.h>
#include "debug.h"

#include <QPen>

using namespace Context;

GraphicsItemScaler::GraphicsItemScaler( ContextBox *item )
    : QObject()
    , m_currWidth( item->boundingRect().width() )
    , m_currHeight( item->boundingRect().height() )
    , m_originalWidth( (int) item->boundingRect().width() )
    , m_originalHeight( (int) item->boundingRect().height() )
    , m_tgtWidth( 0 )
    , m_tgtHeight( 0 )
    , m_negativeWidth( false )
    , m_negativeHeight( false )
    , m_fps( 0 )
    , m_duration( 5000 )
{
    DEBUG_BLOCK
    m_contentItem = item;

    m_timeLine = new QTimeLine( m_duration, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( scaleSlot( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), this, SLOT( scaleFinished() ) );
}

GraphicsItemScaler::~GraphicsItemScaler()
{
    m_timeLine->stop();
    scaleSlot( m_animationSteps );
}

void GraphicsItemScaler::setTargetSize(int width, int height)
{
    m_tgtWidth = width;
    m_tgtHeight = height;
}

void GraphicsItemScaler::setDuration(int ms)
{
    m_duration = ms;
}

void GraphicsItemScaler::setFPS(int fps)
{
    m_fps = fps;
}

void GraphicsItemScaler::scaleSlot(int step)
{
    debug() << "step # " << step << " of " << m_animationSteps << endl;
    if( step == 0 || ( m_currWidth == m_tgtWidth && m_currHeight == m_tgtHeight ) )
        return;
    qreal percent = ( qreal ) step / m_animationSteps;

    qreal desiredWidth, desiredHeight;
    if( m_negativeWidth )
        desiredWidth = m_originalWidth + ( percent * ( m_tgtWidth - m_originalWidth ) );
    else
        desiredWidth = percent * m_tgtWidth;
    
    if( m_negativeHeight )
        desiredHeight = m_originalHeight + ( percent * ( m_tgtHeight - m_originalHeight ) );
    else
        desiredHeight = percent * m_tgtHeight;

    qreal scalefactorWidth = desiredWidth / m_currWidth;
    qreal scalefactorHeight = desiredHeight / m_currHeight;
    
    m_contentItem->scale( scalefactorWidth, scalefactorHeight );
    m_currWidth = desiredWidth;
    m_currHeight = desiredHeight;
    m_contentItem->update();
}

void GraphicsItemScaler::scaleFinished()
{
    DEBUG_BLOCK
    scaleSlot( m_animationSteps );
    emit( animationComplete() );
}

void GraphicsItemScaler::startScaling()
{
    DEBUG_BLOCK
    if( m_timeLine->state() != QTimeLine::NotRunning )
        m_timeLine->stop();

    int fps = ( m_fps ? m_fps : 25 );

    //total number of animation steps;
    m_animationSteps = (int) round( ( fps * ( ( qreal ) m_duration / 1000.0 ) ) );

    if( m_animationSteps == 0 )
        m_animationSteps = 1;

    m_originalWidth = (int) m_currWidth;
    m_originalHeight = (int) m_currHeight;

    if( m_tgtWidth - m_currWidth < 0 )
        m_negativeWidth = true;
    if( m_tgtHeight - m_currHeight < 0 )
        m_negativeHeight = true;

    debug() << "Start scaling, animationSteps = " << m_animationSteps << " over " << m_duration << " mseconds" << endl;

    m_timeLine->setDuration( m_duration );
    m_timeLine->setFrameRange( 0, m_animationSteps );
    m_timeLine->start();
}

#include "graphicsitemscaler.moc"
