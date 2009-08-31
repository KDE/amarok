/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef PLAYLISTLAYOUTCONFIGWIDGET_H
#define PLAYLISTLAYOUTCONFIGWIDGET_H

#include <KAction>
#include <KPushButton>
#include <KMenu>

class QComboBox;

namespace Playlist {

/**
 * Action used to show a menu for selecting active playlist layout. Also contains an entry for showing the
 * playlist layout editor.
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class LayoutConfigAction : public KAction
{
    Q_OBJECT

    public:
        /**
         * Constructor.
         * @param parent Parent widget.
         */
        LayoutConfigAction( QWidget * parent );

        /**
         * Destructor.
         */
        ~LayoutConfigAction();

    protected slots:

        /**
         * Set the currently active layout based on the selected action.
         * @param layoutAction The layoutAction triggered.
         */
        void setActiveLayout( QAction *layoutAction );

        /**
         * Notify the action that the list of selectable layouts have changed. Clears the list of layouts
         * and refetches it from the LayoutManager.
         */
        void layoutListChanged();
        
        /**
         * Launch the playlist layout editor.
         */
        void configureLayouts();
        
        /**
         * The active layout has changed. Update the selected item to represent this.
         */
        void onActiveLayoutChanged();

    private:
        KAction *m_configAction;
        QActionGroup *m_layoutActions;
        KMenu *m_layoutMenu;
};

}

#endif
