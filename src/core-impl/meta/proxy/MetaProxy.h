/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_METAPROXY_H
#define AMAROK_METAPROXY_H

#include "amarok_export.h"
#include "core/capabilities/Capability.h"
#include "core/meta/Meta.h"
#include "core/meta/TrackEditor.h"
#include "core-impl/meta/proxy/MetaProxyWorker.h"

#include <QObject>

namespace Collections
{
    class TrackProvider;
}

namespace MetaProxy
{
    class Track;
    typedef KSharedPtr<Track> TrackPtr;
    class AMAROK_EXPORT Track : public Meta::Track, public Meta::TrackEditor
    {
        public:
            class Private;

            enum LookupType {
                AutomaticLookup,
                ManualLookup
            };

            /**
             * Construct a lazy-loading proxying track. You must assign this track to a
             * KSharedPtr right after constructing it.
             *
             * If @param lookupType is AutomaticLookup (the default), an asynchronous
             * job employing CollectionManager to lookup the track in TrackProviders is
             * enqueued and started right from this constructor.
             *
             * If @param lookupType is ManualLookup, lookup is not done automatically
             * and you are responsible to call lookupTrack() once it is feasible. This way
             * you can also optionally define which TrackProvider will be used.
             */
            Track( const QUrl &url, LookupType lookupType = AutomaticLookup );
            virtual ~Track();

            /**
             * Tell MetaProxy::Track to start looking up the real track. Only valid if
             * this Track is constructed with lookupType = ManualLookup. This method
             * returns quickly and the lookup happens asynchronously in a thread (in
             * other words, @param provider, id supplied, must be thread-safe).
             *
             * If @param provider is null (the default), lookup happens in all
             * registered providers by employing CollectionManager. Otherwise lookup
             * only checks @param provider (still asynchronously).
             */
            void lookupTrack( Collections::TrackProvider *provider = 0 );

        // methods inherited from Meta::MetaCapability
            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        // methods inherited from Meta::Base
            virtual QString name() const;
            virtual QString prettyName() const;
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

            virtual Meta::LabelList labels() const;
            virtual qreal bpm() const;
            virtual QString comment() const;
            virtual qint64 length() const;
            virtual int filesize() const;
            virtual int sampleRate() const;
            virtual int bitrate() const;
            virtual QDateTime createDate() const;
            virtual QDateTime modifyDate() const;
            virtual int trackNumber() const;
            virtual int discNumber() const;
            virtual qreal replayGain( Meta::ReplayGainTag mode ) const;

            virtual QString type() const;

            virtual void prepareToPlay();
            virtual void finishedPlaying( double playedFraction );

            virtual bool inCollection() const;
            virtual Collections::Collection *collection() const;

            virtual QString cachedLyrics() const;
            virtual void setCachedLyrics( const QString &lyrics );

            virtual void addLabel( const QString &label );
            virtual void addLabel( const Meta::LabelPtr &label );
            virtual void removeLabel( const Meta::LabelPtr &label );

            virtual Meta::TrackEditorPtr editor();
            virtual Meta::StatisticsPtr statistics();

            virtual bool operator==( const Meta::Track &track ) const;

        // Meta::TrackEditor methods:
            virtual void setAlbum( const QString &album );
            virtual void setAlbumArtist( const QString &artist );
            virtual void setArtist( const QString &artist );
            virtual void setComposer( const QString &composer );
            virtual void setGenre( const QString &genre );
            virtual void setYear( int year );
            virtual void setComment( const QString &comment );
            virtual void setTitle( const QString &name );
            virtual void setTrackNumber( int number );
            virtual void setDiscNumber( int discNumber );
            virtual void setBpm( const qreal bpm );

            virtual void beginUpdate();
            virtual void endUpdate();

        // custom MetaProxy methods
            /**
             * Return true if underlying track has already been found, false otherwise.
             */
            bool isResolved() const;
            void setLength( qint64 length );

            /**
             * MetaProxy will update the proxy with the track.
             */
            void updateTrack( Meta::TrackPtr track );

        private:
            Q_DISABLE_COPY( Track )

            Private *const d;  // constant pointer to non-constant object
    };

}

#endif
