/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "Splitter.h"

#include "Debug.h"
#include "SvgHandler.h"

#include <QPainter>

namespace Amarok {


SplitterHandle::SplitterHandle( Qt::Orientation orientation, QSplitter * parent )
    : QSplitterHandle( orientation, parent )
{}

SplitterHandle::~ SplitterHandle()
{}

//commented out as for now, we just use the default one. If we decide to use a customone again, just comment in this code.
/*void SplitterHandle::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event )

    QPixmap handle = The::svgHandler()->renderSvg( "splitter_handle", rect().width(), rect().height(), "splitter_handle" );
    QPainter painter( this );
    painter.drawPixmap( 0, 0, handle );
}*/


Splitter::Splitter( QWidget * parent )
    : QSplitter( parent )
{}

Splitter::Splitter( Qt::Orientation orientation, QWidget * parent )
    : QSplitter( orientation, parent )
{}


Splitter::~Splitter()
{
    DEBUG_BLOCK
}


QSplitterHandle * Splitter::createHandle()
{
    return new SplitterHandle( orientation(),  this );
}


} //namespace Amarok

