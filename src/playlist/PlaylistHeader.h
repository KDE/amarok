/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTHEADER_H
#define AMAROK_PLAYLISTHEADER_H

#include <QMap>
#include <QStringList>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QLabel;

namespace Playlist
{
    class HeaderWidget : public QWidget
    {
        Q_OBJECT
        static const QString HeaderMimeType;
        public:
            HeaderWidget( QWidget* parent );
        protected:
            void enterEvent( QEvent* event );
            void leaveEvent( QEvent* event );
            void dragEnterEvent(QDragEnterEvent *event);
            void dropEvent( QDropEvent *event);
            void mousePressEvent(QMouseEvent *event);

        private:
            QHBoxLayout* m_topLayout;
        
            QList<QVBoxLayout*> m_verticalLayouts;
            QList<QLabel*> m_labels;
            QMap<QString, QLabel*> m_textToLabel;
            QStringList m_test;
    };
}
#endif
