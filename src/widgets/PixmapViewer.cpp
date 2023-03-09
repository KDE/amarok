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

#include <QApplication>

#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QScreen>
#include <QWheelEvent>


PixmapViewer::PixmapViewer( QWidget *parent, const QPixmap &pix, int screenNumber )
    : QWidget( parent )
    , m_pixmap( pix )
{
    m_zoomFactor = 1.0; // initial zoom

    int screenWidth = QApplication::screens()[ screenNumber ]->availableGeometry().width();
    int screenHeight = QApplication::screens()[ screenNumber ]->availableGeometry().height();
    if( screenWidth < m_pixmap.width() || screenHeight < m_pixmap.height() )
    {
        qreal zoomFactorX = qreal(screenWidth) / m_pixmap.width();
        qreal zoomFactorY = qreal(screenHeight) / m_pixmap.height();
        m_zoomFactor = qMin( zoomFactorX, zoomFactorY ) * 0.8;
    }
    setMinimumSize( m_pixmap.width() * m_zoomFactor, m_pixmap.height() * m_zoomFactor );
}

PixmapViewer::~PixmapViewer()
{
}

qreal
PixmapViewer::zoomFactor() const
{
    return m_zoomFactor;
}

void
PixmapViewer::setZoomFactor( qreal f )
{
    int w, h;

    if( f == m_zoomFactor )
        return;

    m_zoomFactor = f;
    Q_EMIT( zoomFactorChanged( m_zoomFactor ) );

    w = m_pixmap.width() * m_zoomFactor;
    h = m_pixmap.height() * m_zoomFactor;
    setMinimumSize( w, h );

    QWidget *p = dynamic_cast<QWidget*>( parent() );
    if( p )
        resize( p->width(), p->height() );
}

void
PixmapViewer::paintEvent( QPaintEvent *event )
{
    int xoffset, yoffset;

    if( width() > m_pixmap.width() * m_zoomFactor )
    {
        xoffset = ( width() - m_pixmap.width() * m_zoomFactor ) / 2;
    }
    else
    {
        xoffset = 0;
    }

    if( height() > m_pixmap.height() * m_zoomFactor )
    {
        yoffset = ( height() - m_pixmap.height() * m_zoomFactor ) / 2;
    }
    else
    {
        yoffset = 0;
    }
    
    QPainter p( this );
    p.save();
    p.translate( xoffset, yoffset );
    p.scale( m_zoomFactor, m_zoomFactor );
    p.drawPixmap( 0, 0, m_pixmap );
    p.restore();

    event->accept();
}

void
PixmapViewer::wheelEvent( QWheelEvent *event )
{
    qreal f = m_zoomFactor + 0.001 * event->angleDelta().y(); //FIXME: check if .x() must be used
    qreal ratio = 32.0 / m_pixmap.width();
    if( f < ratio )
        f = ratio;
    setZoomFactor( f );
    event->accept();
}

