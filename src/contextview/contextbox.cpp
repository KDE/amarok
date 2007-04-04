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
    setPos(100, 300);


    m_titleItem = new QGraphicsTextItem( "", this, scene );
    m_titleItem->setDefaultTextColor( QColor(255, 255, 255 ) );
    // increase the font size for the title
    QFont font = m_titleItem->font();
    font.setPointSize( 14 );
    font.setBold( true );
    m_titleItem->setFont( font );


    m_contentRect = new QGraphicsRectItem( this, scene );
    m_contentRect->setRect( 0, 0, boundingRect.width(), boundingRect.height() -  m_titleItem->boundingRect().height());
    m_contentRect->setPos(0 , m_titleItem->boundingRect().height());
     
}

void ContextBox::setTitle( const QString &title )
{
    m_titleItem->setPlainText( title );
}

void ContextBox::setBoundingRectSize( const QSize &sz )
{
    QRectF newRect = QRectF( 0, 0, sz.width(), sz.height() );
    setRect( newRect );
}

void ContextBox::setContentRectSize( const QSize &sz ) {
   
     m_contentRect->setRect( QRectF( 0, 0, sz.width(), sz.height() ) );
    //set correct size of this as well
    setRect( QRectF( 0, 0, sz.width(), sz.height() +  m_titleItem->boundingRect().height()) );

}
