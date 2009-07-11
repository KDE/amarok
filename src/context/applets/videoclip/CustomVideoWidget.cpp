/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CustomVideoWidget.h"

// Amarok
#include "Debug.h"


// QT


#define DEBUG_PREFIX "CustomVideoWidget"

using namespace Phonon;


void CustomVideoWidget::mouseDoubleClickEvent( QMouseEvent* )
{
    DEBUG_BLOCK;
    // If we already are in full screen
    if ( !this->isFullScreen() )
    {
        m_parent = this->parentWidget();
        m_rect = this->geometry();
        this->setWindowFlags( Qt::Window );
        this->setFullScreen( true );
        this->setToolTip("");
    }
    else
    {
        this->setFullScreen( false );
        this->setParent( m_parent, Qt::SubWindow | Qt::FramelessWindowHint );
        this->setGeometry( m_rect );
        this->setToolTip("Double-click = full screen !");
        this->show();
        
    }
}

void CustomVideoWidget::keyPressEvent(QKeyEvent* )
{
    ;//QWidget::keyPressEvent();
}

