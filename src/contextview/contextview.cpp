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

#include "contextview.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>


ContextView *ContextView::s_instance = 0;

ContextView::ContextView()
    : QGraphicsView()
{
    s_instance = this; // we are a singleton class

    m_contextScene = new QGraphicsScene( this );
    m_contextScene->setItemIndexMethod( QGraphicsScene::BspTreeIndex ); //mainly static, so let's keep an index

    QColor color = QColor( 210, 53, 210 );
    QBrush brush = QBrush( color, Qt::LinearGradientPattern );
    setBackgroundBrush( brush );

    setRenderHints( QPainter::Antialiasing );
    setCacheMode( QGraphicsView::CacheBackground ); // this won't be changing regularly
    setScene( m_contextScene );

    showHome();
}


void ContextView::showHome()
{
    QGraphicsTextItem *welcome = m_contextScene->addText( "Hooray, and welcome to Amarok::ContextView!" );
    welcome->setDefaultTextColor( Qt::black );
    welcome->setPos( 5, 5 );
}



