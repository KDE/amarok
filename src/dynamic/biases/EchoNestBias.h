/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "Bias.h"
#include "BiasFactory.h"
#include "TagMatchBias.h"

#include <QMutex>
#include <QNetworkReply>

namespace KIO {
    class StoredTransferJob;
}

class KJob;

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
            virtual void newSimilarArtistQuery();
            virtual void newArtistIdQuery( const QString &artist );

            void artistIdQueryDone( KJob* );
            void similarArtistQueryDone( KJob* );
            virtual void updateFinished();

            void selectionChanged( int );

        private:
            /** Returns the artists we should lookup */
            QStringList currentArtists( int position, const Meta::TrackList& playlist ) const;
            static KUrl createUrl( QString method, QMultiMap< QString, QString > params );
            static QString nameForMatch( MatchType match );
            static MatchType matchForName( const QString &name );

            /** The artist we are currently quering. */
            mutable QStringList m_currentArtists;
            mutable QMap< KIO::StoredTransferJob*, QString> m_artistNameQueries;
            mutable KIO::StoredTransferJob* m_artistSuggestedQuery;

            MatchType m_match;

            mutable QMutex m_mutex; // mutex protecting all of the below structures
            mutable QMap< QString, QString > m_artistIds;
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
