/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "PlaylistTextItem.h"

#include <QFontMetricsF>

QFontMetricsF* Playlist::TextItem::s_fm = 0;


Playlist::TextItem::TextItem( QGraphicsItem* parent )
    : QGraphicsTextItem( parent )
{
    if( !s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
    }
    //setDefaultTextColor( Qt::white );

}

///method assumes text is currently not being edited
void
Playlist::TextItem::setEditableText( const QString& text, qreal width )
{
    m_fullText = text;
    m_width = width;
    setPlainText( s_fm->elidedText( text, Qt::ElideRight, width ) );
}

///Prepare for editing, requires discarding the eliding.
void
Playlist::TextItem::focusInEvent( QFocusEvent *event )
{
    setPlainText( m_fullText );
    QGraphicsTextItem::focusInEvent( event );
}

///Editing finished, restore eliding and notify folks of what the new text is
void
Playlist::TextItem::focusOutEvent( QFocusEvent *event )
{
    QGraphicsTextItem::focusOutEvent( event );
    m_fullText = toPlainText();
    setEditableText( m_fullText, m_width );
}
