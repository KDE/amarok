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

#ifndef AMAROK_META_PLAYLIST_H
#define AMAROK_META_PLAYLIST_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"

#include <QList>
#include <QMetaType>
#include <QRecursiveMutex>
#include <QPixmap>
#include <QSet>
#include <QSharedData>
#include <QString>
#include <QTextStream>

#include "AmarokSharedPointer.h"
#include <QUrl>

class QTextStream;

class QAction;
typedef QList<QAction *> QActionList;

namespace Playlists
{
    class Playlist;
    class PlaylistProvider;

    typedef AmarokSharedPointer<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;

    enum PlaylistCategory
    {
        UserPlaylist = 1,
        PodcastChannelPlaylist
    };

    /**
     * Subclass this class in order to be able to watch playlists as their metadata and
     * track list changes.
     */
    class AMAROKCORE_EXPORT PlaylistObserver
    {
        public:
            PlaylistObserver();
            virtual ~PlaylistObserver();

            /**
             * Subscribe to changes made by @p playlist. Does nothing if playlist is
             * null or if already subscribed.
             *
             * @param playlist the playlist
             *
             * This method is thread-safe.
             */
            void subscribeTo( PlaylistPtr playlist );

            /**
             * Unsubscribe from changes made by @p playlist. Does nothing if not yet
             * subscribed to playlist.
             *
             * @param playlist the playlist
             *
             * This method is thread-safe.
             */
            void unsubscribeFrom( PlaylistPtr playlist );

            /**
             * This method is called when playlist metadata (such as title) has changed.
             * This isn't called when just a list of tracks changes.
             *
             * @param playlist playlist whose metadata were changed
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void metadataChanged( const PlaylistPtr &playlist );

            /**
             * This method is called when a track has been added to the playlist.
             *
             * @param playlist playlist whose track list was changed
             * @param track track that was added
             * @param position position where the track was inserted to, beginning from 0
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void trackAdded( const PlaylistPtr &playlist, const Meta::TrackPtr &track, int position );

            /**
             * This method is called after a track is removed from to the playlist.
             *
             * @param playlist playlist whose track list was changed
             * @param position position occupied by the track right before it was removed
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void trackRemoved( const PlaylistPtr &playlist, int position );

            /**
             * This method is called after loading of playlist is finished
             * (which was started by triggerTrackLoad()) and all tracks are already added.
             *
             * @param playlist playlist loading of which has finished
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void tracksLoaded( PlaylistPtr playlist );

        private:
            QSet<PlaylistPtr> m_playlistSubscriptions;
            QRecursiveMutex m_playlistSubscriptionsMutex; // guards access to m_playlistSubscriptions
    };

    class AMAROKCORE_EXPORT Playlist : public virtual QSharedData
    {
        public:
            Playlist();
            virtual ~Playlist();

            /**
             * @returns a unique identifier for a playlist. Should be similar to
             * Meta::Track::uidUrl
             */
            virtual QUrl uidUrl() const = 0;

            virtual QString name() const = 0;
            virtual QString prettyName() const { return name(); }

            virtual PlaylistProvider *provider() const { return nullptr; }

            virtual void setName( const QString &name );

            /**
             * Returns the number of tracks this playlist contains. -1 if tracks are not
             * yet loaded (call triggerTrackLoad() in this case). If you get non-negative
             * number, all tracks have been already loaded.
             */
            virtual int trackCount() const = 0;

            /**
             * Returns loaded tracks in this playlist. Note that the list may be incomplete,
             * to be sure, check that trackCount() is non-negative. Otherwise you have to
             * become playlist observer, watch for trackAdded() methods and call
             * triggerTrackLoad(). If you want to immediately play or
             * extract metadata of the tracks, be aware that many playlist implementations
             * initially return MetaProxy::Tracks that are resolved asynchronously.
             *
             * Convenient way to overcome the first and optionally the second
             * inconvenience is to use TrackLoader helper class.
             */
            virtual Meta::TrackList tracks() = 0;

