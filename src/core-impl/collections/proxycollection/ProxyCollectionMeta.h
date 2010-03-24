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

namespace Collections {
    class ProxyCollection;
}

namespace Meta {

    class AMAROK_EXPORT_TESTS ProxyTrack : public Meta::Track, private Meta::Observer
    {
        public:
            ProxyTrack( Collections::ProxyCollection *coll, const Meta::TrackPtr &track );
            ~ProxyTrack();

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
            qreal   bpm() const;

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

            Collections::Collection* collection() const;

            virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
            virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

            virtual void addLabel( const QString &label );
            virtual void addLabel( const Meta::LabelPtr &label );
            virtual void removeLabel( const Meta::LabelPtr &label );
            virtual Meta::LabelList labels() const;

            void add( const Meta::TrackPtr &track );

        protected:
            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::TrackPtr track );

        private:
            Collections::ProxyCollection *m_collection;
            Meta::TrackList m_tracks;
            QString m_name;
            Meta::AlbumPtr m_album;
            Meta::ArtistPtr m_artist;
            Meta::GenrePtr m_genre;
            Meta::ComposerPtr m_composer;
            Meta::YearPtr m_year;
    };

    class AMAROK_EXPORT_TESTS ProxyAlbum : public Meta::Album, private Meta::Observer
    {
        public:
        ProxyAlbum( Collections::ProxyCollection *coll, Meta::AlbumPtr album );
        ~ProxyAlbum();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();
        Meta::ArtistPtr albumArtist() const;
        bool isCompilation() const;
        bool hasAlbumArtist() const;

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

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
        Collections::ProxyCollection *m_collection;
        Meta::AlbumList m_albums;
        QString m_name;
        Meta::ArtistPtr m_albumArtist;
    };

    class AMAROK_EXPORT_TESTS ProxyArtist : public Meta::Artist, private Meta::Observer
    {
        public:
        ProxyArtist( Collections::ProxyCollection *coll, Meta::ArtistPtr artist );
        ~ProxyArtist();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        Meta::AlbumList albums();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        void add( Meta::ArtistPtr artist );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ArtistPtr artist );

        private:
        Collections::ProxyCollection *m_collection;
        Meta::ArtistList m_artists;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS ProxyGenre : public Meta::Genre, private Meta::Observer
    {
        public:
        ProxyGenre( Collections::ProxyCollection *coll, Meta::GenrePtr genre );
        ~ProxyGenre();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        void add( Meta::GenrePtr genre );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::GenrePtr genre );

        private:
        Collections::ProxyCollection *m_collection;
        Meta::GenreList m_genres;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS ProxyComposer : public Meta::Composer, private Meta::Observer
    {
        public:
        ProxyComposer( Collections::ProxyCollection *coll, Meta::ComposerPtr composer );
        ~ProxyComposer();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        void add( Meta::ComposerPtr composer );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ComposerPtr composer );

        private:
        Collections::ProxyCollection *m_collection;
        Meta::ComposerList m_composers;
        QString m_name;
    };

    class AMAROK_EXPORT_TESTS ProxyYear : public Meta::Year, private Meta::Observer
    {
        public:
        ProxyYear( Collections::ProxyCollection * coll, Meta::YearPtr year );
        ~ProxyYear();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        /**
          * adds another Meta::Year instance to be proxied.
          */
        void add( Meta::YearPtr year );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::YearPtr year );

        private:
        Collections::ProxyCollection *m_collection;
        Meta::YearList m_years;
        QString m_name;

    };

    class AMAROK_EXPORT_TESTS ProxyLabel : public Meta::Label
    {
    public:
        ProxyLabel( Collections::ProxyCollection *coll, const Meta::LabelPtr &label );
        virtual ~ProxyLabel();

        virtual QString name() const;
        virtual QString prettyName() const;
        virtual QString sortableName() const;

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        /**
          adds another Meta::Label instance to be proxied.
          */
        void add( const Meta::LabelPtr &label );

    private:
        Collections::ProxyCollection *m_collection;
        Meta::LabelList m_labels;
        QString m_name;
    };
}
} //namespace Meta

#endif
