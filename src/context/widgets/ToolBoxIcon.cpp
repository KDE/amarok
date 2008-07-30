/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/
#include "ToolBoxIcon.h"

ToolBoxIcon::ToolBoxIcon( QGraphicsItem *parent )
    : Plasma::Icon( parent )
{}

QPainterPath
ToolBoxIcon::shape() const
{
    QPainterPath path;
    QSizeF toolSize( size().width() + 10, size().height() + 10 );
    path.arcTo( QRectF( QPointF( 0.0, 0.0 ), toolSize ).adjusted( -2, -2, 2, 2 ), 0, 360 );
    return path;
}

#include "ToolBoxIcon.moc"