/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#define DEBUG_PREFIX "Playdar::Controller"

#include "Controller.h"

#include "Query.h"
#include "core/support/Debug.h"

#include <qjson/parser.h>

#include <KIO/Job>

#include <QMap>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantMap>

namespace Playdar {
    
    Controller::Controller( bool queriesShouldWaitForSolutions )
        : m_errorState( ErrorState( NoError ) )
        , m_queriesShouldWaitForSolutions( queriesShouldWaitForSolutions )
        
    {
        DEBUG_BLOCK
    }
    
    Controller::~Controller()
    {
        DEBUG_BLOCK
    }
    
    void
    Controller::resolve( const QString &artist, const QString &album, const QString &title )
    {
        DEBUG_BLOCK
        
        debug() << "Querying playdar for artist name = " << artist
                << ", album name = " << album << ", and track title = " << title;
        
        const QString baseUrl( "http://localhost:60210/api/?method=resolve" );
        QUrl resolveUrl( baseUrl );
        QUrlQuery query( resolveUrl );
        query.addQueryItem( QString( "artist" ), artist );
        query.addQueryItem( QString( "album" ), album );
        query.addQueryItem( QString( "track" ), title );
        resolveUrl.setQuery( query );
        
        debug() << "Starting storedGetJob for " << resolveUrl.url();
        
        KJob* resolveJob = KIO::storedGet( resolveUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( resolveJob, &KJob::result, this, &Controller::processQuery );
    }
    
    void
    Controller::getResults( Query* query )
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=get_results" );
        QUrl getResultsUrl( baseUrl );
        QUrlQuery q( getResultsUrl );

        q.addQueryItem( QString( "qid" ), query->qid() );
        getResultsUrl.setQuery( q );

        KJob* getResultsJob = KIO::storedGet( getResultsUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( getResultsJob, &KJob::result, query, &Query::receiveResults );
    }
    
    void
    Controller::getResultsLongPoll( Query* query )
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=get_results_long" );
        QUrl getResultsUrl( baseUrl );
        QUrlQuery q( getResultsUrl );

        q.addQueryItem( QString( "qid" ), query->qid() );
        getResultsUrl.setQuery( q );

        KJob* getResultsJob = KIO::storedGet( getResultsUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( getResultsJob, &KJob::result, query, &Query::receiveResults );
    }
    
    QUrl
    Controller::urlForSid( const QString &sid ) const
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/sid/" );
        QUrl playableUrl( baseUrl );
        
        playableUrl = playableUrl.adjusted(QUrl::StripTrailingSlash);
        playableUrl.setPath(playableUrl.path() + '/' + ( sid ));
        
        return playableUrl;
    }
    
    void
    Controller::status()
    {
        // DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=stat" );
        QUrl statusUrl( baseUrl );
        
        KJob* statusJob = KIO::storedGet( statusUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( statusJob, &KJob::result, this, &Controller::processStatus );
    }
    
    void
    Controller::processStatus( KJob *statusJob )
    {
        if( statusJob->error() != 0 ) {
            // debug() << "Error getting status from Playdar";
            emit playdarError( Playdar::Controller::ErrorState( ExternalError ) );
            return;
        }
        
        debug() << "Processing received JSON data...";
        KIO::StoredTransferJob* storedStatusJob = static_cast<KIO::StoredTransferJob*>( statusJob );
        
        QVariant parsedStatusVariant;
        
        QJson::Parser parser;
        bool ok;
        parsedStatusVariant = parser.parse( storedStatusJob->data(),&ok );
        if ( !ok )
        {
            debug() << "Error parsing JSON Data";
        }
        QVariantMap parsedStatus = parsedStatusVariant.toMap();
        
        if( !parsedStatus.contains("name") )
        {
            debug() << "Expected a service name from Playdar, received none";
            emit playdarError( Playdar::Controller::ErrorState( MissingServiceName ) );
            return;
        }
        if( parsedStatus.value("name") != QString( "playdar" ) )
        {
            debug() << "Expected Playdar, got response from some other service";
            emit playdarError( Playdar::Controller::ErrorState( WrongServiceName ) );
            return;
        }
        
        debug() << "All good! Emitting playdarReady()";
        emit playdarReady();
    }
    
    void
    Controller::processQuery( KJob *queryJob )
    {
        DEBUG_BLOCK
        
        if( queryJob->error() != 0 )
        {
            debug() << "Error getting qid from Playdar";
            emit playdarError( Playdar::Controller::ErrorState( ExternalError ) );
            return;
        }
        
        debug() << "Processing received JSON data...";
        KIO::StoredTransferJob* storedQueryJob =
            static_cast<KIO::StoredTransferJob*>( queryJob );
        
        QVariant parsedQueryVariant;
        QJson::Parser parser;
        bool ok;
        parsedQueryVariant = parser.parse( storedQueryJob->data(),&ok );
        if ( !ok )
        {
            debug() << "Error parsing JSON Data";
        }
        
        QVariantMap parsedQuery = parsedQueryVariant.toMap();
        if( !parsedQuery.contains( "qid" ) )
        {
            debug() << "Expected qid in Playdar's response, but didn't get it";
            emit playdarError( Playdar::Controller::ErrorState( MissingQid ) );
            return;
        }
        
        Query* query = new Query( parsedQuery.value( "qid" ).toString(), this, m_queriesShouldWaitForSolutions );
        
        debug() << "All good! Emitting queryReady( Playdar::Query* )...";
        emit queryReady( query );
        
        connect( query, &Query::playdarError, this, &Controller::playdarError );
    }
}
