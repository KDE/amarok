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

#include "introanimation.h"

#include "debug.h"

#include <kstandarddirs.h>
#include <QFont>
#include <QTimer>

using namespace Context;

IntroAnimation::IntroAnimation( QGraphicsItem * parent )
    : QObject()
    , QGraphicsItem( parent )
{

    QGraphicsPixmapItem *logoItem = new QGraphicsPixmapItem ( QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );

    logoItem->setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    m_logoFader = new GraphicsItemFader( logoItem, this );

    m_logoFader->setDuration( 5000 );
    m_logoFader->setFPS( 30 );
    m_logoFader->setStartAlpha( 0 );
    m_logoFader->setTargetAlpha( 200 );
    m_logoFader->setZValue ( 1 );
    connect( m_logoFader, SIGNAL( animationComplete() ), this, SLOT( imageFadeoutComplete() ) );


    m_width = m_logoFader->boundingRect().width();
    m_height = m_logoFader->boundingRect().height();;

    m_textFader = new TextFader("Amarok 2 will rock your world!", this);
    QFont font = m_textFader->font();
    font.setPointSize( 20 );
    m_textFader->setFont( font );

    m_textFader->setDuration( 3000 );
    m_textFader->setFPS( 30 );
    m_textFader->setStartAlpha( 0 );
    m_textFader->setTargetAlpha( 255 );
    m_textFader->setZValue( 2 );
    connect( m_textFader, SIGNAL( animationComplete() ), this, SLOT( textFadeinComplete() ) );

    if ( m_textFader->boundingRect().width() > m_width )
        m_width = m_textFader->boundingRect().width();

    if ( m_textFader->boundingRect().height() > m_height )
        m_height = m_textFader->boundingRect().height();

    //center the items
    //debug() << "Width of m_logoFader: " << m_logoFader->boundingRect().width() <<  endl;
    //debug() << "Width of m_textFader: " << m_textFader->boundingRect().width() <<  endl;

    m_textFader->setPos( ( m_width - m_textFader->boundingRect().width() ) / 2, ( m_height - m_textFader->boundingRect().height() ) / 2 );
    m_logoFader->setPos( ( m_width - m_logoFader->boundingRect().width() ) / 2, ( m_height - m_logoFader->boundingRect().height() ) / 2 );
}


IntroAnimation::~IntroAnimation()
{
}


void IntroAnimation::startAnimation()
{
    m_logoFader->startFading();
}

void IntroAnimation::imageFadeoutComplete()
{
    m_textFader->startFading();
}

void IntroAnimation::textFadeinComplete()
{
    QTimer::singleShot( 3000, this, SLOT( trailTimeComplete() ) );
}

void IntroAnimation::trailTimeComplete()
{
    emit( animationComplete() );
}

QRectF IntroAnimation::QGraphicsItem::boundingRect() const
{

}

void IntroAnimation::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    Q_UNUSED( painter );
    Q_UNUSED( option );
    Q_UNUSED( widget );
}

void Context::IntroAnimation::setFadeColor(const QColor & color)
{
    m_logoFader->setFadeColor( color );
}

QRectF Context::IntroAnimation::boundingRect() const
{
    return QRectF( 0,0,m_width, m_height );
}

#include "introanimation.moc"
