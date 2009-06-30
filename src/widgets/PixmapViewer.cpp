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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PixmapViewer.h"
#include "Debug.h"

#include <KApplication>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>


PixmapViewer::PixmapViewer( QWidget *widget, const QPixmap pix )
        : QWidget( widget )
{

    m_pixmap = new QPixmap( pix );
    m_zoomFactor = 1.0; // initial zoom

    if ( KApplication::desktop()->width() < m_pixmap->width() )
    {
        m_zoomFactor = ( ( float ) KApplication::desktop()->width() /
                        ( float ) m_pixmap->width() ) - 0.5;
    }

    setMinimumSize( m_pixmap->width() * m_zoomFactor, m_pixmap->height() * m_zoomFactor );

    // move window to the center of the screen
    // (multiple screens: same screen as parent widget)
    QWidget *p = dynamic_cast<QWidget*>( parent() );
    p->move( ( KApplication::desktop()->availableGeometry( p ).width()
              - ( m_pixmap->width() * m_zoomFactor ) ) / 2,
            ( KApplication::desktop()->availableGeometry( p ).height()
              - ( m_pixmap->height() * m_zoomFactor ) ) / 2 );

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

    repaint();
}

void
PixmapViewer::paintEvent( QPaintEvent *event )
{
    int xoffset, yoffset;
    bool drawBorder = false;

    if ( width() > m_pixmap->width() * m_zoomFactor )
    {
        xoffset = ( width() - m_pixmap->width() * m_zoomFactor ) / 2;
        drawBorder = true;
    }
    else
    {
        xoffset = 0;
    }

    if ( height() > m_pixmap->height()*m_zoomFactor )
    {
        yoffset = ( height() - m_pixmap->height() * m_zoomFactor ) / 2;
        drawBorder = true;
    }
    else
    {
        yoffset = 0;
    }

    QWidget *parentWidget = dynamic_cast<QWidget*>( parent() );
    parentWidget->move( ( KApplication::desktop()->availableGeometry( parentWidget ).width()
                         - ( m_pixmap->width() * m_zoomFactor ) ) / 2,
                       ( KApplication::desktop()->availableGeometry( parentWidget ).height()
                         - ( m_pixmap->height() * m_zoomFactor ) ) / 2 );

    QPainter p( this );
    p.save();
    p.translate( xoffset, yoffset );
    p.scale( m_zoomFactor, m_zoomFactor );
    p.drawPixmap( 0, 0, *m_pixmap );
    p.restore();
    if ( drawBorder )
    {
        p.setPen( Qt::black );
        p.drawRect( xoffset - 1, yoffset - 1, m_pixmap->width() * m_zoomFactor + 1,
                    m_pixmap->height() * m_zoomFactor + 1 );
    }
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
