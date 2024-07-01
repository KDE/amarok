/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef USERPLAYLISTCATEGORY_H
#define USERPLAYLISTCATEGORY_H

#include "PlaylistBrowserCategory.h"

namespace PlaylistBrowserNS {

/**
The widget that displays playlists in the playlist browser

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class UserPlaylistCategory : public PlaylistBrowserCategory
{
Q_OBJECT
public:
    static QString s_configGroup;

    explicit UserPlaylistCategory( QWidget *parent );

    ~UserPlaylistCategory() override;
};

}

#endif //USERPLAYLISTCATEGORY_H
