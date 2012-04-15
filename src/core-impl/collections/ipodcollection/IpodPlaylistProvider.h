/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef IPODPLAYLISTPROVIDER_H
#define IPODPLAYLISTPROVIDER_H

#include "IpodPlaylist.h"

#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"


class IpodCollection;
struct _Itdb_iTunesDB;
typedef _Itdb_iTunesDB Itdb_iTunesDB;

class IpodPlaylistProvider : public Playlists::UserPlaylistProvider, private Playlists::PlaylistObserver
{
    Q_OBJECT

    public:
        IpodPlaylistProvider( IpodCollection *collection );
        virtual ~IpodPlaylistProvider();

        // PlaylistProvider methods:
        virtual QString prettyName() const;
        virtual KIcon icon() const;

        virtual int playlistCount() const;
        virtual Playlists::PlaylistList playlists();

        virtual Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );
        virtual Meta::TrackPtr addTrack( Meta::TrackPtr track );

        // UserPlaylistProvider methods:
        virtual Playlists::PlaylistPtr save( const Meta::TrackList &tracks,
                                             const QString& name = QString() );

        virtual QActionList providerActions();
        virtual QActionList playlistActions( Playlists::PlaylistPtr playlist );
        virtual QActionList trackActions( Playlists::PlaylistPtr playlist,
                                               int trackIndex );

        virtual bool isWritable();
        virtual void rename( Playlists::PlaylistPtr playlist, const QString &newName );
        virtual bool deletePlaylists( Playlists::PlaylistList playlistlist );

        // PlaylistObserver methods:
        virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track, int position );
        virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position );

        // IpodPlaylistProvider specific methods:

        /**
         * Copy tracks stored in playlists tracksToCopy() to iPod and them add them to the
         * playlist. The actual call to start copying tracks is deferred to next eventloop
         * iteration to pickup multiple successive addTrack() calls.
         */
        void scheduleCopyAndInsertToPlaylist( KSharedPtr<IpodPlaylist> playlist );

        /**
         * Remove this track from all playlists it belongs to, it was removed from the
         * database. The @param track is the MemoryMeta proxy track.
         */
        void removeTrackFromPlaylists( Meta::TrackPtr track );

        /**
         * Go through iPod playlists and create Amarok playlists for them. Designed to be
         * run from non-gui thread from IpodParseTracksJob.
         *
         * @param staleTracks list of track from iTunes database whose associated file no longer exists
         * @param knownPaths a set of absolute local paths of all track from iTunes database.
         *                   used for orphaned tracks detection.
         */
        void parseItdbPlaylists( const Meta::TrackList &staleTracks, const QSet<QString> &knownPaths );

    signals:
        /**
         * Signals to IpodCollection that the database has been dirtied and it has to
         * write the database in some point in time.
         */
        void startWriteDatabaseTimer();

    private slots:
        void slotCopyAndInsertToPlaylists();
        void slotConsolidateStaleOrphaned();

    private:
        void copyAndInsertToPlaylist( const TrackPositionList &tracks, Playlists::PlaylistPtr destPlaylist );
        Meta::TrackList findOrphanedTracks( const QSet<QString> &knownPaths );
        bool orphanedAndStaleTracksMatch( const Meta::TrackPtr &orphaned, const Meta::TrackPtr &stale );
        template <class T> bool entitiesDiffer( T first, T second );

        IpodCollection *m_coll;
        Playlists::PlaylistList m_playlists;
        QSet< KSharedPtr<IpodPlaylist> > m_copyTracksTo;
        Playlists::PlaylistPtr m_stalePlaylist;
        Playlists::PlaylistPtr m_orphanedPlaylist;
        QAction *m_consolidateAction;
};

#endif // IPODPLAYLISTPROVIDER_H
