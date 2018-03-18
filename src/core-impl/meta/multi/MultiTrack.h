/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef METAMULTITRACK_H
#define METAMULTITRACK_H

#include "core/capabilities/MultiSourceCapability.h"
#include "core/meta/Meta.h"
#include "core/meta/Observer.h"
#include "core/playlists/Playlist.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"

namespace Meta
{
    /**
     * A track that wraps a playlist. This is useful, for instance, for adding radio
     * streams with multiple fallback streams to the playlist as a single item.
     *
     * @author Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class MultiTrack : public QObject, public Track, private Meta::Observer, private Playlists::PlaylistObserver
    {
        Q_OBJECT

        public:
            explicit MultiTrack( Playlists::PlaylistPtr playlist );
            ~MultiTrack();

            QStringList sources() const;
            void setSource( int source );
            int current() const;
            QUrl nextUrl() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type );

            // forward lots of stuff:
#define FORWARD( Type, method, default ) Type method() const { return m_currentTrack ? m_currentTrack->method() : default; }
            FORWARD( QString, name, QString() )
            FORWARD( QString, prettyName, QString() )
            FORWARD( QUrl, playableUrl, QUrl() )
            FORWARD( QString, prettyUrl, m_playlist->uidUrl().toDisplayString() )
            // TODO: change to m_playlist->uidUrl() unconditionally once playlist restorer can cope with it:
            FORWARD( QString, uidUrl, m_playlist->uidUrl().url() )
            FORWARD( QString, notPlayableReason, i18nc( "Reason why a track is not playable",
                                                        "Underlying playlist is empty" ) )

            FORWARD( AlbumPtr, album, AlbumPtr( new DefaultAlbum() ) );
            FORWARD( ArtistPtr, artist, ArtistPtr( new DefaultArtist() ) );
            FORWARD( ComposerPtr, composer, ComposerPtr( new DefaultComposer() ) );
            FORWARD( GenrePtr, genre, GenrePtr( new DefaultGenre() ) );
            FORWARD( YearPtr, year, YearPtr( new DefaultYear() ) );

            FORWARD( qreal, bpm, -1 )
            FORWARD( QString, comment, QString() )
            FORWARD( qint64, length, 0 )
            FORWARD( int, filesize, 0 )
            FORWARD( int, sampleRate, 0 )
            FORWARD( int, bitrate, 0 )
            FORWARD( int, trackNumber, 0 )
            FORWARD( int, discNumber, 0 )
            FORWARD( QString, type, QString() )
#undef FORWARD

            void prepareToPlay();
            virtual StatisticsPtr statistics();

        Q_SIGNALS:
            void urlChanged( const QUrl &url );

        private:
            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::TrackPtr track );

            using PlaylistObserver::metadataChanged;
            virtual void trackAdded( Playlists::PlaylistPtr playlist, TrackPtr track, int position );

            /**
             * Implementation for setSource. Must be called with m_lock held for writing.
             */
            void setSourceImpl( int source );

            // marked as mutable because many Playlist methods aren't const while they should be
            mutable Playlists::PlaylistPtr m_playlist;
            TrackPtr m_currentTrack;
            /**
             * Guards access to data members; note that m_playlist methods are considered
             * thread-safe and the pointer itself does not change throughout life of thhis
             * object, so mere m_playlist->someMethod() doesn't have to be guarded.
             */
            mutable QReadWriteLock m_lock;
    };
}

#endif
