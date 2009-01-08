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

#include "AppletToolbarItem.h"

#include "Debug.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>

Context::AppletToolbarItem::AppletToolbarItem( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
{
    DEBUG_BLOCK
}

Context::AppletToolbarItem::~AppletToolbarItem()
{
    
}

void 
Context::AppletToolbarItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
  //  DEBUG_BLOCK
    
    painter->save();
    QColor fillColor( 88, 88, 88, 225 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 10, 10 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
}


#include "AppletToolbarItem.moc"
