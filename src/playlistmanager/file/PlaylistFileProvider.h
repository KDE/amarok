/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef PLAYLISTFILEPROVIDER_H
#define PLAYLISTFILEPROVIDER_H

#include <UserPlaylistProvider.h>
#include <PlaylistFileSupport.h>

class KUrl;

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PlaylistFileProvider : public UserPlaylistProvider
{
    public:
        PlaylistFileProvider();
        ~PlaylistFileProvider();

        QString prettyName() const;
        int category() const { return PlaylistManager::UserPlaylist; };

        virtual Meta::PlaylistList playlists();

        virtual bool canSavePlaylists() { return true; }

        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks,
                                        const QString &name );

    private:
        Meta::PlaylistList m_playlists;
        Meta::PlaylistFormat m_defaultFormat;
};

#endif
