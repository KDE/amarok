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

#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/playlists/PlaylistProvider.h"

#include <kicon.h>

class KConfigGroup;
class KUrl;

class QAction;

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
        virtual KIcon icon() const { return KIcon( "folder-documents" ); }

        virtual int category() const { return Playlists::UserPlaylist; }

        virtual int playlistCount() const;
        virtual Playlists::PlaylistList playlists();

        virtual QList<QAction *> playlistActions( Playlists::PlaylistPtr playlist );
        virtual QList<QAction *> trackActions( Playlists::PlaylistPtr playlist,
                                                  int trackIndex );

        virtual bool canSavePlaylists() { return true; }

        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks );
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks,
                                        const QString &name );

        virtual bool import( const KUrl &path );

        virtual bool isWritable() { return true; }
        virtual void rename( Playlists::PlaylistPtr playlist, const QString &newName );
        virtual void deletePlaylists( Playlists::PlaylistList playlistList );

        virtual void loadPlaylists();

    signals:
            void updated();

    private slots:
            void slotRemove();

    private:
        KConfigGroup loadedPlaylistsConfig() const;

        bool m_playlistsLoaded;
        Playlists::PlaylistList m_playlists;
        Playlists::PlaylistFormat m_defaultFormat;
        QMultiMap<QString, Playlists::PlaylistPtr> m_groupMap;

        QAction *m_removeTrackAction;
};

} //namespace Playlists

#endif
