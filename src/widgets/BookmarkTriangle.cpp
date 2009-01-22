/***************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
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
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#include "BookmarkTriangle.h"
#include "Debug.h"
#include <QPainter>
#include <QPolygon>
#include <QSize>
#include <QSizePolicy>

BookmarkTriangle::BookmarkTriangle ( QWidget *parent, int seconds, QString trackUrl  ) : QWidget ( parent ), m_seconds( seconds ), m_trackUrl( trackUrl )
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
    p.setPen ( Qt::white );
    p.setBrush ( Qt::white );
    QPolygon triangle;
    triangle << QPoint ( 0, 0 ) << QPoint ( 10, 0 ) << QPoint ( 5 , 10 );
    p.drawPolygon ( triangle );
}

void BookmarkTriangle::mouseReleaseEvent ( QMouseEvent * event )
{
    Q_UNUSED( event )
    emit clicked( m_seconds );
}

#include "BookmarkTriangle.moc"

