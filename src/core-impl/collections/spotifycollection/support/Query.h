/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#ifndef SPOTIFY_QUERY_
#define SPOTIFY_QUERY_

#include "../SpotifyMeta.h"

namespace Collections
{
    class SpotifyCollection;
}

class QObject;

namespace Spotify
{

class Query;
typedef KSharedPtr< Query > QueryPtr;

enum QueryErrorNumber {
    ENoError,
    EInternalError,
    ESpotifyNotConnected,
    ETimedOut
};

struct QueryError {
    QueryError(): errno( ENoError ) {}
    QueryError(const QueryErrorNumber e, const QString& msg): errno( e ), verbose( msg ) {}
    QueryErrorNumber errno;
    QString verbose;
};

class Query: public QObject, public QSharedData
{
    Q_OBJECT
    public:
        Query( Collections::SpotifyCollection* collection, const QString& qid, const QString& title = QString(), const QString& artist = QString(), const QString& album = QString(), const QString& genre = QString() );
        ~Query();

        /* @return artist of the query
         */
        QString artist() const { return m_artist; }
        /* @return title of the query
         */
        QString title() const { return m_title; }
        /* @return genre of the query
         */
        QString genre() const { return m_genre; }
        /* @return album of the query
         */
        QString album() const { return m_album; }

        /* @return qid of the query
         */
        QString qid() const { return m_qid; }


        bool isSolved() const { return m_solved; }

        void setAlbum( const QString& album ) { m_album = album; }
        void setGenre( const QString& genre ) { m_genre = genre; }
        void setArtist( const QString& artist ) { m_artist = artist; }
        void setTitle( const QString& title ) { m_title = title; }

        Meta::SpotifyTrackList& getTrackList() { return m_results; }

        /* @return the full query string
         */
        QString getFullQueryString() const;

        int error() const { return (int)m_error.errno; }
        QString errorMsg() const { return m_error.verbose; }

    public slots:
        void slotTracksAdded( const Meta::SpotifyTrackList &trackList );
        // Abort current query if timed out
        void slotAbortQuery();
        void slotTimedout();
        void setError( const Spotify::QueryError& error ) { m_error = error; }

    signals:
        void queryError( const Spotify::QueryError& error );
        void newTrackList( const Meta::SpotifyTrackList& trackList );
        void queryDone( const QString& qid );

    private:
        QString m_qid;
        QString m_title;
        QString m_artist;
        QString m_album;
        QString m_genre;

        int m_timeout;

        Collections::SpotifyCollection* m_collection;

        bool m_solved;

        Meta::SpotifyTrackList m_results;

        Spotify::QueryError m_error;
};
} // namesapce Spotify

#endif
