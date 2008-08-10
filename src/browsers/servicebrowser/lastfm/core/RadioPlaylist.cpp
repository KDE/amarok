/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "RadioPlaylist.h"
#include "Radio.h"
#include "MooseCommon.h"
#include "WebService/GetXspfPlaylistRequest.h"
#include "logger.h"
#include "LastFmSettings.h"

#include <QDomDocument>

#include <KLocale>

// Re=request XSPF when queue falls below this size
static const int k_minQueueSize = 2;

RadioPlaylist::RadioPlaylist() :
    m_currentRequest( 0 ),
    m_allXspfRetrieved( false ),        
    m_requestingPlaylist( false )
{
}


RadioPlaylist::~RadioPlaylist()
{
}


void
RadioPlaylist::setSession( const QString& session )
{
    // TODO: figure out why there's a clear here. It causes a resume to
    // refetch xspf unnecessarily.
    clear(); 
    m_session = session;
    m_allXspfRetrieved = false;
    
    if ( m_trackQueue.size() < k_minQueueSize && !m_requestingPlaylist )
    {
        requestPlaylistChunk();
    }
}


void
RadioPlaylist::setXspf( const QByteArray& xspf )
{
    clear();
    m_xspf = xspf;
    parseXspf( m_xspf );
    m_allXspfRetrieved = true;
}


RadioPlaylist::Type
RadioPlaylist::type() const
{
    return m_session.isEmpty() ? Type_Playlist : Type_Station;
}


bool
RadioPlaylist::hasMore() const
{
    return !m_trackQueue.isEmpty();
}


bool
RadioPlaylist::isOutOfContent() const
{
    return m_allXspfRetrieved;
}


TrackInfo
RadioPlaylist::nextTrack()
{
    m_currentTrack = m_trackQueue.dequeue();
    
    if ( !m_allXspfRetrieved &&
         m_trackQueue.size() < k_minQueueSize &&
         !m_requestingPlaylist )
    {
        requestPlaylistChunk();
    }
    
    return m_currentTrack;
}


TrackInfo
RadioPlaylist::currentTrack()
{
    return m_currentTrack;
}


void
RadioPlaylist::clear()
{
    m_trackQueue.clear();
    m_currentTrack = TrackInfo();

    abort();
    m_allXspfRetrieved = true;

    m_session.clear();
    m_xspf.clear();
}


void
RadioPlaylist::discardRemaining()
{
    m_trackQueue.clear();
    m_currentTrack = TrackInfo();
    abort();

    if ( type() == Type_Station && The::radio().isPlaying() )
    {
        requestPlaylistChunk();
    }
}


void
RadioPlaylist::abort()
{
    if ( m_currentRequest )
    {
        m_currentRequest->abort();
    }
}

void
RadioPlaylist::requestPlaylistChunk()
{
    Q_ASSERT( !m_session.isEmpty() );

    LOGL( 4, "Requesting playlist chunk..." );

    m_currentRequest = new GetXspfPlaylistRequest( m_session, m_basePath,
        The::settings().version(), The::currentUser().isDiscovery() );
    connect( m_currentRequest,  SIGNAL( result( Request* ) ),
             this,              SLOT  ( xspfPlaylistRequestReturn( Request* ) ) );
    m_currentRequest->start();
    
    m_requestingPlaylist = true;
}


void
RadioPlaylist::xspfPlaylistRequestReturn( Request* request )
{
    m_requestingPlaylist = false;
    m_currentRequest = 0;

    if ( request->aborted() ) { return; }

    if ( request->failed() )
    {
        emit error( static_cast<RadioError>( request->resultCode() ),
                    request->errorMessage() );
        return;
    }

    QByteArray xspf = request->data();

    parseXspf( xspf );
}

void
RadioPlaylist::parseXspf( QByteArray& xspf )
{
    LOGL( 3, "XSPF to parse:\n" << xspf.constData()  );

    try
    {
        int sizeBefore = m_trackQueue.size();
        
        m_trackQueue += m_resolver.resolveTracks( xspf );
        
        LOGL( 4, "Got " << m_trackQueue.size() - sizeBefore << " tracks worth of playlist" );

        if ( m_trackQueue.size() == sizeBefore )
        {
            // Station has run out of tracks
            m_allXspfRetrieved = true;
        }

        emit playlistLoaded( m_resolver.station(), m_resolver.skipLimit() );
    }
    catch ( XspfResolver::ParseException& e )
    {
        // Seems we won't get any more xspf. Setting this will make station
        // finish properly on reaching the end.
        m_allXspfRetrieved = true;
        
        QString err = i18n( "The playlist could not be read. Error:\n\n%1", e.what() );
        emit error( Radio_BadPlaylist, err );
    }

}
