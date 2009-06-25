/****************************************************************************************
 *  Copyright (c) 2005 Eyal Lotem <eyal.lotem@gmail.com>                  		*
 *  Copyright (c) 2005 Alexandre Oliveira <aleprj@gmail.com>              		*
 *  Copyright (c) 2007 Seb Ruiz <ruiz@kde.org>                            		*
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
#include <QScrollBar>

PixmapViewer::PixmapViewer( QWidget *widget, const QPixmap &pixmap )
    : QScrollArea( widget )
    , m_isDragging( false )
    , m_pixmap( pixmap )
{
    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap( pixmap );
    imageLabel->adjustSize();

    setBackgroundRole( QPalette::Dark );
    setWidget( imageLabel );
}

void PixmapViewer::mousePressEvent(QMouseEvent *event)
{
    if( Qt::LeftButton == event->button() )
    {
        m_currentPos = event->globalPos();
        m_isDragging = true;
    }
}

void PixmapViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if( Qt::LeftButton == event->button() )
    {
        m_currentPos = event->globalPos();
        m_isDragging = false;
    }
}

void PixmapViewer::mouseMoveEvent(QMouseEvent *event)
{
    if( m_isDragging )
    {
        QPoint delta = m_currentPos - event->globalPos();
        horizontalScrollBar()->setValue( horizontalScrollBar()->value() + delta.x() );
        verticalScrollBar()->setValue( verticalScrollBar()->value() + delta.y() );
        m_currentPos = event->globalPos();
    }
}

QSize PixmapViewer::sizeHint() const
{
    return QScrollArea::sizeHint().boundedTo( KApplication::desktop()->size() );
}

#include "PixmapViewer.moc"

