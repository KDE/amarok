/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *                                                                                      *
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

#include "Query.h"

#include "Controller.h"
#include "core/meta/Meta.h"
#include "../PlaydarMeta.h"
#include "core/support/Debug.h"

#include <qjson/parser.h>

#include <QUrl>
#include <KIO/Job>

#include <QString>
#include <QVariant>
#include <QMap>
#include <QVariantMap>

namespace Playdar
{
    Query::Query( const QString &qid,
                  Playdar::Controller* controller,
                  bool waitForSolution )
    : m_controller( controller )
    , m_waitForSolution( waitForSolution )
    , m_qid( qid )
    , m_artist( QString( "" ) )
    , m_album( QString( "" ) )
    , m_title( QString( "" ) )
    , m_solved( false )
    , m_receivedFirstResults( false )
    , m_trackList( )
    {
        DEBUG_BLOCK
        
        if( m_waitForSolution )
        {
            m_receivedFirstResults = true;
            m_controller.data()->getResultsLongPoll( this );
        }
        else
            m_controller.data()->getResults( this );
    }
    
    Query::~Query()
    {
        DEBUG_BLOCK
        
    }
    
    QString
    Query::qid() const
    {
        DEBUG_BLOCK
        
        return m_qid;
    }
    
    QString
    Query::artist() const
    {
        DEBUG_BLOCK
        
        return m_artist;
    }
    
    QString
    Query::album() const
    {
        DEBUG_BLOCK
        
        return m_album;
    }
    
    QString
    Query::title() const
    {
        DEBUG_BLOCK
        
        return m_title;
    }
    
    bool
    Query::isSolved() const
    {
        DEBUG_BLOCK
        
        return m_solved;
    }

    Meta::PlaydarTrackList
    Query::getTrackList() const
    {
        DEBUG_BLOCK
        
        return m_trackList;
    }
    
    void
    Query::receiveResults( KJob* resultsJob)
    {
        DEBUG_BLOCK
        
        if( resultsJob->error() != 0 ) {
            debug() << "Error getting results from Playdar";
            emit playdarError( Playdar::Controller::ErrorState( 1 ) );
            return;
        }
        
        debug() << "Processing received JSON data...";
        KIO::StoredTransferJob* storedResultsJob = static_cast<KIO::StoredTransferJob*>( resultsJob );
                
        QJson::Parser parser;
        bool ok;
        QVariant parsedResultsVariant;
        parsedResultsVariant = parser.parse( storedResultsJob->data(),&ok );
        if ( !ok )
        {
            debug() << "Error parsing JSON Data";
        }
        
        QVariantMap parsedResults = parsedResultsVariant.toMap();
        if( !parsedResults.contains( "results" ) )
        {
            debug() << "Expecting results in Playdar's response, received none";
            emit playdarError( Playdar::Controller::ErrorState( 6 ) );
            return;
        }
        if( !parsedResults.contains( "qid" ) )
        {
            debug() << "Expected qid in Playdar's response, received none";
            emit playdarError( Playdar::Controller::ErrorState( 4 ) );
            return;
        }
        if( parsedResults.value( "qid" ) != m_qid )
        {
            debug() << "A query received the wrong results from Playdar...";
            emit playdarError( Playdar::Controller::ErrorState( 5 ) );
            return;
        }
        
        m_artist = parsedResults.value( "artist" ).toString();
        m_album = parsedResults.value( "album" ).toString();
        m_title = parsedResults.value( "track" ).toString();
        
        foreach( const QVariant &resultVariant, parsedResults.value( "results" ).toList() )
        {
            QVariantMap result = resultVariant.toMap();
            Meta::PlaydarTrackPtr aTrack;
            QUrl resultUrl( m_controller.data()->urlForSid( result.value( "sid" ).toString() ) );
            
            QString trackSid = result.value( "sid" ).toString();
            QString trackUrl = resultUrl.url();
            QString trackTitle = result.value( "track" ).toString();
            QString trackArtist = result.value( "artist" ).toString();
            QString trackAlbum = result.value( "album" ).toString();
            QString trackType = result.value( "mimetype" ).toString();
            QString trackSource = result.value( "source" ).toString();
            qint64 trackLengthInSeconds( result.value( "duration" ).toInt() );
            aTrack = new Meta::PlaydarTrack
            (
                trackSid,
                trackUrl,
                trackTitle,
                trackArtist,
                trackAlbum,
                trackType,
                result.value( "score" ).toDouble() * 100,
                ( trackLengthInSeconds * 1000 ), //convert s to ms
                result.value( "bitrate" ).toInt(),
                result.value( "size" ).toInt(),
                trackSource
            );
            
            if( !m_solved && aTrack->score() >= 1.00 )
            {
                m_solved = true;
                m_trackList.prepend( aTrack );
                emit querySolved( aTrack );
                
                if( m_waitForSolution )
                {
                    emit queryDone( this, m_trackList );
                    return;
                }
            }
            else
            {
                m_trackList.append( aTrack );
            }
            emit newTrackAdded( aTrack );
        }
        
        if( m_receivedFirstResults || m_solved )
        {
            m_receivedFirstResults = true;
            emit queryDone( this, m_trackList );
        }
        else
        {
            m_receivedFirstResults = true;
            m_controller.data()->getResultsLongPoll( this );
        }
    }
}
