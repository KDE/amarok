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
#include "core/meta/Meta.h"

#include <QList>
#include <QMetaType>
#include <QPixmap>
#include <QSet>
#include <QSharedData>
#include <QString>
#include <QTextStream>

#include <KSharedPtr>
#include <KUrl>

class QTextStream;

class QAction;
typedef QList<QAction *> QActionList;

namespace Playlists
{
    class Playlist;
    class PlaylistProvider;

    typedef KSharedPtr<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;

    enum PlaylistCategory
    {
        UserPlaylist = 1,
        PodcastChannelPlaylist
    };

    class AMAROK_CORE_EXPORT PlaylistObserver
    {
        public:
            PlaylistObserver();
            virtual ~PlaylistObserver();

            /**
             * Subscribe to changes made by @param playlist. Does nothing if playlist is
             * null or if already subscribed.
             *
             * This method is thread-safe.
             */
            void subscribeTo( PlaylistPtr playlist );

            /**
             * Unsubscribe from changes made by @param playlist. Does nothing if not yet
             * subscribed to playlist.
             *
             * This method is thread-safe.
             */
            void unsubscribeFrom( PlaylistPtr playlist );

            /**
             * This method is called when playlist metadata (such as title) has changed.
             * This isn't called when just a list of tracks changes.
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void metadataChanged( PlaylistPtr playlist ) = 0;

            /**
             * This method is called when a track has been added to the playlist.
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void trackAdded( PlaylistPtr playlist, Meta::TrackPtr track, int position ) = 0;

            /**
             * This method is called after a track is removed from to the playlist.
             *
             * @note this method may get called from non-main thread and must be
             * implemented in a thread-safe manner
             */
            virtual void trackRemoved( PlaylistPtr playlist, int position ) = 0;

        private:
            QSet<PlaylistPtr> m_playlistSubscriptions;
            QMutex m_playlistSubscriptionsMutex; // guards access to m_playlistSubscriptions
    };

    class AMAROK_CORE_EXPORT Playlist : public virtual QSharedData
    {
        public:
            Playlist();
            virtual ~Playlist();

            /**
             * @returns a unique identifier for a playlist. Should be similar to
             * Meta::Track::uidUrl
             */
            virtual KUrl uidUrl() const = 0;

            virtual QString name() const = 0;
            virtual QString prettyName() const { return name(); }
            virtual QString description() const { return QString(); }

            virtual PlaylistProvider *provider() const { return 0; }

            /**override showing just the filename */
            virtual void setName( const QString &name ) { Q_UNUSED( name ); }

            /**
             * @returns the number of tracks this playlist contains. -1 if this can not
             * be determined before loading them all.
             *
             * Default implementation returns -1.
             */
            virtual int trackCount() const { return -1; }

            /**
             * Returns loaded tracks in this playlist. Note that the list may be incomplete,
             * to be sure, you have to become playlist observer, watch for trackAdded()
             * methods and call triggerTrackLoad()
             */
            virtual Meta::TrackList tracks() = 0;

            /**
             * Trigger full background loading of this playlist. Observer's trackAdded()
             * and metadataChanged() will be called as appropriate. This may even change
             * playlist metadata;
             *
             * Implementors, you should start a background job in this method to
             * actually load tracks, calling notifyObservers[Something]Added/Changed()
             * as appropriate. You should also use MetaProxy::Track as a second-level
             * lazy-loading so that you can return more quickly.
             *
             * Default implementation does nothing.
             */
            virtual void triggerTrackLoad();

            /** Add the track to a certain position in the playlist
             *  @arg position: place to add this track. The default value -1 appends to
             *  the end.
             * @note if the position is larger then the size of the playlist append to the
             * end without generating an error.
             */
            virtual void addTrack( Meta::TrackPtr track, int position = -1 )
                    { Q_UNUSED(track); Q_UNUSED(position); }
            /** Remove track at the specified position */
            virtual void removeTrack( int position ) { Q_UNUSED(position); }

            /** Sync track status between two tracks. This is only
             * useful for podcasts providers and some other exotic
             * playlist providers.
             */
            virtual void syncTrackStatus( int position, Meta::TrackPtr otherTrack )
                    { Q_UNUSED(position); Q_UNUSED(otherTrack); }

            /**
             * Return user-actionable actions for this playlist. Default implementation
             * just returns provider actions for this playlist.
             */
            virtual QActionList actions();

            /**
             * Return actions for track at position @trackIndex for this playlist. Default
             * implementation returns provider()'s trackActions().
             */
            virtual QActionList trackActions( int trackIndex );

            /**
             * A list of groups or labels this playlist belongs to.
             *
             * Can be used for grouping in folders (use ex. '/' as separator) or for
             * labels.
             */
            virtual QStringList groups() { return QStringList(); }

            /**
             * Labels the playlist as part of a group.
             *
             * In a folder-like hierarchy this means adding the playlist to the folder with
             * name groups.first(). If groups is empty that means removing all groups from
             * the playlist.
             */
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED(groups) }

        protected:
            /**
             * Implementations must call this when metadata such as title has changed. Do
             * not call this when just a list of track changes.
             *
             * @param position is the actual new position of the added track, never negative
             * @note calling this from (code called by) Playlist constructor is FORBIDDEN.
             *
             * TODO: find all occurrences where this should be called in Playlist subclasses
             * and add the call!
             */
            void notifyObserversMetadataChanged();

            /**
             * Implementations must call this when a track is added to playlist
             *
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
            QReadWriteLock m_observersLock; // guards access to m_observers
    };
}

Q_DECLARE_METATYPE( Playlists::PlaylistPtr )
Q_DECLARE_METATYPE( Playlists::PlaylistList )

#endif
