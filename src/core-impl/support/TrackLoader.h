/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TRACKLOADER_H
#define AMAROK_TRACKLOADER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"
#include "core/meta/Observer.h"
#include "core/playlists/Playlist.h"

namespace KIO {
    class Job;
    class UDSEntry;
    typedef QList<UDSEntry> UDSEntryList;
}

/**
 * Helper class that helps with loading of urls (with local and remote tracks,
 * playlists and local directories) to tracks.
 *
 * Only explicitly listed playlists are loaded, not the ones found in subdirectories.
 * TrackLoader takes care to preserve order of urls you pass, and it sorts tracks in
 * directories you pass it using directory- and locale-aware sort.
 */
class AMAROK_EXPORT TrackLoader : public QObject, public Playlists::PlaylistObserver, public Meta::Observer
{
    Q_OBJECT

    public:
        /**
         * FullMetadataRequired: signal TrackLoader that it should postpone the finished()
         * signal until the any possible proxy tracks have resolved and their full
         * metadata is available. Also use this flag when you need to immediately play
         * the tracks. This no longer implies any blocking behaviour, you'll just get the
         * finished signal a bit later.
         *
         * RemotePlaylistsAreStreams: treat playlists with remote urls as Streams with
         * multiple alternative download locations (Meta::MultiTracks). Works even when
         * you pass playlists.
         */
        enum Flag {
            FullMetadataRequired = 1 << 0,
            RemotePlaylistsAreStreams = 1 << 1,
        };
        Q_DECLARE_FLAGS( Flags, Flag )

        /**
         * Construct TrackLoader. You must construct it on the heap, it will auto-delete
         * itself.
         *
         * @param flags binary or of flags, see TrackLoader::Flags enum
         * @param timeout if FullMetadataRequired is in flags, this is the timeout in
         * milliseconds for waiting on track to resolve. Ignored otherwise.
         */
        explicit TrackLoader( Flags flags = 0, int timeout = 2000 );
        ~TrackLoader();

        /**
         * Convenience overload for init( const QList<QUrl> &urls )
         */
        void init( const QUrl &url );

        /**
         * Starts TrackLoader's job, you'll get finished() signal in the end and
         * TrackLoader will auto-delete itself.
         *
         * @param urls list of urls to load tracks from, you can pass local and remote urls
         * pointing to directories, tracks and playlists.
         */
        void init( const QList<QUrl> &urls );

        /**
         * Short-hand if you already have a list of playlists and want a convenient way
         * to get notified of their loaded tracks. See init( const QList<QUrl> ) and
         * class description.
         */
        void init( const Playlists::PlaylistList &playlists );

        /* PlaylistObserver methods */
        using PlaylistObserver::metadataChanged;
        void tracksLoaded( Playlists::PlaylistPtr playlist ) override;

        /* Meta::Observer methods */
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track ) override;

    Q_SIGNALS:
        void finished( const Meta::TrackList &tracks );

    private Q_SLOTS:
        void processNextSourceUrl();
        void directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list );
        void processNextResultUrl();
        /**
         * Emits the result and auto-destroys the TrackLoader
         */
        void finish();

    private:
        enum Status {
            LoadingTracks,
            MayFinish,
            Finished
        };
        void mayFinish();

        static bool directorySensitiveLessThan( const QUrl &left, const QUrl &right );

        Status m_status;
        const Flags m_flags;
        int m_timeout;
        /// passed urls, may contain urls of directories
        QList<QUrl> m_sourceUrls;
        /// contains just urls of tracks and playlists
        QList<QUrl> m_resultUrls;
        /// a list of playlists directly passed, same semantics as m_resultUrls
        Playlists::PlaylistList m_resultPlaylists;
        /// the tracks found
        Meta::TrackList m_tracks;
        /// set of unresolved MetaProxy::Tracks that we wait for
        QSet<Meta::TrackPtr> m_unresolvedTracks;
        QMutex m_unresolvedTracksMutex;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( TrackLoader::Flags )

#endif // AMAROK_TRACKLOADER_H
