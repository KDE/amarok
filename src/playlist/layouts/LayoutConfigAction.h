/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2009  Teo Mrnjavac <teo.mrnjavac@gmail.com>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef PLAYLISTLAYOUTCONFIGWIDGET_H
#define PLAYLISTLAYOUTCONFIGWIDGET_H

#include <KAction>
#include <KPushButton>
#include <KMenu>

class QComboBox;

namespace Playlist {

/**
 * A widget containing the gui needed to define playlist layouts
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

class LayoutConfigAction : public KAction
{
    Q_OBJECT

    public:
        LayoutConfigAction( QWidget * parent );
        ~LayoutConfigAction();

    private slots:
        void setActiveLayout( QAction *layoutAction );
        void layoutListChanged();
        void configureLayouts();
        void onActiveLayoutChanged();

    private:
        KAction *m_configAction;
        QActionGroup *m_layoutActions;
        KMenu *m_layoutMenu;
};

}

#endif
