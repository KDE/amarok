/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

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

#include "PlaylistTextItem.moc"
