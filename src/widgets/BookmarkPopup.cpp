/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "BookmarkPopup.h"
#include "BookmarkTriangle.h"
#include "SvgHandler.h"
#include "Debug.h"


#include <KLocale>
#include <KPixmapCache>

#include <QPainter>


BookmarkPopup::BookmarkPopup ( QWidget* parent, QString label, BookmarkTriangle* triangle )
        : QWidget ( parent )
        , m_label ( label )
        , m_triangle ( triangle )

{

    m_deleteIcon = KIcon ( "edit-delete" );

    //calculate height and width
    const int margin = 2;
    QFontMetrics fm ( font() );
    m_lineHeight = fm.height();

    int line1Width = fm.width ( i18n ( "Bookmark" ) ) + 40; //padding and space for delete icon
    int line2Width = fm.width ( m_label );

    m_height = 44;
    m_width = qMax ( line1Width, line2Width ) + 2 * margin;
    setGeometry ( 0, 0, m_width, m_height );

    m_deleteIconRect = QRect( m_width - 20, 4, 16, 16 );

    m_hasMouseOver = false;
    m_overDelete = false;
    setMouseTracking ( true );

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

bool BookmarkPopup::hasMouseOver()
{
    return m_hasMouseOver;
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

    if ( m_overDelete ) p.setOpacity ( m_overDelete ?  1 : 0.1 );
    p.drawPixmap ( m_deleteIconRect.x(), m_deleteIconRect.y(), m_deleteIcon.pixmap ( 16 ) );

    p.setOpacity ( 1 );
    p.drawPixmap ( 5, 1, The::svgHandler()->renderSvg ( "bookmark", 6, 20, "bookmark" ) );

    p.setPen ( Qt::gray );
    rect = QRect ( 15, 3, m_width, m_lineHeight );
    p.drawText ( rect, Qt::AlignLeft, i18n ( "Bookmark" ) );

    p.setPen ( Qt::black );
    rect = QRect ( 0, m_lineHeight + 6, m_width, m_lineHeight );
    p.drawText ( rect, Qt::AlignCenter, m_label );
}

void BookmarkPopup::mouseReleaseEvent ( QMouseEvent * event )
{
    if ( event->button() == Qt::LeftButton && isOverDeleteIcon ( event->pos() ) )
    {
        m_triangle->deleteBookmark();
    }
}

void BookmarkPopup::mouseMoveEvent ( QMouseEvent * event )
{
    bool state = isOverDeleteIcon ( event->pos() );
    if ( state != m_overDelete )
    {
        m_overDelete = state;
        this->update();
    }
}

void BookmarkPopup::enterEvent ( QEvent* )
{
    m_hasMouseOver = true;
}

void BookmarkPopup::leaveEvent ( QEvent* )
{
    m_hasMouseOver = false;
}

bool BookmarkPopup::isOverDeleteIcon ( QPoint pos )
{
    return m_deleteIconRect.contains( pos );
}
