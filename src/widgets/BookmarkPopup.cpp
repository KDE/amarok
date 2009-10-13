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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BookmarkPopup.h"
#include "SvgHandler.h"

#include <KLocale>
#include <KPixmapCache>

#include <QPainter>


BookmarkPopup::BookmarkPopup ( QWidget* parent, QString label )
    : QWidget ( parent )
    , m_label( label )
{

    //calculate height and width
    const int margin = 2;
    QFontMetrics fm( font() );

    m_lineHeight = fm.height();

    int line1Width = fm.width( i18n( "Bookmark" ) );
    int line2Width = fm.width( m_label );

    m_height = 44;
    m_width = qMax( line1Width, line2Width ) + 2 * margin;

    setGeometry( 0, 0, m_width, m_height );


}

QSize BookmarkPopup::sizeHint() const
{
    return QSize ( m_width, m_height );
}

QSizePolicy BookmarkPopup::sizePolicy() const
{
    return QSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Minimum );
}

QSize BookmarkPopup::minimumSizeHint() const
{
    return QSize ( m_width, m_height );
}

void BookmarkPopup::paintEvent ( QPaintEvent* )
{
    QPainter p ( this );
    p.setRenderHint ( QPainter::Antialiasing );
    p.setBrush ( Qt::white );
    p.setOpacity ( 0.85 );
    QPen pen = QPen ( Qt::black );
    pen.setCosmetic ( true );
    p.setPen ( pen );
    QRect rect = QRect ( 0,0, m_width, m_height );
    p.drawRoundedRect ( rect, 5, 5 );
    p.setOpacity ( 1 );

    p.drawPixmap ( m_width - 20, 0, The::svgHandler()->renderSvg ( "bookmark", 6, 20, "bookmark" ) );

    p.setPen ( Qt::gray );
    rect = QRect ( 0, 3, m_width, m_lineHeight );
    p.drawText ( rect, Qt::AlignHCenter, i18n ( "Bookmark" ) );

    p.setPen ( Qt::black );
    rect = QRect ( 0, m_lineHeight + 6, m_width, m_lineHeight );
    p.drawText ( rect, Qt::AlignCenter, m_label );
}

