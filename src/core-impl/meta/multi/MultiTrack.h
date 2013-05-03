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

namespace Meta
{
    /**
     * A track that wraps a playlist. This is useful, for instance, for adding radio
     * streams with multiple fallback streams to the playlist as a single item.
     *
     * @author Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class MultiTrack : public QObject, public Track, public Meta::Observer
    {
        Q_OBJECT

        public:
            MultiTrack( Playlists::PlaylistPtr playlist );
            ~MultiTrack();

            QStringList sources() const;
            void setSource( int source );
            int current() const;
            KUrl nextUrl() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type );

            //forward lots of stuff:

            //TODO: sanity checks on m_currentTrack
            virtual QString name() const { return m_currentTrack->name(); }
            virtual QString prettyName() const { return m_currentTrack->prettyName(); }
            virtual KUrl playableUrl() const { return m_currentTrack->playableUrl(); }
            virtual QString prettyUrl() const { return m_currentTrack->prettyUrl(); }
            virtual QString uidUrl() const { return m_currentTrack->uidUrl(); }
            virtual QString notPlayableReason() const { return m_currentTrack->notPlayableReason(); }

            virtual AlbumPtr album() const { return m_currentTrack->album(); }
            virtual ArtistPtr artist() const { return m_currentTrack->artist(); }
            virtual ComposerPtr composer() const { return m_currentTrack->composer(); }
            virtual GenrePtr genre() const { return m_currentTrack->genre(); }
            virtual YearPtr year() const { return m_currentTrack->year(); }

            virtual qreal bpm() const { return m_currentTrack->bpm(); }
            virtual QString comment() const { return m_currentTrack->comment(); }
            virtual qint64 length() const { return m_currentTrack->length(); }
            virtual int filesize() const { return m_currentTrack->filesize(); }
            virtual int sampleRate() const { return m_currentTrack->sampleRate(); }
            virtual int bitrate() const { return m_currentTrack->bitrate(); }
            virtual int trackNumber() const { return m_currentTrack->trackNumber(); }
            virtual int discNumber() const { return m_currentTrack->discNumber(); }

            virtual QString type() const { return m_currentTrack->type(); }

            virtual StatisticsPtr statistics();

            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::TrackPtr track );

        signals:
            void urlChanged( const KUrl &url );

        private:
            // marked as mutable because many Playlist methods aren't const while they should be
            mutable Playlists::PlaylistPtr m_playlist;
            TrackPtr m_currentTrack;
            int m_index;
    };
}

#endif
