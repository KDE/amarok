/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AGGREGATEMETA_H
#define AGGREGATEMETA_H

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "core/meta/Observer.h"
#include "core/meta/Statistics.h"

#include <QList>

namespace Collections {
    class AggregateCollection;
}

namespace Meta {

    class AMAROK_EXPORT AggregateTrack : public Meta::Track, public Meta::Statistics, private Meta::Observer
    {
        public:
            AggregateTrack( Collections::AggregateCollection *coll, const Meta::TrackPtr &track );
            ~AggregateTrack();

            QString name() const;
            QString prettyName() const;
            virtual QString sortableName() const;

            QUrl playableUrl() const;
            QString prettyUrl() const;
            QString uidUrl() const;

            /**
             * Return a comma separated list of reasons why constituent
             * tracks are unplayable or an empty string if any of the tracks is playable
             */
            QString notPlayableReason() const;

            Meta::AlbumPtr album() const;
            Meta::ArtistPtr artist() const;
            Meta::ComposerPtr composer() const;
            Meta::GenrePtr genre() const;
            Meta::YearPtr year() const;

            QString comment() const;
            qreal   bpm() const;

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

            virtual TrackEditorPtr editor();
            virtual StatisticsPtr statistics();

            // Meta::Statistics methods:
            double score() const;
            void setScore( double newScore );
            int rating() const;
            void setRating( int newRating );
            QDateTime firstPlayed() const;
            void setFirstPlayed( const QDateTime &date );
            QDateTime lastPlayed() const;
            void setLastPlayed( const QDateTime &date );
            int playCount() const;
            void setPlayCount( int newPlayCount );

            void add( const Meta::TrackPtr &track );

        protected:
            using Observer::metadataChanged;
            virtual void metadataChanged( Meta::TrackPtr track );

        private:
            Collections::AggregateCollection *m_collection;
            Meta::TrackList m_tracks;
            QString m_name;
            Meta::AlbumPtr m_album;
            Meta::ArtistPtr m_artist;
            Meta::GenrePtr m_genre;
            Meta::ComposerPtr m_composer;
            Meta::YearPtr m_year;
    };

    class AMAROK_EXPORT AggregateAlbum : public Meta::Album, private Meta::Observer
    {
        public:
        AggregateAlbum( Collections::AggregateCollection *coll, Meta::AlbumPtr album );
        ~AggregateAlbum();

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
        virtual bool hasImage( int size = 0 ) const;
        /** returns the cover of the album */
        virtual QImage image( int size = 0 ) const;
        /** returns the image location on disk */
        virtual QUrl imageLocation( int size = 0 );
        /** returns the cover of the album with a nice border around it*/
        virtual QPixmap imageWithBorder( int size = 0, int borderWidth = 5 );
        /** Returns true if it is possible to update the cover of the album */
        virtual bool canUpdateImage() const;
        /** updates the cover of the album */
        virtual void setImage( const QImage &image );
        virtual void removeImage();
        /** don't automatically fetch artwork */
        virtual void setSuppressImageAutoFetch( const bool suppress );
        /** should automatic artwork retrieval be suppressed? */
        virtual bool suppressImageAutoFetch() const;

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::AlbumPtr album );

        private:
        Collections::AggregateCollection *m_collection;
        Meta::AlbumList m_albums;
        QString m_name;
        Meta::ArtistPtr m_albumArtist;
    };

    class AMAROK_EXPORT AggregateArtist : public Meta::Artist, private Meta::Observer
    {
        public:
        AggregateArtist( Collections::AggregateCollection *coll, Meta::ArtistPtr artist );
        ~AggregateArtist();

        QString name() const;
        QString prettyName() const;
        virtual QString sortableName() const;

        Meta::TrackList tracks();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        void add( Meta::ArtistPtr artist );

        protected:
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::ArtistPtr artist );

        private:
        Collections::AggregateCollection *m_collection;
        Meta::ArtistList m_artists;
        QString m_name;
    };

    class AMAROK_EXPORT AggregateGenre : public Meta::Genre, private Meta::Observer
    {
        public:
        AggregateGenre( Collections::AggregateCollection *coll, Meta::GenrePtr genre );
        ~AggregateGenre();

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
        Collections::AggregateCollection *m_collection;
        Meta::GenreList m_genres;
        QString m_name;
    };

    class AMAROK_EXPORT AggregateComposer : public Meta::Composer, private Meta::Observer
    {
        public:
        AggregateComposer( Collections::AggregateCollection *coll, Meta::ComposerPtr composer );
        ~AggregateComposer();

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
        Collections::AggregateCollection *m_collection;
        Meta::ComposerList m_composers;
        QString m_name;
    };

    class AMAROK_EXPORT AggreagateYear : public Meta::Year, private Meta::Observer
    {
        public:
        AggreagateYear( Collections::AggregateCollection * coll, Meta::YearPtr year );
        ~AggreagateYear();

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
        Collections::AggregateCollection *m_collection;
        Meta::YearList m_years;
        QString m_name;

    };

    class AMAROK_EXPORT AggregateLabel : public Meta::Label
    {
    public:
        AggregateLabel( Collections::AggregateCollection *coll, const Meta::LabelPtr &label );
        virtual ~AggregateLabel();

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
        Collections::AggregateCollection *m_collection;
        Meta::LabelList m_labels;
        QString m_name;
    };
} // namespace Meta

#endif // AGGREGATEMETA_H
