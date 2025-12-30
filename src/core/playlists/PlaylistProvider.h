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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTPROVIDER_H
#define AMAROK_PLAYLISTPROVIDER_H

#include "core/amarokcore_export.h"
#include "core/playlists/Playlist.h"

#include <QObject>

class QIcon;

namespace Playlists {

class AMAROKCORE_EXPORT PlaylistProvider : public QObject
{
    Q_OBJECT

    public:
        explicit PlaylistProvider( QObject *parent = nullptr );

        /**
         * A translated string to identify this Provider.
         */
        virtual QString prettyName() const = 0;

        /**
         * A nice icon for this playlist provider.
         */
        virtual QIcon icon() const = 0;

        /**
         * @returns An unique integer that identifies the category of the offered playlists.
         * Use the PlaylistManager::PlaylistCategory enum.
         */
        virtual int category() const = 0;

        /**
         * @returns the number of playlists this provider has or a negative value if it
         * can not determine that before loading them all.
         *
         * Default implementation returns -1.
         */
        virtual int playlistCount() const { return -1; }

        /**
         * Return a list of playlists of this provider. If playlistCount() is negative,
         * this list may be incomplete.
         */
        virtual PlaylistList playlists() = 0;

        virtual QActionList providerActions();
        virtual QActionList playlistActions( const PlaylistList &playlists );
        virtual QActionList trackActions( const QMultiHash<PlaylistPtr, int> &playlistTracks );

        /**
         * Return true if this providers supports modification made by the user.
         *
         * I.e. whether addPlaylist(), renamePlaylist(), deletePlaylists() make sense
         * to be triggered by user action.
         *
         * Default implementation returns false.
         */
        virtual bool isWritable();

        /**
         * Copy a playlist to the provider.
         */
        virtual PlaylistPtr addPlaylist( PlaylistPtr playlist );

        /**
         * Rename a playlist of this provider.
         */
        virtual void renamePlaylist( PlaylistPtr playlist, const QString &newName );

        /**
         * Deletes a list of playlists. Returns true of successful, false otherwise.
         *
         * Default implementation does nothing and returns false.
         */
        virtual bool deletePlaylists( const PlaylistList &playlistlist );

        /**
         * Copy a track directly to a playlist provider without being in a playlist.
         * It's up to the implementation to decide what to do but could for instance allow the
         * creation of a new playlist from scratch.
         */
        virtual Meta::TrackPtr addTrack( const Meta::TrackPtr &track );

    Q_SIGNALS:
        void updated();
        void playlistAdded( const Playlists::PlaylistPtr &playlist );
        void playlistRemoved( const Playlists::PlaylistPtr &playlist );
};

} //namespace Playlists

Q_DECLARE_METATYPE( Playlists::PlaylistProvider * )

#endif // AMAROK_PLAYLISTPROVIDER_H
