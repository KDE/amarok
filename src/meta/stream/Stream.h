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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_STREAM_H
#define AMAROK_STREAM_H

#include "amarok_export.h"
#include "Meta.h"



namespace MetaStream
{
    class AMAROK_EXPORT Track : public Meta::Track
    {
        public:
            class Private;

            Track( const KUrl &url );
            virtual ~Track();

        //methods inherited from Meta::MetaBase
            virtual QString name() const;
            virtual QString prettyName() const;
            virtual QString fullPrettyName() const;
            virtual QString sortableName() const;

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

            virtual void setAlbum( const QString &newAlbum );
            virtual void setArtist( const QString &newArtist );
            virtual void setGenre( const QString &newGenre );
            virtual void setComposer( const QString &newComposer );
            virtual void setYear( const QString &newYear );

            virtual void setTitle( const QString &newTitle );

            virtual QString comment() const;
            virtual void setComment( const QString &newComment );

            virtual double score() const;
            virtual void setScore( double newScore );

            virtual int rating() const;
            virtual void setRating( int newRating );

            virtual int trackNumber() const;
            virtual void setTrackNumber( int newTrackNumber );

            virtual int discNumber() const;
            virtual void setDiscNumber( int newDiscNumber );

            virtual int length() const;
            virtual int filesize() const;
            virtual int sampleRate() const;
            virtual int bitrate() const;
            
            virtual uint lastPlayed() const;
            virtual int playCount() const;

            virtual QString type() const;

            virtual void beginMetaDataUpdate();
            virtual void endMetaDataUpdate();
            virtual void abortMetaDataUpdate();

            virtual void finishedPlaying( double playedFraction );

            virtual bool inCollection() const;
            virtual Amarok::Collection *collection() const;

            virtual void subscribe( Meta::Observer *observer );
            virtual void unsubscribe( Meta::Observer *observer );

            virtual void updateUrl( const KUrl &url );

        private:
            Private * const d;
    };

}

#endif
