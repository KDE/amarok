/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "HorizontalDivider.h"
#include "SvgHandler.h"

#include <QPainter>

HorizontalDivider::HorizontalDivider( QWidget * parent )
    : QWidget( parent )
{
    setFixedHeight( 2 );
}


HorizontalDivider::~HorizontalDivider()
{
}

void HorizontalDivider::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event )

    QPainter p( this );
    
    p.drawPixmap( 0, 0, The::svgHandler()->renderSvg( "divider_bottom", rect().width(),  1, "divider_bottom" ) );
    p.drawPixmap( 0, 1, The::svgHandler()->renderSvg( "divider_top", rect().width(), 1, "divider_top" ) );
}


