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

#ifndef PLAYLISTFILEPROVIDER_H
#define PLAYLISTFILEPROVIDER_H

#include "core/playlists/PlaylistFormat.h"
#include "core/playlists/PlaylistProvider.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

class QTimer;

namespace Playlists {
/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PlaylistFileProvider : public Playlists::UserPlaylistProvider
{
    Q_OBJECT

    public:
        PlaylistFileProvider();
        virtual ~PlaylistFileProvider();

        virtual QString prettyName() const;
        virtual QIcon icon() const;

        virtual int category() const { return Playlists::UserPlaylist; }

        virtual int playlistCount() const;
        virtual Playlists::PlaylistList playlists();

        /**
        * Returns a Playlists::PlaylistPtr to the new playlist, NULL if something failed.
        * @param tracks Tracks being added to that new playlist.
        * @param name File name of the new playlist. If no extension is being given we
        *             default to xspf. '/' and '\' are being replaced by '-'.
        */
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks,
                                             const QString &name = QString() );

        virtual bool import( const QUrl &path );

        virtual bool isWritable() { return true; }
        virtual void renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName );
        virtual bool deletePlaylists( const Playlists::PlaylistList &playlists );

        /* PlaylistFileProvider methods */
        /** Schedules a PlaylistFile to be saved on the next iteration of the mainloop.
          * Each playlist will be scheduled and saved only once.
          */
        void saveLater( Playlists::PlaylistFilePtr playlist );

    private slots:
        void loadPlaylists();
        void slotSaveLater();

    private:
        bool deletePlaylistFiles( Playlists::PlaylistFileList playlistFiles );
        KConfigGroup loadedPlaylistsConfig() const;

        bool m_playlistsLoaded;
        QList<QUrl> m_urlsToLoad;
        Playlists::PlaylistFileList m_playlists;
        QMultiMap<QString, Playlists::PlaylistPtr> m_groupMap;

        QTimer *m_saveLaterTimer;
        QList<Playlists::PlaylistFilePtr> m_saveLaterPlaylists;
};

} //namespace Playlists

#endif
