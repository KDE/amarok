/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <kdebug.h>
#include "overlayWidget.h"
#include <qpoint.h>


namespace KDE {


OverlayWidget::OverlayWidget( KDE::StatusBar *statusbar, QWidget *anchor, const char* name )
    #define statusbar reinterpret_cast<QWidget*>(statusbar)
        : QFrame( statusbar->parentWidget(), name )
        , m_anchor( anchor )
{
    statusbar->installEventFilter( this );
    #undef statusbar

    hide();
}

void
OverlayWidget::reposition()
{
    adjustSize();

    // p is in the alignWidget's coordinates
    QPoint p;
    // We are always above the alignWidget, right-aligned with it.
    p.setX( m_anchor->width() - width() );
    p.setY( -height() );
    // Position in the toplevelwidget's coordinates
    QPoint pTopLevel = m_anchor->mapTo( topLevelWidget(), p );
    // Position in the widget's parentWidget coordinates
    QPoint pParent = parentWidget() ->mapFrom( topLevelWidget(), pTopLevel );
    // keep it on the screen
    if ( pParent.x() < 0 )
        pParent.rx() = 0;
    // Move 'this' to that position.
    move( pParent );
}

bool
OverlayWidget::eventFilter( QObject* o, QEvent* e )
{
    if ( e->type() == QEvent::Move || e->type() == QEvent::Resize )
        reposition();

    return QFrame::eventFilter( o, e );
}

void
OverlayWidget::resizeEvent( QResizeEvent* ev )
{
    reposition();
    QFrame::resizeEvent( ev );
}

bool
OverlayWidget::event( QEvent *e )
{
    if ( e->type() == QEvent::ChildInserted )
        adjustSize();

    return QFrame::event( e );
}

}
