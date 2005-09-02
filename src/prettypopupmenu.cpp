/***************************************************************************
 *   Copyright (C) 2005 by Mark Kretschmann <markey@web.de>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "prettypopupmenu.h"

#include <qpainter.h>
#include <qstyle.h>

#include <kstandarddirs.h>


////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

PrettyPopupMenu::PrettyPopupMenu( QWidget* parent, const char* name )
    : KPopupMenu( parent, name )
    , m_sidePixmap( locate( "data","amarok/images/menu_sidepixmap.png" ) )
{}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

QRect PrettyPopupMenu::sideImageRect()
{
    return QStyle::visualRect( QRect( frameWidth(), frameWidth(), m_sidePixmap.width(),
                                      height() - 2*frameWidth() ), this );
}

void
PrettyPopupMenu::setMinimumSize(const QSize & s)
{
    KPopupMenu::setMinimumSize(s.width() + m_sidePixmap.width(), s.height());
}

void
PrettyPopupMenu::setMaximumSize(const QSize & s)
{
    KPopupMenu::setMaximumSize(s.width() + m_sidePixmap.width(), s.height());
}

void
PrettyPopupMenu::setMinimumSize(int w, int h)
{
    KPopupMenu::setMinimumSize(w + m_sidePixmap.width(), h);
}

void
PrettyPopupMenu::setMaximumSize(int w, int h)
{
  KPopupMenu::setMaximumSize(w + m_sidePixmap.width(), h);
}

void PrettyPopupMenu::resizeEvent(QResizeEvent * e)
{
    KPopupMenu::resizeEvent( e );

    setFrameRect( QStyle::visualRect( QRect( m_sidePixmap.width(), 0,
                                      width() - m_sidePixmap.width(), height() ), this ) );
}

//Workaround Qt3.3.x sizing bug, by ensuring we're always wide enough.
void PrettyPopupMenu::resize( int width, int height )
{
    width = kMax(width, maximumSize().width());
    KPopupMenu::resize(width, height);
}

void
PrettyPopupMenu::paintEvent( QPaintEvent* e )
{
    if ( m_sidePixmap.isNull()) {
        KPopupMenu::paintEvent(e);
        return;
    }

    QPainter p(this);
    p.setClipRegion(e->region());

    style().drawPrimitive( QStyle::PE_PanelPopup, &p,
                           QRect( 0, 0, width(), height() ),
                           colorGroup(), QStyle::Style_Default,
                           QStyleOption( frameWidth(), 0 ) );

    QRect r = sideImageRect();
    r.setTop( r.bottom() - m_sidePixmap.height() );
    if ( r.intersects( e->rect() ) )
    {
        QRect drawRect = r.intersect( e->rect() );
        QRect pixRect = drawRect;
        pixRect.moveBy( -r.left(), -r.top() );
        p.drawPixmap( drawRect.topLeft(), m_sidePixmap, pixRect );
    }

    drawContents( &p );
}


#include "prettypopupmenu.moc"
