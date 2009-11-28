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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CustomVideoWidget.h"
#include "Debug.h"

#include <KAction>
#include <KMenu>

#include <QKeyEvent>

#define DEBUG_PREFIX "CustomVideoWidget"

using namespace Phonon;

void CustomVideoWidget::mouseDoubleClickEvent( QMouseEvent* )
{
    // If we already are in full screen
    if ( !isFullScreen() )
    {
        enableFullscreen();
    }
    else
    {
        disableFullscreen();
    }
}

void
CustomVideoWidget::keyPressEvent( QKeyEvent *e )
{
    if( !isFullScreen() )
    {
        Phonon::VideoWidget::keyPressEvent( e );
    }
    else
    {
        if( e->key() == Qt::Key_Escape )
        {
            disableFullscreen();
        }
    }
}

void
CustomVideoWidget::mousePressEvent( QMouseEvent* e )
{
    switch( e->button() )
    {
    case Qt::RightButton :
            videoMenu( e->globalPos() );
            break;

    case Qt::LeftButton :
    case Qt::MidButton :
    case Qt::MouseButtonMask :
    case Qt::NoButton :
    case Qt::XButton1 :
    case Qt::XButton2 :
            break;
    }
}

void
CustomVideoWidget::videoMenu( QPoint point )
{
    KMenu *men = new KMenu( this );
    if ( !isFullScreen() )
    {
        KAction *toggle = new KAction( KIcon( "view-fullscreen" ), i18n( "Enter &fullscreen" ), this );
        men->addAction( toggle );
        connect( toggle, SIGNAL( triggered(bool) ), this, SLOT( enableFullscreen() ) );
    }
    else
    {
        KAction *toggle = new KAction( KIcon( "edit-undo" ), i18n( "E&xit fullscreen" ), this );
        men->addAction( toggle );
        connect( toggle, SIGNAL( triggered(bool) ), this, SLOT( disableFullscreen() ) );
    }
    men->exec( point );
}

void
CustomVideoWidget::enableFullscreen()
{
     m_parent = parentWidget();
     m_rect = geometry();
     setWindowFlags( Qt::Window );
     setFullScreen( true );
}

void
CustomVideoWidget::disableFullscreen()
{
    setFullScreen( false );
    setParent( m_parent, Qt::SubWindow | Qt::FramelessWindowHint );
    setGeometry( m_rect );
    show();
}

