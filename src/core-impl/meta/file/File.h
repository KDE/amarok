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

#ifndef AMAROK_META_FILE_H
#define AMAROK_META_FILE_H

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"

namespace MetaFile
{
    class Track;

    typedef AmarokSharedPointer<Track> TrackPtr;

    class AMAROK_EXPORT Track : public Meta::Track, public Meta::Statistics, Meta::TrackEditor
    {
        public:
            explicit Track( const QUrl &url );
            ~Track() override;

        //methods inherited from Meta::Base
            QString name() const override;

        //methods inherited from Meta::Track
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
            QDateTime createDate() const override;

            qreal replayGain( Meta::ReplayGainTag mode ) const override;

            QString type() const override;

            bool inCollection() const override;
            Collections::Collection *collection() const override;

            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

            Meta::TrackEditorPtr editor() override;
            Meta::StatisticsPtr statistics() override;

        // Meta::TrackEditor methods:
            void setAlbum( const QString &newAlbum ) override;
            void setAlbumArtist( const QString &newAlbumArtist ) override;
            void setArtist( const QString &newArtist ) override;
            void setComposer( const QString &newComposer ) override;
            void setGenre( const QString &newGenre ) override;
            void setYear( int newYear ) override;
            void setTitle( const QString &newTitle ) override;
            void setComment( const QString &newComment ) override;
            void setTrackNumber( int newTrackNumber ) override;
            void setDiscNumber( int newDiscNumber ) override;
            void setBpm( const qreal newBpm ) override;

        // Meta::Statistics methods:
            double score() const override;
            void setScore( double newScore ) override;

            int rating() const override;
            void setRating( int newRating ) override;

            int playCount() const override;
            void setPlayCount( int newPlayCount ) override;

        // combined Meta::TrackEditor, Meta::Statistics methods:
            void beginUpdate() override;
            void endUpdate() override;

        // MetaFile::Track own methods:
            bool isEditable() const;

            /**
             * Return true if file at @param url is a track.
             *
             * This method does only basic checking of the mime type and is pretty
             * optimistic, so it may be possible that is the song is not playable with
             * current backend even if isTrack() returns true.
             */
            static bool isTrack( const QUrl &url );

            virtual QImage getEmbeddedCover() const;
            virtual void setCollection( Collections::Collection *newCollection );

            // publish method so that it can be called by Private.
            using Meta::Track::notifyObservers;

            class Private;

        private:
            Private * const d;

            /**
             * Must be called at end of every set*() method, with d->lock locked for
             * writing. Takes care of writing back the fields, re-reading them and
             * notifying observers.
             */
            void commitIfInNonBatchUpdate( qint64 field, const QVariant &value );
            void commitIfInNonBatchUpdate();
    };
}

#endif
