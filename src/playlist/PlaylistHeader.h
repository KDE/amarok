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

class QVBoxLayout;
class QLabel;

namespace PlaylistNS
{
    class HeaderWidget : public QWidget
    {
        Q_OBJECT
        static const QString HeaderMimeType;
        public:
            HeaderWidget( QWidget* parent );
        protected:
            void mousePressEvent(QMouseEvent *event);
            void dragEnterEvent(QDragEnterEvent *event);
            void dropEvent( QDropEvent *event);
        private:
            QList<QVBoxLayout*> m_verticalLayouts;
            QList<QLabel*> m_labels;
            QMap<QString, QLabel*> m_textToLabel;
            QStringList m_test;
    };
}
#endif
