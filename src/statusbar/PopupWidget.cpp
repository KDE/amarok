/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "core/support/Debug.h"


PopupWidget::PopupWidget( const QString &name )
    : BoxWidget( true )
{
    Q_UNUSED( name );

    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );

    setFrameStyle( QFrame::Box );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );

    setContentsMargins( 4, 4, 4, 4 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

PopupWidget::~PopupWidget()
{
    DEBUG_BLOCK
}

void PopupWidget::reposition()
{
    adjustSize();

    if( !The::mainWindow() )
        return;

    //HACK: put longmessage popup in the bottom right of the window.
    QPoint p;
    p.setX( The::mainWindow()->width() - width() );
    p.setY( The::mainWindow()->height() - height() );
    move( p );
}
