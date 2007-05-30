/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#include "meta.h"

#include <QObject>

namespace LastFm
{
    class Track : public Meta::Track, QObject
    {
        Q_OBJECT
        public:
            Track();
            virtual ~Track();

        //methods inherited from Meta::Track
            virtual KUrl playableUrl() const;
            virtual QString prettyUrl() const;
            virtual QString url() const;

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
            virtual Collection *collection() const;

            virtual void subscribe( TrackObserver *observer );
            virtual void unsubscribe( TrackObserver *observer );


        //LastFm specific methods, cast the object to LastFm::Track to use them
        //you can cast the Track when type() returns "stream/lastfm" (or use a dynamic cast:)
        public slots:
            void love();
            void ban();
            void skip();

        private:
            //use a d-pointer because some code is going to work directly with LastFm::Track
            class Private;
            Private * const d;
    };

};
