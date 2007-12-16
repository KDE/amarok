/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "imagebutton.h"
#include <QDesktopServices>
#include <QIcon>
#include <QMouseEvent>


ImageButton::ImageButton( QWidget *parent ) :
        QLabel( parent ),
        m_enabled( true )
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}


void
ImageButton::setPixmap( const QPixmap& pixmap )
{
    m_normal = pixmap;
    QLabel::setPixmap( pixmap );
}


void
ImageButton::setEnabled( bool enabled )
{
    m_enabled = enabled;

    if ( !enabled && !m_disabled.isNull() )
        setPixmap( m_disabled );
    else
        setPixmap( m_normal );
}


void
ImageButton::mousePressEvent( QMouseEvent* e )
{
    if ( e->button() == Qt::LeftButton && m_enabled && !m_down.isNull() )
        setPixmap( m_down );
    else
        setPixmap( m_normal );
}


void
ImageButton::mouseReleaseEvent( QMouseEvent* e )
{
    if ( e->button() == Qt::LeftButton && m_enabled && rect().contains( e->pos() ) )
    {
        setPixmap( m_normal );
        emit clicked();

        if ( !m_url.isEmpty() )
        {
            #ifndef Q_WS_WIN
                QDesktopServices::openUrl( QUrl::fromEncoded( m_url.toString().toUtf8() ) );
            #else
                QDesktopServices::openUrl( m_url );
            #endif
        }
    }
}


void
ImageButton::mouseMoveEvent( QMouseEvent* e )
{
    // we only get this event when the mouse button is pressed
    if ( rect().contains( e->pos() ) && !m_down.isNull() )
        setPixmap( m_down );
    else
        setPixmap( m_normal );
}


void
ImageButton::enterEvent( QEvent* /* e */ )
{
    if ( m_enabled )
    {
        if ( !m_hover.isNull() )
            setPixmap( m_hover );
        else
            setPixmap( m_normal );

        emit urlHovered( m_url.toString() );
    }
}


void
ImageButton::leaveEvent( QEvent* /* e */ )
{
    if ( m_enabled )
    {
        setPixmap( m_normal );
        emit urlHovered( "" );
    }
}
