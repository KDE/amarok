/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "PopupWidget.h"
#include "MainWindow.h"

#include "Debug.h"

#include <QToolTip>

PopupWidget::PopupWidget( QWidget * anchor, const QString &name )
        : KVBox( The::mainWindow() )
        , m_anchor( anchor )
{
    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );

    setFrameStyle( QFrame::Box );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );

    setContentsMargins( 4, 4, 4, 4 );
    setSpacing( 0 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    reposition();
}

PopupWidget::~PopupWidget()
{}

void PopupWidget::reposition()
{
    adjustSize();

    // p is in the anchor's coordinates
    QPoint p;

    p.setX( m_anchor->x() );
    p.setY( m_anchor->y() - ( height() + 4 ) );
    debug() << "p before: " << p;
    p = m_anchor->mapTo( The::mainWindow(), p );
    debug() << "p after: " << p;

    move( p );
}


