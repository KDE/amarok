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

#include "core/meta/Meta.h"

#include <QList>

namespace Amarok
{
    class Collection;
}

namespace ProxyCollection
{
    class Collection;

    class AMAROK_EXPORT_TESTS Track : public Meta::Track, private Meta::Observer
    {
        public:
            Track( Collection *coll, const Meta::TrackPtr &track );
            ~Track();

            QString name() const;
            QString prettyName() const;
            virtual QString sortableName() const;

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
            float   bpm() const;

            double score() const;
            void setScore( double newScore );
            int rating() const;
            void setRating( int newRating );
            uint firstPlayed() const;
            uint lastPlayed() const;
            int playCount() const;
            void finishedPlaying( double playedFraction );

            qint64 length() const;
            int filesize() const;
            int sampleRate() const;
            int bitrate() const;
            QDateTime createDate() const;
            int trackNumber() const;
            int discNumber() const;
            QString type() const;

            Amarok::Collection* collection() const;

            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
            virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

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

    class AMAROK_EXPORT_TESTS Album : public Meta::Album, private Meta::Observer
    {
        public:
        Album( Collection *coll, Meta::AlbumPtr album );
        ~Album();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();
        Meta::ArtistPtr albumArtist() const;
        bool isCompilation() const;
        bool hasAlbumArtist() const;

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        void add( Meta::AlbumPtr album );

        /** returns true if the album has a cover set */
        virtual bool hasImage( int size = 1 ) const;
        /** returns the cover of the album */
        virtual QPixmap image( int size = 1 );
        /** returns the image location on disk */
        virtual KUrl imageLocation( int size = 1 );
        /** returns the cover of the album with a nice border around it*/
        virtual QPixmap imageWithBorder( int size = 1, int borderWidth = 5 );
        /** Returns true if it is possible to update the cover of the album */
        virtual bool canUpdateImage() const;
        /** updates the cover of the album */
        virtual void setImage( const QPixmap &pixmap );
        virtual void removeImage();
        /** don't automatically fetch artwork */
        virtual void setSuppressImageAutoFetch( const bool suppress );
        /** should automatic artwork retrieval be suppressed? */
        virtual bool suppressImageAutoFetch() const;

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::AlbumPtr album );

        private:
        Collection *m_collection;
        Meta::AlbumList m_albums;
        QString m_name;
        Meta::ArtistPtr m_albumArtist;
    };

    class AMAROK_EXPORT_TESTS Artist : public Meta::Artist, private Meta::Observer
    {
        public:
        Artist( Collection *coll, Meta::ArtistPtr artist );
        ~Artist();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        Meta::AlbumList albums();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        void add( Meta::ArtistPtr artist );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ArtistPtr artist );

        private:
        Collection *m_collection;
        Meta::ArtistList m_artists;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS Genre : public Meta::Genre, private Meta::Observer
    {
        public:
        Genre( Collection *coll, Meta::GenrePtr genre );
        ~Genre();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        void add( Meta::GenrePtr genre );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::GenrePtr genre );

        private:
        Collection *m_collection;
        Meta::GenreList m_genres;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS Composer : public Meta::Composer, private Meta::Observer
    {
        public:
        Composer( Collection *coll, Meta::ComposerPtr composer );
        ~Composer();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        void add( Meta::ComposerPtr composer );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ComposerPtr composer );

        private:
        Collection *m_collection;
        Meta::ComposerList m_composers;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS Year : public Meta::Year, private Meta::Observer
    {
        public:
        Year( Collection * coll, Meta::YearPtr year );
        ~Year();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

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
