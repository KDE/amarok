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

#include "textfader.h"
#include <debug.h>
#include <QPainter>

using namespace Context;

TextFader::TextFader( const QString & text, QGraphicsItem * parent )
    : QGraphicsTextItem ( text, parent )
    , m_startAlpha( 0 )
    , m_targetAlpha( 255 )
    , m_currentAlpha( 0 )
    , m_fps ( 30 )
    , m_duration( 5000 )
{

    m_timeLine = new QTimeLine( 0, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( fadeSlot( int ) ) );
}


TextFader::~TextFader()
{
}

void TextFader::setStartAlpha(int alpha)
{
    m_startAlpha = alpha;
    m_currentAlpha = m_startAlpha;
}

void TextFader::setTargetAlpha(int alpha)
{
    m_targetAlpha = alpha;
}

void TextFader::setDuration(int ms)
{
    m_duration = ms;
}

void TextFader::setFPS(int fps)
{
    m_fps = fps;
}


void TextFader::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    
    painter->setPen( defaultTextColor () );
    painter->setFont( font() );

    float alpha = ( (float) m_currentAlpha ) / 255.0;

    //debug() << "TextFader::paint, current alpha = " << m_currentAlpha << ", opacity = " << alpha << endl;

    painter->setOpacity( alpha );
    painter->drawText( boundingRect(), Qt::AlignCenter, toPlainText() );

}

void TextFader::fadeSlot(int step)
{
    m_currentAlpha = (int) m_startAlpha + ( step * m_alphaStep );
    //debug() << "fading, new alpha = " << m_currentAlpha <<  endl;
    update();
    if ( m_currentAlpha == m_targetAlpha )
        emit( animationComplete() );
}

void TextFader::startFading()
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


#include "textfader.moc"

