/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PROXYCOLLECTIONMETA_H
#define PROXYCOLLECTIONMETA_H

#include "meta/Meta.h"

#include <QList>

namespace ProxyCollection
{
    class Collection;

    class Track : public Meta::Track, private Meta::Observer
    {
        public:
            Track( Collection *coll, const Meta::TrackPtr &track );
            ~Track();

            QString name() const;
            QString prettyName() const;

            KUrl playableUrl() const;
            QString prettyUrl() const;
            QString uidUrl() const;
            bool isPlayable() const;

            Meta::AlbumPtr album() const;
            Meta::ArtistPtr artist() const;
            Meta::ComposerPtr composer() const;
            Meta::GenrePtr genre() const;
            Meta::YearPtr year() const;

            QString comment() const;

            double score() const;
            void setScore( double newScore );
            int rating() const;
            void setRating( int newRating );
            uint firstPlayed() const;
            uint lastPlayed() const;
            int playCount() const;
            void finishedPlaying( double playedFraction );

            int length() const;
            int filesize() const;
            int sampleRate() const;
            int bitrate() const;
            int trackNumber() const;
            int discNumber() const;
            QString type() const;


            void add( const Meta::TrackPtr &track );

        protected:
            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::TrackPtr track );

        private:
            Collection *m_collection;
            Meta::TrackList m_tracks;
            QString m_name;
            Meta::AlbumPtr m_album;
            Meta::ArtistPtr m_artist;
            Meta::GenrePtr m_genre;
            Meta::ComposerPtr m_composer;
            Meta::YearPtr m_year;
    };

    class Album : public Meta::Album, private Meta::Observer
    {
        public:
        Album( Collection *coll, Meta::AlbumPtr album );
        ~Album();

        QString name() const;
        QString prettyName() const;

        Meta::TrackList tracks();
        Meta::ArtistPtr albumArtist() const;
        bool isCompilation() const;
        bool hasAlbumArtist() const;

        void add( Meta::AlbumPtr album );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::AlbumPtr album );

        private:
        Collection *m_collection;
        Meta::AlbumList m_albums;
        QString m_name;
        Meta::ArtistPtr m_albumArtist;
    };

    class Artist : public Meta::Artist, private Meta::Observer
    {
        public:
        Artist( Collection *coll, Meta::ArtistPtr artist );
        ~Artist();

        QString name() const;
        QString prettyName() const;

        Meta::TrackList tracks();

        Meta::AlbumList albums();

        void add( Meta::ArtistPtr artist );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ArtistPtr artist );

        private:
        Collection *m_collection;
        Meta::ArtistList m_artists;
        QString m_name;
    };

    class Genre : public Meta::Genre, private Meta::Observer
    {
        public:
        Genre( Collection *coll, Meta::GenrePtr genre );
        ~Genre();

        QString name() const;
        QString prettyName() const;

        Meta::TrackList tracks();

        void add( Meta::GenrePtr genre );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::GenrePtr genre );

        private:
        Collection *m_collection;
        Meta::GenreList m_genres;
        QString m_name;
    };

    class Composer : public Meta::Composer, private Meta::Observer
    {
        public:
        Composer( Collection *coll, Meta::ComposerPtr composer );
        ~Composer();

        QString name() const;
        QString prettyName() const;

        Meta::TrackList tracks();

        void add( Meta::ComposerPtr composer );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ComposerPtr composer );

        private:
        Collection *m_collection;
        Meta::ComposerList m_composers;
        QString m_name;
    };

    class Year : public Meta::Year, private Meta::Observer
    {
        public:
        Year( Collection * coll, Meta::YearPtr year );
        ~Year();

        QString name() const;
        QString prettyName() const;

        Meta::TrackList tracks();

        /**
          * adds another Meta::Year instance to be proxied.
          */
        void add( Meta::YearPtr year );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::YearPtr year );

        private:
        Collection *m_collection;
        Meta::YearList m_years;
        QString m_name;

    };
}

#endif
