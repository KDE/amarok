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

#include "shared/amarok_export.h"
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
            void subscribeTo( PlaylistPtr );
            void unsubscribeFrom( PlaylistPtr );

            /** This method is called when a track has been added to the playlist.
             */
            virtual void trackAdded( PlaylistPtr playlist, Meta::TrackPtr track, int position ) = 0;

            /** This method is called after a track is removed from to the playlist. */
            virtual void trackRemoved( PlaylistPtr playlist, int position ) = 0;

            virtual ~PlaylistObserver();

        private:
            QSet<PlaylistPtr> m_playlistSubscriptions;
    };

    class AMAROK_CORE_EXPORT Playlist : public virtual QSharedData
    {
        public:
            virtual ~Playlist() {}
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

            /** @returns the number of tracks this playlist contains. -1 if this can not
              * be determined before loading them all.
              */
            virtual int trackCount() const { return -1; }

            /** returns all tracks in this playlist */
            virtual Meta::TrackList tracks() = 0;

            /** Called to make a playlist load it's tracks in memory.
              * This is used by PlaylistBrowserModel to do on-demand loading.
              * It's recommended that this function starts a background task in order not to block
              * the GUI thread.
              */
            virtual void triggerTrackLoad() {}

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
             * playlists providers.
             */
            virtual void syncTrackStatus( int position, Meta::TrackPtr otherTrack )
                    { Q_UNUSED(position); Q_UNUSED(otherTrack); }

            virtual void subscribe( PlaylistObserver *observer )
                    { if( observer ) m_observers.insert( observer ); }
            virtual void unsubscribe( PlaylistObserver *observer )
                    { m_observers.remove( observer ); }

            /** A list of groups or labels this playlist belongs to.
              *
              * Can be used for grouping in folders (use ex. '/' as separator) or for
              * labels.
              */
            virtual QStringList groups() { return QStringList(); }

            virtual QActionList actions();
            virtual QActionList trackActions( int trackIndex );

            /**
            * "labels" the playlist as part of a group. In a folder-like hierachy this means adding
            * the playlist to the folder with name groups.first().
            * If groups is empty that means removing all groups from the playlist.
            */
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED(groups) }

        protected:
            /**
             * Implementations must call this when a track is added to playlist
             *
             * @param position is the actual new position of the added track, never negative
             */
            inline void notifyObserversTrackAdded( Meta::TrackPtr track, int position )
            {
                Q_ASSERT( position >= 0 ); // notice bug 293295 early
                foreach( Playlists::PlaylistObserver *observer, m_observers )
                {
                    if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
                        observer->trackAdded( Playlists::PlaylistPtr( this ), track, position );
                }
            }

            /**
             * Implementations must call this when a track is added to playlist
             *
             * @param position is the position where the track was before removal
             */
            inline void notifyObserversTrackRemoved( int position )
            {
                foreach( Playlists::PlaylistObserver *observer, m_observers )
                {
                    if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
                        observer->trackRemoved( Playlists::PlaylistPtr( this ), position );
                }
            }

            QSet<Playlists::PlaylistObserver*> m_observers;
    };

}

Q_DECLARE_METATYPE( Playlists::PlaylistPtr )
Q_DECLARE_METATYPE( Playlists::PlaylistList )

#endif
