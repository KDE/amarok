/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef USERPLAYLISTPROVIDER_H
#define USERPLAYLISTPROVIDER_H

#include "meta/Playlist.h"
#include "playlistmanager/PlaylistManager.h"

class QAction;

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
        /**
            @returns true if this provider supports - and is currently able to - save playlists
        **/
        virtual bool canSavePlaylists() = 0;

        /**
            Save a list of tracks as a playlist in the database.
            @returns a non-null Meta::PlaylistPtr if successful
        **/
        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks ) = 0;

        virtual Meta::PlaylistPtr save( const Meta::TrackList &tracks, const QString& name ) = 0;

        virtual bool supportsEmptyGroups();

        virtual QList<QAction *> playlistActions( Meta::PlaylistPtr playlist );
        virtual QList<QAction *> trackActions( Meta::PlaylistPtr playlist,
                                                  int trackIndex );

        // UserPlaylistProvider-specific

        virtual bool isWritable() { return false; }
        virtual void rename( Meta::PlaylistPtr playlist, const QString &newName ) {Q_UNUSED( playlist ) Q_UNUSED(newName)}
        virtual void deletePlaylists( Meta::PlaylistList playlistlist ) { Q_UNUSED( playlistlist ) }

    signals:
            virtual void updated();
};

#endif
