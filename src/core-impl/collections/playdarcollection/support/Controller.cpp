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

#include "Controller.h"

#include "Query.h"
#include "core/support/Debug.h"
#include "external/JsonQt/lib/JsonToVariant.h"
#include "external/JsonQt/lib/ParseException.h"

#include <KUrl>
#include <KIO/Job>

#include <QString>
#include <QVariant>
#include <QMap>
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
        KUrl resolveUrl( baseUrl );
        
        resolveUrl.addQueryItem( QString( "artist" ), artist );
        resolveUrl.addQueryItem( QString( "album" ), album );
        resolveUrl.addQueryItem( QString( "track" ), title );
        
        debug() << "Starting storedGetJob for " << resolveUrl.url();
        
        KJob* resolveJob = KIO::storedGet( resolveUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( resolveJob, SIGNAL( result( KJob* ) ), this, SLOT( processQuery( KJob* ) ) );
    }
    
    void
    Controller::getResults( Query* query )
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=get_results" );
        KUrl getResultsUrl( baseUrl );
        
        getResultsUrl.addQueryItem( QString( "qid" ), query->qid() );
        
        KJob* getResultsJob = KIO::storedGet( getResultsUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( getResultsJob, SIGNAL( result( KJob* ) ), query, SLOT( receiveResults( KJob* ) ) );
    }
    
    void
    Controller::getResultsLongPoll( Query* query )
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=get_results_long" );
        KUrl getResultsUrl( baseUrl );
        
        getResultsUrl.addQueryItem( QString( "qid" ), query->qid() );
        
        KJob* getResultsJob = KIO::storedGet( getResultsUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( getResultsJob, SIGNAL( result( KJob* ) ), query, SLOT( receiveResults( KJob* ) ) );
    }
    
    KUrl
    Controller::urlForSid( const QString &sid ) const
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/sid/" );
        KUrl playableUrl( baseUrl );
        
        playableUrl.addPath( sid );
        
        return playableUrl;
    }
    
    void
    Controller::status()
    {
        DEBUG_BLOCK
        
        const QString baseUrl( "http://localhost:60210/api/?method=stat" );
        KUrl statusUrl( baseUrl );
        
        KJob* statusJob = KIO::storedGet( statusUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( statusJob, SIGNAL( result( KJob* ) ), this, SLOT( processStatus( KJob* ) ) );
    }
    
    void
    Controller::processStatus( KJob *statusJob )
    {
        DEBUG_BLOCK
        
        if( statusJob->error() != 0 ) {
            debug() << "Error getting status from Playdar";
            emit playdarError( Playdar::Controller::ErrorState( ExternalError ) );
            return;
        }
        
        debug() << "Processing received JSON data...";
        KIO::StoredTransferJob* storedStatusJob = static_cast<KIO::StoredTransferJob*>( statusJob );
        QString statusJsonData = storedStatusJob->data();
        debug() << "Data received: " << statusJsonData;
        
        QVariant parsedStatusVariant;
        try
        {
            parsedStatusVariant = JsonQt::JsonToVariant::parse( statusJsonData );
        }
        catch( JsonQt::ParseException exception )
        {
            debug() << "JSonQt Exception: "
                    << "got: " << exception.got()
                    << ", expected: " << exception.expected()
                    << ", remaining: " << exception.remaining();
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
        QString queryJsonData = storedQueryJob->data();
        debug() << "Data received: " << queryJsonData;
        
        QVariant parsedQueryVariant;
        try
        {
            parsedQueryVariant = JsonQt::JsonToVariant::parse( queryJsonData );
        }
        catch( JsonQt::ParseException exception )
        {
            debug() << "JSonQt Exception: "
            << "got: " << exception.got()
            << ", expected: " << exception.expected()
            << ", remaining: " << exception.remaining();
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
        
        connect( query, SIGNAL( playdarError( Playdar::Controller::ErrorState ) ),
                 this, SIGNAL( playdarError( Playdar::Controller::ErrorState ) ) );
    }
}