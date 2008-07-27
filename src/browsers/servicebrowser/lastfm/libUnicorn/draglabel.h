/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef DRAGLABEL_H
#define DRAGLABEL_H

#include "UnicornDllExportMacro.h"

#include <QHash>
#include <QLabel>
#include <QUrl>


class UNICORN_DLLEXPORT DragLabel : public QLabel
{
    Q_OBJECT

    public:
        DragLabel( QWidget *parent = 0 );

        QStringList items();

        virtual int heightForWidth( int w ) const;
        virtual QSize sizeHint() const;
        virtual QSize minimumSizeHint() const;

        void setCommaSeparated( bool on ) { m_commas = on; }
        bool commaSeparated() { return m_commas; }

        void setJustified( bool on ) { m_justified = on; }
        bool justified() { return m_justified; }

    public slots:
        void append( const QString& text );
        void clear();
        void clearText();
        void clearSelections();

        void setBackground( const QPixmap& pixmap );
        void setHeader( const QString& header, const QFont& font = QFont() );
        void setText( const QString& text );
        QString text() const;
        void setItems( const QStringList& list );
        void setItemText( int index, const QString& text );
        void setItemTooltip( int index, const QString& text );
        void setFont( const QFont& font );
        void setItemFont( int index, QFont font );
        void setItemColor( int index, QColor color );
        void setItemURL( int index, QString url );
        void setURL( const QUrl& url );

        QHash<QString, QString> itemData( int index ) const;
        void setItemData( int index, QHash<QString, QString> data );

        int itemType() const;
        void setItemType( int type );

        void setItemsSelectable( bool selectable );
        void setItemSelected( const QString& text, bool selected, bool emitSignal = true );
        void setItemSelected( int index, bool selected, bool emitSignal = true );

        void setUniformLineHeight( int height ) { m_uniformLineHeight = height; }
        void setHoverable( bool state ) { m_hoverable = state; }

    signals:
        void clicked( int index );
        void urlHovered( const QString& url );

    protected:
        virtual bool event( QEvent* event );

        void paintEvent( QPaintEvent* event );

        void leaveEvent( QEvent* event );

        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );

    private:

        class DragItem
        {
            public:
                DragItem() : m_ascent( -1 ), m_selected( false ) { }

                QString m_text;
                QString m_tooltip;
                QUrl    m_url;
                QFont   m_font;
                QColor  m_colour;
                QRect   m_extent;
                int     m_ascent;
                bool    m_selected;
                QHash<QString, QString> m_dragData;
        };

        // Updates extent and ascent of an item
        void calcFontProperties( DragItem& d, bool isHeader = false );

        // Refreshes display
        void updateDragLabel();

        // Recalculates item layout based on passed-in width. Returns size
        // needed including margins.
        QSize layoutItems( QList<QRect>& layoutOut, int width ) const;

        void baseAlign( QList<QRect>& layoutOut, int startIdx, int endIdx, int lineHeight ) const;
        void justify( QList<QRect>& layoutOut, int startIdx, int endIdx, int width ) const;

        bool anythingToDraw();
        int roundnessForLength( int len );

        // Returns the index of the item at coord pos, if any. -1 for none.
        int itemAt( const QPoint& pos );

        QString m_header;
        QList<DragItem> m_items;
        int m_itemsStartAt; // index into m_items where first real item is (0 or 1)

        mutable QList<QRect> m_hfwLayout;  // last calculated height for width layout
        mutable QList<QRect> m_lineLayout; // last calculated single line layout

        mutable int m_lastWidth;

        mutable QSize m_lastHfwSize;
        mutable QSize m_sizeHint;

        int m_itemType;

        QRectF m_answerRect;
        QPoint m_hoverPoint;
        int m_hoverIndex;

        bool m_hoverable;
        bool m_selectable;
        bool m_commas;
        bool m_justified;
        int m_uniformLineHeight;

        QPoint m_dragStartPosition;

        QPixmap m_background;
};

#endif
