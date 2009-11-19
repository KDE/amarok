/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef NAVIGATORCONFIGACTION_H
#define NAVIGATORCONFIGACTION_H

#include <KAction>


class NavigatorConfigAction : public KAction
{
    Q_OBJECT
public:
    
    /**
    * Constructor.
    * @param parent Parent widget.
    */
    NavigatorConfigAction( QWidget * parent );

    /**
    * Destructor.
    */
    ~NavigatorConfigAction();

protected slots:

    /**
    * Set the currently active navigator based on the selected action.
    * @param layoutAction The action triggered.
    */
    void setActiveNavigator( QAction *navigatorAction );

    void setFavored( QAction *favorAction );

private:

    QAction * m_standardNavigatorAction;
    QAction * m_repeatTrackNavigatorAction;
    QAction * m_repeatAlbumNavigatorAction;
    QAction * m_repeatPlaylistNavigatorAction;
    QAction * m_randomTrackNavigatorAction;
    QAction * m_randomAlbumNavigatorAction;

    QAction * m_favorNoneAction;
    QAction * m_favorScoresAction;
    QAction * m_favorRatingsAction;
    QAction * m_favorLastPlayedAction;
};

#endif // NAVIGATORCONFIGACTION_H
