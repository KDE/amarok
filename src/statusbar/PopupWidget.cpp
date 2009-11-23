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

#include "PopupWidget.h"
#include "MainWindow.h"

#include "Debug.h"


PopupWidget::PopupWidget( QWidget * anchor, const QString &name )
        : KVBox( The::mainWindow() )
        , m_anchor( anchor )
{
    Q_UNUSED( name );

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
{
    DEBUG_BLOCK
}

void PopupWidget::reposition()
{
    adjustSize();

    // p is no longer in the anchor's coordinates
    QPoint p;

    p.setX( m_anchor->x() + ( m_anchor->width() - width() ) );
    p.setY( m_anchor->y() - height() );
    move( p );
}
