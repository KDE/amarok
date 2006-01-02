/*
   This file is part of libkdepim.
   Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>
   based on code by Cornelius Schumacher <schumacher@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "clicklineedit.h"

#include "qpainter.h"


ClickLineEdit::ClickLineEdit( const QString &msg, QWidget *parent, const char* name ) :
        KLineEdit( parent, name )
{
    mDrawClickMsg = true;
    setClickMessage( msg );
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
/////////////////////////////////////////////////////////////////////////////////////

void ClickLineEdit::setClickMessage( const QString &msg )
{
    mClickMessage = msg;
    repaint();
}


void ClickLineEdit::setText( const QString &txt )
{
    mDrawClickMsg = txt.isEmpty();
    repaint();
    KLineEdit::setText( txt );
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

//#include <kiconloader.h>
void ClickLineEdit::drawContents( QPainter *p )
{
    KLineEdit::drawContents( p );

    if ( mDrawClickMsg == true && !hasFocus() ) {
        QPen tmp = p->pen();
        p->setPen( palette().color( QPalette::Disabled, QColorGroup::Text ) );
        QRect cr = contentsRect();

        //p->drawPixmap( 3, 3, SmallIcon("filter") );

        // Add two pixel margin on the left side
        cr.rLeft() += 3;
        p->drawText( cr, AlignAuto | AlignVCenter, mClickMessage );
        p->setPen( tmp );
    }
}

void ClickLineEdit::dropEvent( QDropEvent *ev )
{
    mDrawClickMsg = false;
    KLineEdit::dropEvent( ev );
}


void ClickLineEdit::focusInEvent( QFocusEvent *ev )
{
    if ( mDrawClickMsg == true ) {
        mDrawClickMsg = false;
        repaint();
    }
    QLineEdit::focusInEvent( ev );
}


void ClickLineEdit::focusOutEvent( QFocusEvent *ev )
{
    if ( text().isEmpty() ) {
        mDrawClickMsg = true;
        repaint();
    }
    QLineEdit::focusOutEvent( ev );
}

#include "clicklineedit.moc"
