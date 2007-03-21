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
}

void ContextBox::setTitle( const QString &title )
{
    if( !m_titleItem )
    {
        m_titleItem = new QGraphicsTextItem( title, this, scene() );

        // increase the font size for the title
        QFont font = m_titleItem->font();
        font.setPointSize( 12 );
        m_titleItem->setFont( font );

    }
    else
        m_titleItem->setPlainText( title );
}

void ContextBox::setBoundingRectSize( const QSize &sz )
{
    QRectF newRect = QRectF( 0, 0, sz.width(), sz.height() );
    setRect( newRect );
}
