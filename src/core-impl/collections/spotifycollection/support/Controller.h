/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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
#ifndef PLAYDAR_CONTROLLER_H
#define PLAYDAR_CONTROLLER_H

#include <QObject>
#include <QWeakPointer>

class KUrl;
class KJob;

class QString;

/**
 * @namespace Spotify contains the implementation of the Spotify API
 * in Amarok, so far as it is not specific to collection or context use.
 */
namespace Spotify
{
    class Query;
    class ProxyResolver;
    
    /**
     * This class provides a basic interface to Spotify's resolution
     * functionality. A user should initialize a Controller, wait for
     * spotifyReady(), and proceed to use resolve( artist, album, title )
     * as much as they'd like. Unless some error occurs, queryReady( QueryPtr )
     * will provide a QueryPtr for each call to resolve(). Results will
     * be provided by the appropriate Query as they become available, and
     * the friendly relationship between Controller and Query ensures that
     * results are properly matched with Querys.
     */
    class Controller : public QThread
    {
        Q_OBJECT
        public:
            /**
             * We invoke the private function status() here, return immediately,
             * and will emit spotifyReady() once things are actually set up.
             * @param queriesShouldWaitForSolutions
             *        If true, Spotify::Querys created by this controller will
             *        only use getResultsLongPoll instead of first using getResults.
             */
            Controller( bool queriesShouldWaitForSolutions = false );
            /**
             * Controllers don't hold on to anything, so the deconstructor does nothing.
             */
            ~Controller();
            
            /** 
            * Asks Spotify for status information, which eventually results in
            * the emission of spotifyReady() if it does or error() otherwise.
            */
            void status();
            
            /**
             * Asks Spotify to resolve a query, which eventually results in the
             * emission of queryReady() if okay, or error() if something goes wrong.
             * @param artist Name of artist to search for.
             * @param album Name of album (by artist) to search for.
             * @param title Name of track on album to search for.
             * NOTE: Currently, for a query to have any chance of being solved,
             *       both artist and title must be non-empty!
             */
            void resolve( const QString &artist,
                          const QString &album,
                          const QString &title );
            
            /** These errors are used when Spotify::Controller emits error(). */
            enum ErrorState
            {
                /** Nothing bad happened yet! */
                NoError,
                /** Indicates an error from KIO or the Spotify service itself. */
                ExternalError,
                /** A request for status revealed a service claiming to not be Spotify. */
                WrongServiceName,
                /** A service asked for status didn't identify itself. */
                MissingServiceName,
                /** No "qid" field was found in a response to resolve() or getResults(). */
                MissingQid,
                /** Results were delivered to the wrong query */
                WrongQid,
                /** A response to getResults() didn't include a "results" array. */
                MissingResults
            };

        signals:
            /**
            * Emitted once after construction, as long as some service responds
            * from where we think Spotify is, and identifies itself as "spotify".
            * Clients shouldn't try to resolve things until they get this signal.
            */
            void spotifyReady();
            /**
            * Emitted after resolve() is invoked, as long as Spotify's response
            * includes a "qid" field. If the client object's looking for results,
            * the Query's where they'll be when Spotify's got back to us. The
            * controller doesn't keep any QueryPtrs around, so interested clients
            * should keep a QueryList for any QueryPtrs they'd like to have.
            */
            void queryReady( Spotify::Query* );
            /**
            * Emitted after something unfortunate happens,
            * along with the (hopefully) appropriate ErrorType
            */
            void spotifyError( Spotify::Controller::ErrorState );
        
        public:
            /**
             * NOTE: Querys handle invoking these on their own.
             */
            /**
             * Asks Spotify for the state of the query with @p qid.
             * If all goes well, resultsReady() is emitted, error() if something goes wrong.
             * @param query The query to get results for
             */
            void getResults( Spotify::Query* query );
            /**
             * Like getResults(), but Spotify will wait to respond until the query
             * has either been solved or enough time passes, (usually about 4000ms),
             * that the query would have been solved it was possible.
             * @param query The query to get results for
             */
            void getResultsLongPoll( Spotify::Query* query );

            /**
             * Makes a naive attempt to produce a KUrl that points to
             * a playable location of a query result.
             * @param sid The sid of the result desired.
             */
            KUrl urlForSid( const QString &sid ) const;
            
        private Q_SLOTS:
            void processStatus( KJob* statusJob );
            void processQuery( KJob* queryJob );
            
        private:
            ErrorState m_errorState;
            bool m_queriesShouldWaitForSolutions;
    };
}

#endif
