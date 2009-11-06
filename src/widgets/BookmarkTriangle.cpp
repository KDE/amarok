/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org                             *
 * Copyright (c) 2009 Simon Bühler <simon@aktionspotenzial.de>                          *
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

#include "BookmarkTriangle.h"


#include "BookmarkModel.h"
#include "Debug.h"
#include "MainWindow.h"
#include "MetaUtility.h"
#include "SvgHandler.h"

#include <KLocale>

#include <QMenu>
#include <QPainter>
#include <QSize>
#include <QSizePolicy>

BookmarkTriangle::BookmarkTriangle ( QWidget *parent, int milliseconds, QString name )
        : QWidget ( parent ),
        m_mseconds ( milliseconds ),
        m_name ( name ),
        m_tooltip ( 0 )
{
    m_timerId = 0;
}

BookmarkTriangle::~BookmarkTriangle()
{
    DEBUG_BLOCK
}

QSize BookmarkTriangle::sizeHint() const
{
    return QSize ( 10, 10 );
}

QSizePolicy BookmarkTriangle::sizePolicy() const
{
    return QSizePolicy ( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

QSize BookmarkTriangle::minimumSizeHint() const
{
    return QSize ( 10, 10 );
}

int BookmarkTriangle::getTimeValue()
{
    return m_mseconds;
}

void BookmarkTriangle::paintEvent ( QPaintEvent* )
{
    QPainter p ( this );
    p.drawPixmap ( 0, 0, The::svgHandler()->renderSvg ( "blue_triangle", 10 , 10, "blue_triangle" ) ); // TODO: This doesn't work
}


void BookmarkTriangle::mousePressEvent ( QMouseEvent * event )
{
    Q_UNUSED ( event )
// we don't do anything here, but we want to prevent the event from being
// propagated to the parent.
}

void BookmarkTriangle::mouseReleaseEvent ( QMouseEvent * event )
{
   emit clicked ( m_mseconds );
}

void BookmarkTriangle::deleteBookmark ()
{
    DEBUG_BLOCK

    debug() << "Name: " << m_name;
    hidePopup();
    BookmarkModel::instance()->deleteBookmark ( m_name );

}

void BookmarkTriangle::enterEvent ( QEvent * event )
{
    DEBUG_BLOCK
    Q_UNUSED ( event )
    emit focused ( m_mseconds );
    if ( m_timerId != 0 )
    {
        // Keep Popup displayed, cancel a existing Hide-Timer here
        killTimer ( m_timerId );
        m_timerId = 0;
        return;
    }
    QString timeLabel = Meta::secToPrettyTime ( m_mseconds/1000 );
    if ( !m_tooltip )
        m_tooltip = new BookmarkPopup ( The::mainWindow(), m_name , this );
    QPoint pt = mapTo ( The::mainWindow(), QPoint ( 0, 0 ) );
    // Calculate x position where the tooltip is fully visible
    int offsetX = pt.x() + m_tooltip->width() - The::mainWindow()->width();
    if ( offsetX < 0 ) offsetX = 0;
    // Calculate y position above
    int offsetY =  - m_tooltip->height() - 2;
    // Not enough space? put it below
    if ( pt.y() <= m_tooltip->height() + 2 ) offsetY =  this->height() + 2;
    m_tooltip->move ( pt.x() - offsetX, pt.y() + offsetY );
    m_tooltip->show();
}

void BookmarkTriangle::leaveEvent ( QEvent * event )
{
    Q_UNUSED ( event )
    m_timerId = startTimer ( 500 );
}
void BookmarkTriangle::timerEvent ( QTimerEvent * event )
{
    if ( event->timerId() == m_timerId )
    {
        if ( m_tooltip && !m_tooltip->hasMouseOver() )
        {
            m_tooltip->hide();
            killTimer ( m_timerId );
            m_timerId = 0;
        }
    }
    else
    {
        QWidget::timerEvent ( event );
    }
}

void BookmarkTriangle::hidePopup()
{
    if ( m_tooltip )
        m_tooltip->hide();
    if ( m_timerId != 0 )
    {
        killTimer ( m_timerId );
        m_timerId = 0;
    }
}
#include "BookmarkTriangle.moc"

