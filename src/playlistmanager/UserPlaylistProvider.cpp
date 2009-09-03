/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "UserPlaylistProvider.h"


UserPlaylistProvider::~UserPlaylistProvider()
{
}

int
UserPlaylistProvider::category() const
{
     return PlaylistManager::UserPlaylist;
}

bool
UserPlaylistProvider::supportsEmptyGroups()
{
    return false;
}

QList<QAction *>
UserPlaylistProvider::playlistActions( Meta::PlaylistPtr playlist )
{
    Q_UNUSED( playlist );
    return QList<QAction *>();
}

QList<QAction *>
UserPlaylistProvider::trackActions( Meta::PlaylistPtr playlist, int trackIndex )
{
    Q_UNUSED( playlist );
    Q_UNUSED( trackIndex );
    return QList<QAction *>();
}

#include "UserPlaylistProvider.moc"
