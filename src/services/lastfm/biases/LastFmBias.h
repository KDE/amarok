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

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"
#include "dynamic/biases/TagMatchBias.h"

#include <QMutex>
#include <QNetworkReply>

class KJob;

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
                SimilarAlbum
            };

            LastFmBias();
            LastFmBias( QXmlStreamReader *reader );

            void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual Dynamic::TrackSet matchingTracks( int position,
                                                      const Meta::TrackList& playlist,
                                                      int contextCount,
                                                      Dynamic::TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;


            MatchType match() const;
            void setMatch( MatchType value );

        public slots:
            virtual void invalidate();

        private slots:
            virtual void newQuery();
            virtual void newSimilarQuery();

            void similarArtistQueryDone();
            void similarTrackQueryDone();

            void selectionChanged( int );

        private:
            void saveDataToFile();
            void loadFromFile();

            mutable QString m_currentArtist;
            mutable QString m_currentTrack;
            QNetworkReply* m_artistQuery;
            QNetworkReply* m_trackQuery;

            MatchType m_match;


            mutable QMutex m_mutex; // mutex protecting all of the below structures
            mutable QMap< QString, QStringList> m_similarArtistMap;
            mutable QMap< QString, QStringList> m_similarAlbumMap;
            mutable QMap< QString, TrackSet> m_tracksMap;

        private:
            Q_DISABLE_COPY(LastFmBias)
    };


    class LastFmBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
            virtual BiasPtr createBias( QXmlStreamReader *reader );
    };

}

#endif
