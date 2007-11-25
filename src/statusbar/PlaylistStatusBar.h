/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *   Copyright (C) 2007 Seb Ruiz <ruiz@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTSTATUSBAR_H
#define AMAROK_PLAYLISTSTATUSBAR_H

#include "StatusBarBase.h"  //baseclass
//TODO:PORT to new playlist
// #include "queueLabel.h"

#include <QLabel>

namespace Amarok
{
    class AMAROK_EXPORT PlaylistStatusBar : public KDE::StatusBar
    {
        Q_OBJECT

        static PlaylistStatusBar* s_instance;

        public:
            explicit PlaylistStatusBar( QWidget *parent, const char *name = 0 );
            static   PlaylistStatusBar* instance() { return s_instance; }

        public slots:
            /** update total song count */
            void slotItemCountChanged( int newCount ); //TODO improve
            //FIXME: PORT
            // void updateQueueLabel() { m_queueLabel->update(); }

        private:
            QLabel *m_itemCountLabel;
            //FIXME: Port
            // QueueLabel *m_queueLabel;
    };
}

namespace The
{
    inline Amarok::PlaylistStatusBar *playlistStatusBar() { return Amarok::PlaylistStatusBar::instance(); }
}

#endif
