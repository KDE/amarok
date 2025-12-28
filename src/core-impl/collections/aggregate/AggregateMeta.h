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
            ~AggregateTrack() override;

            QString name() const override;
            QString prettyName() const override;
            QString sortableName() const override;

            QUrl playableUrl() const override;
            QString prettyUrl() const override;
            QString uidUrl() const override;

            /**
             * Return a comma separated list of reasons why constituent
             * tracks are unplayable or an empty string if any of the tracks is playable
             */
            QString notPlayableReason() const override;

            Meta::AlbumPtr album() const override;
            Meta::ArtistPtr artist() const override;
            Meta::ComposerPtr composer() const override;
            Meta::GenrePtr genre() const override;
            Meta::YearPtr year() const override;

            QString comment() const override;
            qreal   bpm() const override;

            void finishedPlaying( double playedFraction ) override;

            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;
            QDateTime createDate() const override;
            int trackNumber() const override;
            int discNumber() const override;
            QString type() const override;
            qreal replayGain( ReplayGainTag mode ) const override;

            Collections::Collection* collection() const override;

            bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
            Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

            void addLabel( const QString &label ) override;
            void addLabel( const Meta::LabelPtr &label ) override;
            void removeLabel( const Meta::LabelPtr &label ) override;
            Meta::LabelList labels() const override;

            TrackEditorPtr editor() override;
            StatisticsPtr statistics() override;

            // Meta::Statistics methods:
            double score() const override;
            void setScore( double newScore ) override;
            int rating() const override;
            void setRating( int newRating ) override;
            QDateTime firstPlayed() const override;
            void setFirstPlayed( const QDateTime &date ) override;
            QDateTime lastPlayed() const override;
            void setLastPlayed( const QDateTime &date ) override;
            int playCount() const override;
            void setPlayCount( int newPlayCount ) override;

            void add( const Meta::TrackPtr &track );

        protected:
            using Observer::metadataChanged;
            void metadataChanged( const Meta::TrackPtr &track ) override;

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
        ~AggregateAlbum() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        Meta::TrackList tracks() override;
        Meta::ArtistPtr albumArtist() const override;
        bool isCompilation() const override;
        bool hasAlbumArtist() const override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        void add( const Meta::AlbumPtr &album );

        /** returns true if the album has a cover set */
        bool hasImage( int size = 0 ) const override;
        /** returns the cover of the album */
        QImage image( int size = 0 ) const override;
        /** returns the image location on disk */
        QUrl imageLocation( int size = 0 ) const override;
        /** returns the cover of the album with a nice border around it*/
        virtual QPixmap imageWithBorder( int size = 0, int borderWidth = 5 );
        /** Returns true if it is possible to update the cover of the album */
        bool canUpdateImage() const override;
        /** updates the cover of the album */
        void setImage( const QImage &image ) override;
        void removeImage() override;
        /** don't automatically fetch artwork */
        void setSuppressImageAutoFetch( const bool suppress ) override;
        /** should automatic artwork retrieval be suppressed? */
        bool suppressImageAutoFetch() const override;

        protected:
        using Observer::metadataChanged;
        void metadataChanged(const  Meta::AlbumPtr &album ) override;

        private:
        Collections::AggregateCollection *m_collection;
        Meta::AlbumList m_albums;
        QString m_name;
        Meta::ArtistPtr m_albumArtist;
    };

    class AMAROK_EXPORT AggregateArtist : public Meta::Artist, private Meta::Observer
    {
        public:
        AggregateArtist( Collections::AggregateCollection *coll, const Meta::ArtistPtr &artist );
        ~AggregateArtist() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        Meta::TrackList tracks() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        void add( const Meta::ArtistPtr &artist );

        protected:
        using Observer::metadataChanged;
        void metadataChanged( const Meta::ArtistPtr &artist ) override;

        private:
        Collections::AggregateCollection *m_collection;
        Meta::ArtistList m_artists;
        QString m_name;
    };

    class AMAROK_EXPORT AggregateGenre : public Meta::Genre, private Meta::Observer
    {
        public:
        AggregateGenre( Collections::AggregateCollection *coll, const Meta::GenrePtr &genre );
        ~AggregateGenre() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        Meta::TrackList tracks() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        void add( const Meta::GenrePtr &genre );

        protected:
        using Observer::metadataChanged;
        void metadataChanged( const Meta::GenrePtr &genre ) override;

        private:
        Collections::AggregateCollection *m_collection;
        Meta::GenreList m_genres;
        QString m_name;
    };

    class AMAROK_EXPORT AggregateComposer : public Meta::Composer, private Meta::Observer
    {
        public:
        AggregateComposer(Collections::AggregateCollection *coll, const ComposerPtr &composer );
        ~AggregateComposer() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        Meta::TrackList tracks() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        void add( const Meta::ComposerPtr &composer );

        protected:
        using Observer::metadataChanged;
        void metadataChanged( const Meta::ComposerPtr &composer ) override;

        private:
        Collections::AggregateCollection *m_collection;
        Meta::ComposerList m_composers;
        QString m_name;
    };

    class AMAROK_EXPORT AggreagateYear : public Meta::Year, private Meta::Observer
    {
        public:
        AggreagateYear( Collections::AggregateCollection * coll, const Meta::YearPtr &year );
        ~AggreagateYear() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        Meta::TrackList tracks() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        /**
          * adds another Meta::Year instance to be proxied.
          */
        void add( const Meta::YearPtr &year );

        protected:
        using Observer::metadataChanged;
        void metadataChanged( const Meta::YearPtr &year ) override;

        private:
        Collections::AggregateCollection *m_collection;
        Meta::YearList m_years;
        QString m_name;

    };

    class AMAROK_EXPORT AggregateLabel : public Meta::Label
    {
    public:
        AggregateLabel( Collections::AggregateCollection *coll, const Meta::LabelPtr &label );
        ~AggregateLabel() override;

        QString name() const override;
        QString prettyName() const override;
        QString sortableName() const override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

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
