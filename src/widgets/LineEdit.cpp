/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "LineEdit.h"

#include <QKeyEvent>

namespace Amarok
{

LineEdit::LineEdit( QWidget *parent )
   : KLineEdit( parent )
{
}

void LineEdit::keyPressEvent( QKeyEvent *event )
{
    if( event->key() == Qt::Key_Escape )
    {
        event->accept();
        clear();
        return;
    }
    else if( event->key() == Qt::Key_Down )
    {
        event->accept();
        emit downPressed();
        return;
    }
    KLineEdit::keyPressEvent( event );
}

}

#include "LineEdit.moc"
