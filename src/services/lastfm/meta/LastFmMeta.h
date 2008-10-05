/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2008 Shane King <kde@dontletsstart.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_LASTFMMETA_H
#define AMAROK_LASTFMMETA_H

#include "meta/Meta.h"
#include "meta/Capability.h"
#include "ServiceMetaBase.h" // for the SourceInfoProvider
#include "Track.h"

#include <QObject>

class Request;
class TrackInfo;

namespace LastFm
{
    class Track : public QObject, public Meta::Track, public SourceInfoProvider
    {
        Q_OBJECT
        public:
            class Private;

            Track( const QString &lastFmUri );
            Track( ::Track track ); //Convienience Constructor to allow constructing a Meta::LastFmTrack from a LastFmTrack (confusing?)
            virtual ~Track();

        //methods inherited from Meta::MetaBase
            virtual QString name() const;
            virtual QString prettyName() const;
            virtual QString fullPrettyName() const;
            virtual QString sortableName() const;
            virtual QString fixedName() const;
        //methods inherited from Meta::Track
            virtual KUrl playableUrl() const;
            virtual QString prettyUrl() const;
            virtual QString uidUrl() const;

            virtual bool isPlayable() const;

            virtual Meta::AlbumPtr album() const;
            virtual Meta::ArtistPtr artist() const;
            virtual Meta::GenrePtr genre() const;
            virtual Meta::ComposerPtr composer() const;
            virtual Meta::YearPtr year() const;

            virtual QString comment() const;

            virtual double score() const;
            virtual void setScore( double newScore );

            virtual int rating() const;
            virtual void setRating( int newRating );

            virtual int trackNumber() const;

            virtual int discNumber() const;

            virtual int length() const;
            virtual int filesize() const;
            virtual int sampleRate() const;
            virtual int bitrate() const;
            virtual uint lastPlayed() const;
            virtual int playCount() const;

            virtual QString type() const;

            virtual void finishedPlaying( double playedFraction );

            virtual bool inCollection() const;
            virtual Collection *collection() const;

            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;

            virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

            void setTrackInfo( const TrackInfo &trackInfo );

            virtual QString sourceName();
            virtual QString sourceDescription();
            virtual QPixmap emblem();

            QList< PopupDropperAction * > nowPlayingActions() const;

        //LastFm specific methods, cast the object to LastFm::Track to use them
        //you can cast the Track when type() returns "stream/lastfm" (or use a dynamic cast:)
            QString streamName() const; // A nice name for the stream..
        public slots:
            void love();
            void ban();
            void skip();

        // MultiPlayableCapability methods
            void playCurrent();
            void playNext();

        private slots:
            void slotResultReady( Request* );
        private:
            void init( int id = -1 );
            //use a d-pointer because some code is going to work directly with LastFm::Track
            Private * const d;
            QList< PopupDropperAction * > m_currentTrackActions;
    };

    class LastFmProviderCapability : public Meta::Capability
    {
        public:
            LastFmProviderCapability();
            ~LastFmProviderCapability();
    };

    typedef KSharedPtr<Track> TrackPtr;
}

#endif
