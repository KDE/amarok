/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTWIDGET_H
#define AMAROK_PLAYLISTWIDGET_H

#include <QStackedWidget>
#include <QWidget>

namespace Playlist {

    class Widget : public QWidget
    {
        Q_OBJECT
        public:
            Widget( QWidget* parent );

        public slots:
            void switchView();

        private:
            QStackedWidget *m_stackedWidget;
    };
}

#endif
