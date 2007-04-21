/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                :(C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "contextbox.h"
#include "debug.h"

#include <QGraphicsItemAnimation>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QRectF>
#include <QTimeLine>

using namespace Context;

ContextBox::ContextBox( QGraphicsItem *parent, QGraphicsScene *scene )
    : QObject()
    , QGraphicsRectItem( parent, scene )
    , m_titleItem( 0 )
    , m_goingUp( false )
    , m_optimumHeight( 0 )
    , m_animationTimer( 0 )
{
    setHandlesChildEvents( true ); // events from the sub items are passed onto this object

    const QRectF boundingRect = QRectF( 0, 0, 400, 200 );
    setRect( boundingRect );

    setPen( QPen( Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin ) );

    m_titleBarRect = new QGraphicsRectItem( this, scene );

    m_titleItem = new QGraphicsTextItem( "", m_titleBarRect, scene );
    m_titleItem->setDefaultTextColor( QColor( 255, 255, 255 ) );
    // increase the font size for the title
    QFont font = m_titleItem->font();
    font.setPointSize( 14 );
    font.setBold( true );
    m_titleItem->setFont( font );

    m_titleBarRect->setRect( 0, 0, boundingRect.width(), m_titleItem->boundingRect().height() );
    m_titleBarRect->setPos( 0, 0 );
    m_titleBarRect->setPen( Qt::NoPen );

    QLinearGradient titleGradient(QPointF( 0, 0 ), QPointF( 0, m_titleBarRect->boundingRect().height() ) );
    titleGradient.setColorAt( 0, QColor( 200, 200, 255 ) );
    titleGradient.setColorAt( 1, QColor( 50, 50, 255 ) );

    m_titleBarRect->setBrush( QBrush( titleGradient ) );

    m_contentRect = new QGraphicsRectItem( this, scene );
    m_contentRect->setRect( 0, 0, boundingRect.width(), boundingRect.height() - m_titleBarRect->boundingRect().height() );
    m_contentRect->setPos( 0, m_titleBarRect->boundingRect().height() );
    m_contentRect->setPen( Qt::NoPen );

    //make a nice shadow
    QLinearGradient shadowGradient( QPointF( 0, 0 ), QPointF( 0, 10) );
    shadowGradient.setColorAt( 0, QColor( 150, 150, 150 ) );
    shadowGradient.setColorAt( 1, QColor( 255, 255, 255 ) );
    m_contentRect->setBrush( QBrush( shadowGradient ) );

    m_optimumHeight = m_contentRect->rect().height();
}

void ContextBox::setTitle( const QString &title )
{
    m_titleItem->setPlainText( title );

    qreal titleWidth = m_titleItem->boundingRect().width();
    // If the title is too big for the box, make the box bigger
    if( titleWidth > m_titleBarRect->boundingRect().width() )
    {
        // this function takes care of setting everything to the correct size!
        setContentRectSize( QSize( (int)titleWidth, (int)m_contentRect->boundingRect().height() ) );
    }

    // Center the title
    int xOffset = (int)( m_titleBarRect->boundingRect().width() - titleWidth ) / 2;
    m_titleItem->setPos( xOffset, 0 );
}

void ContextBox::setBoundingRectSize( const QSize &sz )
{
    QRectF newRect = QRectF( 0, 0, sz.width(), sz.height() );
    setRect( newRect );
}

void ContextBox::setContentRectSize( const QSize &sz )
{
    m_contentRect->setRect( QRectF( 0, 0, sz.width(), sz.height() ) );
    //set correct size of this as well
    setRect( QRectF( 0, 0, sz.width(), sz.height() +  m_titleBarRect->boundingRect().height()) );
    m_titleBarRect->setRect( 0, 0, sz.width(), m_titleBarRect->boundingRect().height() );

//     m_optimumHeight = boundingRect().height();
}

void ContextBox::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->buttons() & Qt::LeftButton ) // only handle left button clicks for now
    {
        QPointF pressPoint = event->buttonDownPos( Qt::LeftButton );
        if( m_titleBarRect->contains( pressPoint ) )
            toggleVisibility();
    }
}

void ContextBox::toggleVisibility()
{
    static const int range = 100;

    if( !m_animationTimer )
    {
        m_animationTimer = new QTimeLine( 1000 );
        m_animationTimer->setUpdateInterval( 30 ); // ~33 fps
        m_animationTimer->setFrameRange( 0, range );
        m_animationTimer->setLoopCount( 0 ); // loop forever until we explicitly stop it
    }

    if( m_animationTimer->state() == QTimeLine::Running )
    {
        m_goingUp = !m_goingUp; // change direction if the is already an animation
        debug() << "chaning direction!" << endl;
        return;
    }

    m_animationTimer->setStartFrame( 0 );

    m_animationIncrement = m_optimumHeight / range;
    debug() << "m_goingUp: " << m_goingUp << endl;
    debug() << "m_optimumHeight: " << m_optimumHeight << endl;
    debug() << "m_animationIncrement: " << m_animationIncrement << endl;

    connect( m_animationTimer, SIGNAL( frameChanged(int) ), SLOT( visibilityTimerSlot() ) );
    m_animationTimer->start();
}

void ContextBox::visibilityTimerSlot()
{
    const qreal desiredHeight = m_goingUp ? m_optimumHeight : 0;

    qreal newHeight = m_goingUp ?
            m_contentRect->rect().height() + m_animationIncrement: //get bigger if hidden
            m_contentRect->rect().height() - m_animationIncrement; //get smaller if visible

    debug() << "        : " << newHeight << " -> " << desiredHeight << endl;

    if( ( !m_goingUp && newHeight <= desiredHeight ) ||
        (  m_goingUp && newHeight >= desiredHeight ) )
    {
        newHeight = desiredHeight;
        m_animationTimer->stop(); //stop the timeline _before_ changing the direction
        m_goingUp = !m_goingUp;
    }

    setContentRectSize( QSize( m_contentRect->rect().width(), newHeight ) );
}

#include "contextbox.moc"
