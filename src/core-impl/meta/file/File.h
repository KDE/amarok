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
            virtual ~Track();

        //methods inherited from Meta::Base
            virtual QString name() const;

        //methods inherited from Meta::Track
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
            virtual QDateTime createDate() const;

            virtual qreal replayGain( Meta::ReplayGainTag mode ) const;

            virtual QString type() const;

            virtual bool inCollection() const;
            virtual Collections::Collection *collection() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            virtual Meta::TrackEditorPtr editor();
            virtual Meta::StatisticsPtr statistics();

        // Meta::TrackEditor methods:
            virtual void setAlbum( const QString &newAlbum );
            virtual void setAlbumArtist( const QString &newAlbumArtist );
            virtual void setArtist( const QString &newArtist );
            virtual void setComposer( const QString &newComposer );
            virtual void setGenre( const QString &newGenre );
            virtual void setYear( int newYear );
            virtual void setTitle( const QString &newTitle );
            virtual void setComment( const QString &newComment );
            virtual void setTrackNumber( int newTrackNumber );
            virtual void setDiscNumber( int newDiscNumber );
            virtual void setBpm( const qreal newBpm );

        // Meta::Statistics methods:
            virtual double score() const;
            virtual void setScore( double newScore );

            virtual int rating() const;
            virtual void setRating( int newRating );

            virtual int playCount() const;
            virtual void setPlayCount( int newPlayCount );

        // combined Meta::TrackEditor, Meta::Statistics methods:
            virtual void beginUpdate();
            virtual void endUpdate();

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
