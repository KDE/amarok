/****************************************************************************************
 * Copyright (c) 2005 Eyal Lotem <eyal.lotem@gmail.com>                                 *
 * Copyright (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009 Pascal Pollet <pascal@bongosoft.de>                               *
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

#include "PixmapViewer.h"
#include "core/support/Debug.h"

#include <KApplication>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>


PixmapViewer::PixmapViewer( QWidget *widget, const QPixmap pix, int screenNumber)
        : QWidget( widget )
{

    m_pixmap = new QPixmap( pix );
    m_zoomFactor = 1.0; // initial zoom

    if ( KApplication::desktop()->availableGeometry( screenNumber ).width() < m_pixmap->width() ||
            KApplication::desktop()->availableGeometry( screenNumber ).height() < m_pixmap->height() )
    {
        float zoomFactorX =
            ( (float) KApplication::desktop()->availableGeometry( screenNumber ).width() /
             (float) m_pixmap->width() );
        float zoomFactorY =
            ( (float) KApplication::desktop()->availableGeometry( screenNumber ).height() /
             (float) m_pixmap->height() );

        m_zoomFactor = std::min( zoomFactorX, zoomFactorY ) * 0.8;
    }

    setMinimumSize( m_pixmap->width() * m_zoomFactor, m_pixmap->height() * m_zoomFactor );

}

void
PixmapViewer::setZoomFactor( float f )
{
    int w, h;

    if ( f == m_zoomFactor )
        return;

    m_zoomFactor = f;
    emit( zoomFactorChanged( m_zoomFactor ) );

    w = m_pixmap->width() * m_zoomFactor;
    h = m_pixmap->height() * m_zoomFactor;
    setMinimumSize( w, h );

    QWidget *p = dynamic_cast<QWidget*>( parent() );
    if ( p )
        resize( p->width(), p->height() );

}

void
PixmapViewer::paintEvent( QPaintEvent *event )
{
    Q_UNUSED(event);

    int xoffset, yoffset;

    if ( width() > m_pixmap->width() * m_zoomFactor )
    {
        xoffset = ( width() - m_pixmap->width() * m_zoomFactor ) / 2;
    }
    else
    {
        xoffset = 0;
    }

    if ( height() > m_pixmap->height()*m_zoomFactor )
    {
        yoffset = ( height() - m_pixmap->height() * m_zoomFactor ) / 2;
    }
    else
    {
        yoffset = 0;
    }
    
    QPainter p( this );
    p.save();
    p.translate( xoffset, yoffset );
    p.scale( m_zoomFactor, m_zoomFactor );
    p.drawPixmap( 0, 0, *m_pixmap );
    p.restore();
}

void
PixmapViewer::wheelEvent( QWheelEvent *event )
{
    float f;

    f = m_zoomFactor + 0.001 * event->delta();
    if ( f < 32.0 / m_pixmap->width() )
        f = 32.0 / m_pixmap->width();

    setZoomFactor( f );
}

#include "PixmapViewer.moc"
