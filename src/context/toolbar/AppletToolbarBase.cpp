/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AppletToolbarBase.h"

#include "PaletteHandler.h"

#include <QPainter>

Context::AppletToolbarBase::AppletToolbarBase( QGraphicsItem* parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
{

}

Context::AppletToolbarBase::~AppletToolbarBase()
{}

void
Context::AppletToolbarBase::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    QColor topColor = The::paletteHandler()->palette().color( QPalette::Base );
    QColor bottomColor = topColor;
    topColor.setAlpha( 200 );
    bottomColor.setAlpha( 100 );
    qreal radius = 3;
    qreal boundWidth = boundingRect().width();
    qreal boundHeight = boundingRect().height();

    // draw top half of rounded applet
    QPainterPath path;
    path.moveTo( 0, boundHeight / 2 );
    path.lineTo( 0, radius );
    path.quadTo( 0, 0, radius, 0 );
    path.lineTo( boundWidth - radius, 0 );
    path.quadTo( boundWidth, 0, boundWidth, radius );
    path.lineTo( boundWidth, boundHeight / 2 );
    path.lineTo( 0, boundHeight / 2 );

    painter->fillPath( path, topColor );
    QPainterPath bottom;
    bottom.moveTo( 0, boundHeight / 2 );
    bottom.lineTo( 0, boundHeight - radius );
    bottom.quadTo( 0, boundHeight, radius, boundHeight );
    bottom.lineTo( boundWidth - radius, boundHeight );
    bottom.quadTo( boundWidth, boundHeight, boundWidth, boundHeight - radius );
    bottom.lineTo( boundWidth, boundHeight / 2 );
    bottom.lineTo( 0, boundHeight / 2 );

    painter->fillPath( bottom, bottomColor );
    painter->restore();
}
