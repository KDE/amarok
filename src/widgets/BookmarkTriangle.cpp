/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "BookmarkTriangle.h"
#include "Debug.h"
#include "MetaUtility.h"
#include "SvgHandler.h"

#include <KLocale>

#include <QPainter>
#include <QSize>
#include <QSizePolicy>

BookmarkTriangle::BookmarkTriangle ( QWidget *parent, int milliseconds, QString name ) : QWidget ( parent ), m_mseconds ( milliseconds ), m_name ( name ), m_tooltip ( 0 )
{
}

BookmarkTriangle::~BookmarkTriangle()
{}

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

void BookmarkTriangle::paintEvent ( QPaintEvent* )
{
    QPainter p ( this );
    p.drawPixmap ( 0, 0, The::svgHandler()->renderSvg ( "blue_triangle", 10 , 10, "blue_triangle" ) ); // TODO: This doesn't work
}

void BookmarkTriangle::mousePressEvent ( QMouseEvent * event )
{
    Q_UNUSED( event )
// we don't do anything here, but we want to prevent the event from being
// propagated to the parent.
}

void BookmarkTriangle::mouseReleaseEvent ( QMouseEvent * event )
{
    Q_UNUSED ( event )
    emit clicked ( m_mseconds );
}

void BookmarkTriangle::enterEvent ( QEvent * event )
{
    DEBUG_BLOCK
    Q_UNUSED( event )
    debug() << The::svgHandler()->themeFile();
    QString timeLabel = Meta::secToPrettyTime( m_mseconds/1000 );
    if ( !m_tooltip )
        m_tooltip = new BookmarkPopup ( nativeParentWidget(), m_name );
    QPoint pt = mapTo( nativeParentWidget(), QPoint( 0, 0 ) );
    m_tooltip->move ( pt.x(), 25 ); //TODO better way to calculate the y position
    m_tooltip->show();
}

void BookmarkTriangle::leaveEvent ( QEvent * event )
{
    Q_UNUSED( event )
    if ( m_tooltip )
        m_tooltip->hide();
}
#include "BookmarkTriangle.moc"

