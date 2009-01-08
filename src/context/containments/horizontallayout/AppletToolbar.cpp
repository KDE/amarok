/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbar.h"

#include "Debug.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>

Context::AppletToolbar::AppletToolbar( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
{
    DEBUG_BLOCK
}

Context::AppletToolbar::~AppletToolbar()
{
    
}

void 
Context::AppletToolbar::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    DEBUG_BLOCK
    
    debug() << "drawing rect:" << boundingRect();
    painter->save();
    painter->setPen( QColor( Qt::blue ) );   
    painter->setOpacity( 0.75 ); 
    painter->drawRect( boundingRect() );
    painter->restore();
}

void
Context::AppletToolbar::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    
}

#include "AppletToolbar.moc"
