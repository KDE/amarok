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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>

#include <KIO/Job>
#include <KIO/StoredTransferJob>


namespace Playdar
{
    Query::Query( const QString &qid,
                  Playdar::Controller* controller,
                  bool waitForSolution )
    : m_controller( controller )
    , m_waitForSolution( waitForSolution )
    , m_qid( qid )
    , m_artist( QStringLiteral( "" ) )
    , m_album( QStringLiteral( "" ) )
    , m_title( QStringLiteral( "" ) )
    , m_solved( false )
    , m_receivedFirstResults( false )
    , m_trackList( )
    {
        DEBUG_BLOCK
        
        if( m_waitForSolution )
        {
            m_receivedFirstResults = true;
            m_controller->getResultsLongPoll( this );
        }
        else
            m_controller->getResults( this );
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
            Q_EMIT playdarError( Playdar::Controller::ErrorState( 1 ) );
            return;
        }
        
        debug() << "Processing received JSON data...";
        KIO::StoredTransferJob* storedResultsJob = static_cast<KIO::StoredTransferJob*>( resultsJob );
                
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson( storedResultsJob->data(), &err );

        if ( err.error != QJsonParseError::NoError )
            debug() << "Error parsing JSON Data:" << err.errorString();

        if( !doc.isObject() )
        {
            debug() << "Parsed Json data is not an object";
            return;
        }

        auto object = doc.object();

        if( !object.contains( QStringLiteral("results") ) )
        {
            debug() << "Expecting results in Playdar's response, received none";
            Q_EMIT playdarError( Playdar::Controller::ErrorState( 6 ) );
            return;
        }
        if( !object.contains( QStringLiteral("qid") ) )
        {
            debug() << "Expected qid in Playdar's response, received none";
            Q_EMIT playdarError( Playdar::Controller::ErrorState( 4 ) );
            return;
        }
        if( object.value( QStringLiteral("qid") ) != m_qid )
        {
            debug() << "A query received the wrong results from Playdar...";
            Q_EMIT playdarError( Playdar::Controller::ErrorState( 5 ) );
            return;
        }
        
        m_artist = object.value( QStringLiteral("artist") ).toString();
        m_album = object.value( QStringLiteral("album") ).toString();
        m_title = object.value( QStringLiteral("track") ).toString();
        
        for( const auto &resultVariant : object.value( QStringLiteral("results") ).toArray() )
        {
            auto result = resultVariant.toObject();
            Meta::PlaydarTrackPtr aTrack;
            QUrl resultUrl( m_controller->urlForSid( result.value( QStringLiteral("sid") ).toString() ) );
            
            QString trackSid = result.value( QStringLiteral("sid") ).toString();
            QString trackUrl = resultUrl.url();
            QString trackTitle = result.value( QStringLiteral("track") ).toString();
            QString trackArtist = result.value( QStringLiteral("artist") ).toString();
            QString trackAlbum = result.value( QStringLiteral("album") ).toString();
            QString trackType = result.value( QStringLiteral("mimetype") ).toString();
            QString trackSource = result.value( QStringLiteral("source") ).toString();
            qint64 trackLengthInSeconds( result.value( QStringLiteral("duration") ).toInt() );
            aTrack = new Meta::PlaydarTrack
            (
                trackSid,
                trackUrl,
                trackTitle,
                trackArtist,
                trackAlbum,
                trackType,
                result.value( QStringLiteral("score") ).toDouble() * 100,
                ( trackLengthInSeconds * 1000 ), //convert s to ms
                result.value( QStringLiteral("bitrate") ).toInt(),
                result.value( QStringLiteral("size") ).toInt(),
                trackSource
            );
            
            if( !m_solved && aTrack->score() >= 1.00 )
            {
                m_solved = true;
                m_trackList.prepend( aTrack );
                Q_EMIT querySolved( aTrack );
                
                if( m_waitForSolution )
                {
                    Q_EMIT queryDone( this, m_trackList );
                    return;
                }
            }
            else
            {
                m_trackList.append( aTrack );
            }
            Q_EMIT newTrackAdded( aTrack );
        }
        
        if( m_receivedFirstResults || m_solved )
        {
            m_receivedFirstResults = true;
            Q_EMIT queryDone( this, m_trackList );
        }
        else
        {
            m_receivedFirstResults = true;
            m_controller->getResultsLongPoll( this );
        }
    }
}
