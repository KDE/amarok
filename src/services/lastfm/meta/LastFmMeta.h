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
            virtual ~Track();

        // methods inherited from Meta::Base
            virtual QString name() const;
            virtual QString sortableName() const;

        // methods inherited from Meta::Track
            virtual QUrl playableUrl() const;
            virtual QString prettyUrl() const;
            virtual QString uidUrl() const;
            virtual QString notPlayableReason() const;

            virtual Meta::AlbumPtr album() const;
            virtual Meta::ArtistPtr artist() const;
            virtual Meta::GenrePtr genre() const;
            virtual Meta::ComposerPtr composer() const;
            virtual Meta::YearPtr year() const;

            virtual qreal bpm() const;

            virtual QString comment() const;

            virtual int trackNumber() const;

            virtual int discNumber() const;

            virtual qint64 length() const;
            virtual int filesize() const;
            virtual int sampleRate() const;
            virtual int bitrate() const;

            virtual QString type() const;

            virtual bool inCollection() const;
            virtual Collections::Collection *collection() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            virtual Meta::StatisticsPtr statistics();

        // own methods:
            void setTrackInfo( const lastfm::Track &trackInfo );

            virtual QString sourceName();
            virtual QString sourceDescription();
            virtual QPixmap emblem();
            virtual QString scalableEmblem();

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
            ~LastFmProviderCapability();
    };

    typedef AmarokSharedPointer<Track> TrackPtr;
}

#endif
