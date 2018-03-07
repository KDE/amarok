/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011, 2013 Ralf Engels <ralf-engels@gmx.de>                      *
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

#ifndef ECHO_NEST_BIAS_H
#define ECHO_NEST_BIAS_H

#include "dynamic/biases/TagMatchBias.h"

#include <QMutex>

namespace KIO {
    class StoredTransferJob;
}
class KJob;
class QUrl;

namespace Dynamic
{
    /**
     * This is a bias which adds the suggested songs to the playlist.
     */
    class EchoNestBias : public SimpleMatchBias
    {
        Q_OBJECT

        public:

            /** The tracks that are used for the matching */
            enum MatchType
            {
                PreviousTrack,
                Playlist
            };

            EchoNestBias();
            ~EchoNestBias();

            virtual void fromXml( QXmlStreamReader *reader );
            virtual void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;
            virtual QString toString() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual Dynamic::TrackSet matchingTracks( const Meta::TrackList& playlist,
                                                      int contextCount, int finalCount,
                                                      Dynamic::TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;


            MatchType match() const;
            void setMatch( MatchType value );

        public Q_SLOTS:
            virtual void invalidate();

        private Q_SLOTS:
            virtual void newQuery();
            virtual void newSimilarArtistQuery();

            void similarArtistQueryDone( KJob* );
            virtual void updateFinished();

            void setMatchTypePlaylist( bool );

        private:
            /** Returns the artists we should lookup */
            QStringList currentArtists( int position, const Meta::TrackList& playlist ) const;
            static QUrl createUrl( QString method, QMultiMap< QString, QString > params );
            static QString nameForMatch( MatchType match );
            static MatchType matchForName( const QString &name );

            /** Returns the key used for m_tracksMap */
            static QString tracksMapKey( QStringList artists );

            void saveDataToFile() const;

            void readSimilarArtists( QXmlStreamReader *reader );
            void loadDataFromFile();

            /** The artist we are currently querying. */
            mutable QStringList m_currentArtists;
            mutable QMap< KIO::StoredTransferJob*, QString> m_artistNameQueries;
            mutable KIO::StoredTransferJob* m_artistSuggestedQuery;

            MatchType m_match;

            mutable QMutex m_mutex; // mutex protecting all of the below structures
            mutable QMap< QString, QStringList> m_similarArtistMap;
            mutable QMap< QString, TrackSet> m_tracksMap;

        private:
            Q_DISABLE_COPY(EchoNestBias)
    };


    class AMAROK_EXPORT EchoNestBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
    };
}

#endif
