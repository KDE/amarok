/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTTEXTITEM_H
#define AMAROK_PLAYLISTTEXTITEM_H

#include <QGraphicsTextItem>

class QFontMetricsF;

namespace Playlist
{
    class TextItem : public QGraphicsTextItem 
    {
        Q_OBJECT
        public:
            TextItem( QGraphicsItem* parent );
            void setEditableText( const QString& text, qreal width );
        protected:
            void focusInEvent(QFocusEvent *event);
            void focusOutEvent(QFocusEvent *event);
        private:
            QString m_fullText;
            int m_width;
            static QFontMetricsF* s_fm;
    };
}

#endif
