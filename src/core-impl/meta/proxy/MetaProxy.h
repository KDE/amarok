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
    typedef AmarokSharedPointer<Track> TrackPtr;
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
             * AmarokSharedPointer right after constructing it.
             *
             * If @p lookupType is AutomaticLookup (the default), an asynchronous
             * job employing CollectionManager to lookup the track in TrackProviders is
             * enqueued and started right from this constructor.
             *
             * If @p lookupType is ManualLookup, lookup is not done automatically
             * and you are responsible to call lookupTrack() once it is feasible. This way
             * you can also optionally define which TrackProvider will be used.
             *
             * @param url th URL
             * @param lookupType lookup type
             */
            explicit Track( const QUrl &url, LookupType lookupType = AutomaticLookup );
            virtual ~Track();

            /**
             * Tell MetaProxy::Track to start looking up the real track. Only valid if
             * this Track is constructed with lookupType = ManualLookup. This method
             * returns quickly and the lookup happens asynchronously in a thread (in
             * other words, @p provider, id supplied, must be thread-safe).
             *
             * If @p provider is null (the default), lookup happens in all
             * registered providers by employing CollectionManager. Otherwise lookup
             * only checks @param provider (still asynchronously).
             */
            void lookupTrack( Collections::TrackProvider *provider = 0 );

        // methods inherited from Meta::MetaCapability
            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        // methods inherited from Meta::Base
            QString name() const override;
            QString prettyName() const override;
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

            Meta::LabelList labels() const override;
            qreal bpm() const override;
            QString comment() const override;
            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;
            QDateTime createDate() const override;
            QDateTime modifyDate() const override;
            int trackNumber() const override;
            int discNumber() const override;
            qreal replayGain( Meta::ReplayGainTag mode ) const override;

            QString type() const override;

            void prepareToPlay() override;
            void finishedPlaying( double playedFraction ) override;

            bool inCollection() const override;
            Collections::Collection *collection() const override;

            QString cachedLyrics() const override;
            void setCachedLyrics( const QString &lyrics ) override;

            void addLabel( const QString &label ) override;
            void addLabel( const Meta::LabelPtr &label ) override;
            void removeLabel( const Meta::LabelPtr &label ) override;

            Meta::TrackEditorPtr editor() override;
            Meta::StatisticsPtr statistics() override;

            bool operator==( const Meta::Track &track ) const override;

        // Meta::TrackEditor methods:
            void setAlbum( const QString &album ) override;
            void setAlbumArtist( const QString &artist ) override;
            void setArtist( const QString &artist ) override;
            void setComposer( const QString &composer ) override;
            void setGenre( const QString &genre ) override;
            void setYear( int year ) override;
            void setComment( const QString &comment ) override;
            void setTitle( const QString &name ) override;
            void setTrackNumber( int number ) override;
            void setDiscNumber( int discNumber ) override;
            void setBpm( const qreal bpm ) override;

            void beginUpdate() override;
            void endUpdate() override;

        // custom MetaProxy methods
            /**
             * Return true if underlying track has already been found, false otherwise.
             */
            bool isResolved() const;
            void setLength( qint64 length );

            /**
             * MetaProxy will update the proxy with the track.
             */
            void updateTrack( const Meta::TrackPtr &track );

        private:
            Q_DISABLE_COPY( Track )

            Private *const d;  // constant pointer to non-constant object
    };

}

#endif