            /**
             * Trigger full background loading of this playlist. Observer's trackAdded()
             * and metadataChanged() will be called as appropriate. This may even change
             * playlist metadata;
             *
             * Implementors, you should start a background job in this method to
             * actually load tracks, calling notifyObservers[Something]Added/Changed()
             * as appropriate.
             * It is guaranteed that tracksLoaded() observer method will be called
             * exactly once, either sooner (before returning from this method) or
             * later (asynchronously perhaps from a different thread).
             *
             * Implementors should also use MetaProxy::Track as a second-level
             * lazy-loading.
             *
             * Default implementation just calls notifyObserversTracksLoaded().
             */
            virtual void triggerTrackLoad();

            /**
             * Add the track to a certain position in the playlist
             *
             * @param track the track to add
             * @param position place to add this track. The default value -1 appends to
             *                 the end.
             *
             * @note if the position is larger then the size of the playlist append to the
             * end without generating an error.
             */
            virtual void addTrack( const Meta::TrackPtr &track, int position = -1 );

            /**
             * Remove track at the specified position
             */
            virtual void removeTrack( int position );

            /**
             * Sync track status between two tracks. This is only
             * useful for podcasts providers and some other exotic
             * playlist providers.
             */
            virtual void syncTrackStatus( int position, const Meta::TrackPtr &otherTrack );

            /**
             * A list of groups or labels this playlist belongs to.
             *
             * Can be used for grouping in folders (use ex. '/' as separator) or for
             * labels. Default implementation returns empty list.
             */
            virtual QStringList groups();

            /**
             * Labels the playlist as part of a group.
             *
             * In a folder-like hierarchy this means adding the playlist to the folder with
             * name groups.first(). If groups is empty that means removing all groups from
             * the playlist. Default implementation does nothing.
             */
            virtual void setGroups( const QStringList &groups );

            // FIXME: two methods below are a temporary solution
            // and should be removed after support of async loading will
            // added everywhere
            /**
             * Call this method to assure synchronously loading.
             * @note not all playlist implementations support asynchronous loading
             */
            QT_DEPRECATED void makeLoadingSync() { m_async = false; }
            /**
             * Allows to check if asynchronously loading is deactivated
             */
            bool isLoadingAsync() const { return m_async; }

        protected:
            /**
             * Implementations must call this when metadata such as title has changed. Do
             * not call this when just a list of track changes.
             *
             * @note calling this from (code called by) Playlist constructor is FORBIDDEN.
             *
             * TODO: find all occurrences where this should be called in Playlist subclasses
             * and add the call!
             */
            void notifyObserversMetadataChanged();

            /**
             * Implementations must call this when playlist loading started
             * by trriggerTrackLoad() is finished and all tracks are added.
             *
             * @note calling this from (code called by) Playlist constructor is FORBIDDEN.
             */
            void notifyObserversTracksLoaded();

            /**
             * Implementations must call this when a track is added to playlist
             *
             * @param track the track that was added
             * @param position is the actual new position of the added track, never negative
             * @note calling this from (code called by) Playlist constructor is FORBIDDEN.
             */
            void notifyObserversTrackAdded( const Meta::TrackPtr &track, int position );

            /**
             * Implementations must call this when a track is added to playlist
             *
             * @param position is the position where the track was before removal
             * @note calling this from (code called by) Playlist constructor is FORBIDDEN.
             */
            void notifyObserversTrackRemoved( int position );

        private:
            friend class PlaylistObserver; // so that it can call (un)subscribe()
            void subscribe( PlaylistObserver *observer );
            void unsubscribe( PlaylistObserver *observer );

            QSet<PlaylistObserver *> m_observers;
            /**
             * Guards access to m_observers. It would seem that QReadWriteLock would be
             * more efficient, but when it is locked for read, it cannot be relocked for
             * write, even if it is recursive. This can cause deadlocks, so it would be
             * never safe to lock it just for read.
             */
            QRecursiveMutex m_observersMutex;
            bool m_async;
    };
}

Q_DECLARE_METATYPE( Playlists::PlaylistPtr )
Q_DECLARE_METATYPE( Playlists::PlaylistList )

#endif
