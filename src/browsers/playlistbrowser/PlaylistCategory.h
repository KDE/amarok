/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PLAYLISTCATEGORY_H
#define PLAYLISTCATEGORY_H


#include "UserPlaylistTreeView.h"
#include "browsers/BrowserCategory.h"

#include <KDialog>

#include <QModelIndex>
#include <QPoint>

class QToolBar;
class QTreeView;

class KAction;
class KLineEdit;

class PlaylistsInGroupsProxy;

namespace PlaylistBrowserNS {

/**
The widget that displays playlists in the playlist browser

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class PlaylistCategory : public BrowserCategory
{
Q_OBJECT
public:
    PlaylistCategory( QWidget * parent );

    ~PlaylistCategory();

private slots:
    void newPalette( const QPalette & palette );

private:

    QToolBar * m_toolBar;
    UserPlaylistTreeView * m_playlistView;

    KAction * m_addGroupAction;
    PlaylistsInGroupsProxy *m_groupedProxy;

};

}

#endif
