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

#include "BookmarkPopup.h"
#include "SvgHandler.h"

#include <KLocale>
#include <KPixmapCache>

#include <QPainter>

BookmarkPopup::BookmarkPopup ( QWidget* parent, QString label ) : QWidget ( parent ), m_label ( label )
{
}

QSize BookmarkPopup::sizeHint() const
{
    return QSize ( 101, 46 );
}

QSizePolicy BookmarkPopup::sizePolicy() const
{
    return QSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Minimum );
}

QSize BookmarkPopup::minimumSizeHint() const
{
    return QSize ( 101, 41 );
}

void BookmarkPopup::paintEvent ( QPaintEvent* )
{
    QPainter p ( this );
    p.setRenderHint ( QPainter::Antialiasing );
    p.setBrush ( Qt::white );
    p.setOpacity ( 0.7 );
    QPen pen = QPen ( Qt::black );
    pen.setCosmetic ( true );
    p.setPen ( pen );
    QRect rect = QRect ( 0,0, 100, 40 );
    p.drawRoundedRect ( rect, 5, 5 );
    p.setOpacity ( 1 );

    p.drawPixmap ( 70, 0, The::svgHandler()->renderSvg ( "bookmark", 6, 20, "bookmark" ) ); // TODO THIS DOESNT WORK

    p.setPen ( Qt::gray );
    rect = QRect ( 0, 3, 100, 40 );
    p.drawText ( rect, Qt::AlignHCenter, i18n ( "Bookmark" ) );

    p.setPen ( Qt::black );
    rect = QRect ( 0, 10, 100, 40 );
    p.drawText ( rect, Qt::AlignCenter, m_label );
}

