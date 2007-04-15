/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
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

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QRectF>

using namespace Context;

ContextBox::ContextBox( QGraphicsItem *parent, QGraphicsScene *scene )
    : QGraphicsRectItem( parent, scene )
      , m_titleItem( 0 )
{
    const QRectF boundingRect = QRectF( 0, 0, 400, 200 );
    setRect( boundingRect );

    setPen( QPen( Qt::black, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin ) );

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
    m_contentRect->setPos( 0 , m_titleBarRect->boundingRect().height() );
    m_contentRect->setPen( Qt::NoPen );

    //make a nice shadow
    QLinearGradient shadowGradient( QPointF( 0, 0 ), QPointF( 0, 10) );
    shadowGradient.setColorAt( 0, QColor( 150, 150, 150 ) );
    shadowGradient.setColorAt( 1, QColor( 255, 255, 255 ) );
    m_contentRect->setBrush( QBrush( shadowGradient ) );
}

void ContextBox::setTitle( const QString &title )
{
    m_titleItem->setPlainText( title );

    qreal titleWidth = m_titleItem->boundingRect().width();
    // If the title is too big for the box, make the box bigger
    if( titleWidth > m_titleBarRect->boundingRect().width() )
    {
        /*
        // Make the context box wider
        QRectF conRect = m_contentRect->boundingRect();
        conRect.setWidth( titleWidth );
        m_contentRect->setRect( conRect );

        // make the title box bigger
        QRectF titleRect = m_titleBarRect->boundingRect();
        titleRect.setWidth( titleWidth );
        m_titleBarRect->setRect( titleRect );

        QRectF boxRect = boundingRect();
        boxRect.setWidth( titleWidth );
        setRect( boxRect );
*/
        // this function takes care of setting everything to the correct size!
        setContentRectSize( QSize( titleWidth, m_contentRect->boundingRect().height() ) );
    }

    // Center the title
    qreal xOffset = ( m_titleBarRect->boundingRect().width() - titleWidth ) / 2;
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
}
