/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef USERPLAYLISTPROVIDER_H
#define USERPLAYLISTPROVIDER_H

#include <PlaylistManager.h>

class KUrl;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class UserPlaylistProvider : public PlaylistProvider
{
    public:
        UserPlaylistProvider();

        ~UserPlaylistProvider();

        QString prettyName() const;
        int category() const { return PlaylistManager::UserPlaylist; };
        QString typeName() const;

        Meta::PlaylistList playlists();

    private:
//         m_acceptedMimetypes;
        Meta::PlaylistList m_playlists;
};

#endif
