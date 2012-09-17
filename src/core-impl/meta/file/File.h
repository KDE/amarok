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

#include "core/meta/Meta.h"

namespace MetaFile
{
    class Track;

    typedef KSharedPtr<Track> TrackPtr;

    class AMAROK_EXPORT Track : public Meta::Track
    {
        public:
            Track( const KUrl &url );
            virtual ~Track();

        //methods inherited from Meta::MetaBase
            virtual QString name() const;

        //methods inherited from Meta::Track
            virtual KUrl playableUrl() const;
            virtual QString prettyUrl() const;
            virtual QString uidUrl() const;

            virtual bool isPlayable() const;
            virtual bool isEditable() const;

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

            virtual void finishedPlaying( double playedFraction );
            virtual bool inCollection() const;
            virtual Collections::Collection *collection() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            virtual Meta::StatisticsPtr statistics();

        // MetaFile::Track own methods:
            /**
             * Return true if file at @param url is a track.
             *
             * This method does only basic checking of the mime type and is pretty
             * optimistic, so it may be possible that is the song is not playable with
             * current backend even if isTrack() returns true.
             */
            static bool isTrack( const KUrl &url );

            virtual void beginMetaDataUpdate();
            virtual void endMetaDataUpdate();

            virtual QImage getEmbeddedCover() const;

            virtual void setCollection( Collections::Collection *newCollection );
            virtual void setTitle( const QString &newTitle );
            virtual void setAlbum( const QString &newAlbum );
            virtual void setAlbumArtist( const QString &newAlbumArtist );
            virtual void setArtist( const QString &newArtist );
            virtual void setGenre( const QString &newGenre );
            virtual void setComposer( const QString &newComposer );
            virtual void setYear( int newYear );
            virtual void setTrackNumber( int newTrackNumber );
            virtual void setBpm( const qreal newBpm );
            virtual void setComment( const QString &newComment );
            virtual void setDiscNumber( int newDiscNumber );

            // publish method so that it can be called by Private.
            using Meta::MetaBase::notifyObservers;

            class Private;

        private:
            Private * const d;
    };
}

#endif
