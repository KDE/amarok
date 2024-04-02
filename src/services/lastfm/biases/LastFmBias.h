/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef LASTFM_BIAS_H
#define LASTFM_BIAS_H

#include "dynamic/biases/TagMatchBias.h"

#include <QRecursiveMutex>

namespace Dynamic
{

    /** This is a bias which adds the suggested songs to the playlist. */
    class LastFmBias : public SimpleMatchBias
    {
        Q_OBJECT

        public:

            /** The tracks that are used for the matching */
            enum MatchType
            {
                SimilarArtist,
                SimilarTrack
            };

            LastFmBias();
            ~LastFmBias() override;

            void fromXml( QXmlStreamReader *reader ) override;
            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;

            Dynamic::TrackSet matchingTracks( const Meta::TrackList& playlist,
                                              int contextCount, int finalCount,
                                              const Dynamic::TrackCollectionPtr& universe ) const override;

            bool trackMatches( int position,
                               const Meta::TrackList& playlist,
                               int contextCount ) const override;


            MatchType match() const;
            void setMatch( MatchType value );

        public Q_SLOTS:
            void invalidate() override;

        private Q_SLOTS:
            void newQuery() override;
            virtual void newSimilarQuery();

            void similarArtistQueryDone();
            void similarTrackQueryDone();
            void queryFailed( const char *message );

            void setMatchTypeArtist( bool matchArtist );

        private:
            /** The pair is used for the tracks */
            typedef QPair<QString, QString> TitleArtistPair;

            static QString nameForMatch( MatchType match );
            static MatchType matchForName( const QString &name );

            void saveDataToFile() const;

            void readSimilarArtists( QXmlStreamReader *reader );
            TitleArtistPair readTrack( QXmlStreamReader *reader );
            void readSimilarTracks( QXmlStreamReader *reader );
            void loadDataFromFile();

            mutable QString m_currentArtist;
            mutable QString m_currentTrack;

            MatchType m_match;

            mutable QRecursiveMutex m_mutex; // mutex protecting all of the below structures
            mutable QMap< QString, QStringList> m_similarArtistMap;
            mutable QMap< TitleArtistPair, QList<TitleArtistPair> > m_similarTrackMap;
            mutable QMap< QString, TrackSet> m_tracksMap; // for artist AND album

        private:
            Q_DISABLE_COPY(LastFmBias)
    };


    class LastFmBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            BiasPtr createBias() override;
    };

}

#endif
