/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef USERPLAYLISTPROVIDER_H
#define USERPLAYLISTPROVIDER_H

#include "core/playlists/Playlist.h"
#include "core/playlists/PlaylistProvider.h"

class QAction;

namespace Playlists {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT UserPlaylistProvider : public PlaylistProvider
{
    Q_OBJECT
    public:
        virtual ~UserPlaylistProvider();

        /* PlaylistProvider functions */
        virtual int category() const;

        /* UserPlaylistProvider functions */
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks,
                                             const QString& name = QString() ) = 0;

        virtual QList<QAction *> playlistActions( Playlists::PlaylistPtr playlist );
        virtual QList<QAction *> trackActions( Playlists::PlaylistPtr playlist,
                                                  int trackIndex );

        // UserPlaylistProvider-specific
        virtual bool isWritable() { return false; }
        virtual void rename( Playlists::PlaylistPtr playlist, const QString &newName )
                { Q_UNUSED( playlist ) Q_UNUSED(newName) }
        virtual bool deletePlaylists( Playlists::PlaylistList playlistlist )
                { Q_UNUSED( playlistlist ) return false; }
};

} //namespace Playlists

#endif
