/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "Widget.h"

#include "PaletteHandler.h"

Amarok::Widget::Widget( QWidget * parent )
    : QWidget( parent )
{
    QPalette p = palette();
    QColor c = p.color( QPalette::Window );
    c.setAlpha( 0 );
    p.setColor( QPalette::Window, c );
    setPalette( p );

    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );
}

void Amarok::Widget::newPalette( const QPalette & palette )
{
    QPalette p = palette;
    QColor c = p.color( QPalette::Window );
    c.setAlpha( 0 );
    p.setColor( QPalette::Window, c );
    setPalette( p );
}

#include "Widget.moc"
