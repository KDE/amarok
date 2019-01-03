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

#ifndef IPODPLAYLIST_H
#define IPODPLAYLIST_H

#include "core/playlists/Playlist.h"

#include <QPointer>
#include <QReadWriteLock>


class IpodCollection;
struct _Itdb_Playlist;
typedef struct _Itdb_Playlist Itdb_Playlist;

// we cannot use QMap<Track, int> because it doesn't preserve order
typedef QPair<Meta::TrackPtr, int> TrackPosition;
typedef QList<TrackPosition> TrackPositionList;

/**
 * Represents playlist on the iPod. Takes ownership of the m_playlist pointer.
 */
class IpodPlaylist : public Playlists::Playlist
{
    public:
        enum Type {
            Normal,  // regular iPod playlist
            Stale,  // playlist containing stale iTunes database entries
            Orphaned,  // playlist containing track on iPod filesystem that are not it database
        };

        /**
         * Create Amarok iPod playlist out of existing itdb playlist
         */
        IpodPlaylist( Itdb_Playlist *ipodPlaylist, IpodCollection *collection );

        /**
         * Create new Amarok iPod playlist. Some @p tracks may not be in corresponding
         * iPod collection, these are copied to iPod (unless not matched by meta tags)
         *
         * @param tracks the tracks
         * @param name the playlist name
         * @param collection iPod collection
         * @param type whether this playlist is an ordinatory one or a kind of special
         */
        IpodPlaylist( const Meta::TrackList &tracks, const QString &name,
                      IpodCollection *collection, Type type = Normal );

        virtual ~IpodPlaylist();

        QUrl uidUrl() const override;
        QString name() const override;
        void setName( const QString &name ) override;

        Playlists::PlaylistProvider *provider() const override;

        int trackCount() const override;
        Meta::TrackList tracks() override;
        void addTrack( Meta::TrackPtr track, int position = -1 ) override;
        void removeTrack( int position ) override;

        // IpodPlaylist specific:
        Itdb_Playlist *itdbPlaylist();

        /**
         * Return tracks that should be copied to iPod and then added to this playlist,
         * clearing this map.
         */
        TrackPositionList takeTracksToCopy();

        /**
         * Return type of this playlist. Can be used to determine whether this one is
         * special or not.
         */
        Type type() const { return m_type; }

    private:
        Q_DISABLE_COPY( IpodPlaylist )

        /**
         * Copy m_tracksToCopy to iPod and them add them to this playlist. The actual call
         * to start copying tracks is deferred to next eventloop iteration to pickup
         * multiple successive addTrack() calls
         */
        void scheduleCopyAndInsert();

        /**
         * Does the dirty job of adding @p track to this playlist, both to m_tracks
         * and to underlying libgpoid playlist. @p position must be a valid position
         * otherwise this method asserts out.
         * @param track the track
         * @param position the position
         */
        void addIpodTrack( Meta::TrackPtr track, int position );

        Itdb_Playlist *m_playlist;
        mutable QReadWriteLock m_playlistLock;
        QPointer<IpodCollection> m_coll;
        Type m_type;
        Meta::TrackList m_tracks; // playlists tracks, in fact MemoryMeta::Track objects
        TrackPositionList m_tracksToCopy;
};

#endif // IPODPLAYLIST_H
