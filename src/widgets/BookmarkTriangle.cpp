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

#include "EngineController.h"
#include "MainWindow.h"
#include "SvgHandler.h"
#include "amarokurls/BookmarkModel.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QMenu>
#include <QPainter>
#include <QSize>
#include <QSizePolicy>

BookmarkTriangle::BookmarkTriangle (QWidget *parent, int milliseconds, const QString &name,
                                     int sliderwidth, bool showPopup )
    : QWidget ( parent ),
    m_mseconds ( milliseconds ),
    m_name ( name ),
    m_sliderwidth ( sliderwidth ),
    m_showPopup ( showPopup ),
    m_tooltip ( nullptr ),
    m_pos ( 0 )
{
}

BookmarkTriangle::~BookmarkTriangle()
{
    DEBUG_BLOCK
    if (m_tooltip)
      m_tooltip->deleteLater();
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

void BookmarkTriangle::showEvent ( QShowEvent * event )
{
    Q_UNUSED( event );  //FIXME: event->accept() should probably be called

    if ( m_showPopup )
    {
        m_showPopup = false; // Force immediate Popup Display after editing
        initPopup();
    }
}

void BookmarkTriangle::mousePressEvent ( QMouseEvent * event )
{
    event->accept();
    m_offset = event->pos();
    m_pos = this->x();
}

void BookmarkTriangle::mouseMoveEvent ( QMouseEvent * event )
{
    event->accept();
    int distance_x = event->x() - m_offset.x();
    QPoint pt(distance_x, 0);
    move(mapToParent( pt ));
}

void BookmarkTriangle::mouseReleaseEvent ( QMouseEvent * event )
{
    event->accept();

    if( this->x() == m_pos ){
        Q_EMIT clicked ( m_mseconds );
    }
    else
    {
        if( this->x() < 0 || this->x() > m_sliderwidth )
        {
            this->setGeometry(m_pos, 1, 11, 11);
            this->update();
        }
        else{
            qreal percentage = (qreal) ( this->x() ) / (qreal) m_sliderwidth;
            long trackLength = The::engineController()->trackLength();
            qint64 trackPosition = trackLength * percentage;
            moveBookmark( trackPosition, m_name );
        }
    }
}

void BookmarkTriangle::moveBookmark ( qint64 newMilliseconds, const QString &name )
{
    hidePopup();
    Meta::TrackPtr track = The::engineController()->currentTrack();
    PlayUrlGenerator::instance()->moveTrackBookmark( track, newMilliseconds, name );
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

    Q_EMIT focused ( m_mseconds );
    initPopup();
}

void BookmarkTriangle::leaveEvent ( QEvent * event )
{
    DEBUG_BLOCK
    Q_UNUSED ( event )
    if (m_tooltip)
        m_tooltip->displayNeeded(false);
}

void BookmarkTriangle::initPopup()
{
    if ( !m_tooltip )  m_tooltip = new BookmarkPopup ( The::mainWindow(), m_name , this );
    // Keep existing tooltip alive
    m_tooltip->displayNeeded(true);

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

void BookmarkTriangle::hidePopup()
{
    if ( m_tooltip )  m_tooltip->hide();
}

