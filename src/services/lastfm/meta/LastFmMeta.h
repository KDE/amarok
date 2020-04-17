/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_LASTFMMETA_H
#define AMAROK_LASTFMMETA_H

#include "core/meta/Meta.h"
#include "core/capabilities/Capability.h"
#include "services/ServiceMetaBase.h" // for the SourceInfoProvider

namespace lastfm {
    class Track;
}

namespace LastFm
{
    class Track : public QObject, public Meta::Track, public SourceInfoProvider
    {
        Q_OBJECT
        public:
            class Private;

            explicit Track( const QString &lastFmUri );
            explicit Track( lastfm::Track track ); //Convenience Constructor to allow constructing a Meta::LastFmTrack from a LastFmTrack (confusing?)
            ~Track() override;

        // methods inherited from Meta::Base
            QString name() const override;
            QString sortableName() const override;

        // methods inherited from Meta::Track
            QUrl playableUrl() const override;
            QString prettyUrl() const override;
            QString uidUrl() const override;
            QString notPlayableReason() const override;

            Meta::AlbumPtr album() const override;
            Meta::ArtistPtr artist() const override;
            Meta::GenrePtr genre() const override;
            Meta::ComposerPtr composer() const override;
            Meta::YearPtr year() const override;

            qreal bpm() const override;

            QString comment() const override;

            int trackNumber() const override;

            int discNumber() const override;

            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;

            QString type() const override;

            bool inCollection() const override;
            Collections::Collection *collection() const override;

            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

            Meta::StatisticsPtr statistics() override;

        // own methods:
            void setTrackInfo( const lastfm::Track &trackInfo );

            QString sourceName() override;
            QString sourceDescription() override;
            QPixmap emblem() override;
            QString scalableEmblem() override;

            //LastFm specific methods, cast the object to LastFm::Track to use them
            //you can cast the Track when type() returns "stream/lastfm" (or use a dynamic cast:)
            QUrl internalUrl() const; // this returns the private temporary url to the .mp3, DO NOT USE,
                                   // if you are asking, it has already expired
            QString streamName() const; // A nice name for the stream..

        public Q_SLOTS:
            void ban();

        private Q_SLOTS:
            void slotResultReady();
            void slotWsReply();

        Q_SIGNALS:
            void skipTrack(); // needed for communication with multiplayablecapability

        private:
            void init( int id = -1 );
            //use a d-pointer because some code is going to work directly with LastFm::Track
            Private * const d;
            QList< QAction * > m_trackActions;
    };

    class LastFmProviderCapability : public Capabilities::Capability
    {
        public:
            LastFmProviderCapability();
            ~LastFmProviderCapability() override;
    };

    typedef AmarokSharedPointer<Track> TrackPtr;
}

#endif
