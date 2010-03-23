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

#include "amarok_export.h"
#include "meta/Meta.h"
#include "capabilities/Capability.h"

#include <QList>
#include <QMetaType>
#include <QPixmap>
#include <QSet>
#include <QSharedData>
#include <QString>
#include <QTextStream>

#include <ksharedptr.h>
#include <kurl.h>

class PlaylistProvider;
class QTextStream;

namespace Meta
{
    class Playlist;

    typedef KSharedPtr<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;

    enum PlaylistCategory
    {
        UserPlaylist = 1,
        PodcastChannelPlaylist
    };

    class AMAROK_EXPORT PlaylistObserver
    {
        public:
            void subscribeTo( PlaylistPtr );
            void unsubscribeFrom( PlaylistPtr );

            /** This method is called when a track has been added to the playlist.
             */
            virtual void trackAdded( PlaylistPtr playlist, TrackPtr track, int position ) = 0;
            /** This method is called when a track is removed from to the playlist.
             */
            virtual void trackRemoved( PlaylistPtr playlist, int position ) = 0;

            virtual ~PlaylistObserver();

        private:
            QSet<PlaylistPtr> m_playlistSubscriptions;
    };

    class AMAROK_EXPORT Playlist : public virtual QSharedData
    {
        public:
            virtual ~Playlist() {}
            virtual QString name() const = 0;
            virtual QString prettyName() const = 0;
            virtual QString description() const { return QString(); }

            virtual PlaylistProvider *provider() const { return 0; }

            /**override showing just the filename */
            virtual void setName( const QString &name ) { m_name = name; }

            /** @returns the number of tracks this playlist contains. -1 if this can not
              * be determined before loading them all.
              */
            virtual int trackCount() const { return -1; }
            /** returns all tracks in this playlist */
            virtual TrackList tracks() = 0;
            virtual void addTrack( Meta::TrackPtr track, int position = -1 )
                    { Q_UNUSED(track); Q_UNUSED(position); }
            virtual void removeTrack( int position ) { Q_UNUSED(position); }

            virtual void subscribe( PlaylistObserver *observer )
                    { if( observer ) m_observers.insert( observer ); }
            virtual void unsubscribe( PlaylistObserver *observer )
                    { m_observers.remove( observer ); }

            virtual QStringList groups() { return QStringList(); }

            /**
            * "labels" the playlist as part of a group. In a folder-like hierachy this means adding
            * the playlist to the folder with name groups.first().
            * If groups is empty that means removing all groups from the playlist.
            */
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED(groups) }

            /* the following has been copied from Meta.h
            * it is my hope that we can integrate Playlists
            * better into the rest of the Meta framework someday ~Bart Cerneels
            * TODO: Playlist : public MetaBase
            */
            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const = 0;

            virtual Capability* createCapabilityInterface( Capability::Type type ) = 0;

            /**
             * Retrieves a specialized interface which represents a capability of this
             * MetaBase object.
             *
             * @returns a pointer to the capability interface if it exists, 0 otherwise
             */
            template <class CapIface> CapIface *create()
            {
                Meta::Capability::Type type = CapIface::capabilityInterfaceType();
                Meta::Capability *iface = createCapabilityInterface(type);
                return qobject_cast<CapIface *>(iface);
            }

            /**
             * Tests if a MetaBase object provides a given capability interface.
             *
             * @returns true if the interface is available, false otherwise
             */
            template <class CapIface> bool is() const
            {
                return hasCapabilityInterface( CapIface::capabilityInterfaceType() );
            }

            virtual KUrl retrievableUrl() { return KUrl(); }

        protected:
            inline void notifyObserversTrackAdded( Meta::TrackPtr track, int position )
            {
                foreach( Meta::PlaylistObserver *observer, m_observers )
                {
                    if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
                        observer->trackAdded( Meta::PlaylistPtr( this ), track, position );
                }
            }

            inline void notifyObserversTrackRemoved( int position )
            {
                foreach( Meta::PlaylistObserver *observer, m_observers )
                {
                    if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
                        observer->trackRemoved( Meta::PlaylistPtr( this ), position );
                }
            }

            QSet<Meta::PlaylistObserver*> m_observers;
            QString m_name;
    };

}

Q_DECLARE_METATYPE( Meta::PlaylistPtr )
Q_DECLARE_METATYPE( Meta::PlaylistList )

#endif
