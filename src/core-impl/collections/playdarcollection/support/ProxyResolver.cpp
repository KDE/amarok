/****************************************************************************************
* Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com                              *
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

#include "ProxyResolver.h"

#include "Controller.h"
#include "../PlaydarCollection.h"
#include "../PlaydarMeta.h"
#include "core-impl/meta/proxy/MetaProxy.h"

#include <KUrl>

#include <QObject>

Playdar::ProxyResolver::ProxyResolver( Collections::PlaydarCollection *collection,
                                       const KUrl &url, MetaProxy::TrackPtr track )
    : m_collection( collection )
    , m_proxyTrack( track )
    , m_controller( new Playdar::Controller )
    , m_query()
{
    connect( m_controller, SIGNAL( playdarError( Playdar::Controller::ErrorState ) ),
             this, SLOT( slotPlaydarError( Playdar::Controller::ErrorState ) ) );
    connect( m_controller, SIGNAL( queryReady( Playdar::Query* ) ),
             this, SLOT( collectQuery( Playdar::Query* ) ) );
    m_controller->resolve( url.queryItem( "artist" ),
                           url.queryItem( "album" ),
                           url.queryItem( "title" ) );
}

Playdar::ProxyResolver::~ProxyResolver()
{
    
}

void
Playdar::ProxyResolver::slotPlaydarError( Playdar::Controller::ErrorState error )
{
    m_proxyTrack->updateTrack( Meta::TrackPtr( 0 ) );
    emit playdarError( error );
}

void
Playdar::ProxyResolver::collectQuery( Playdar::Query *query )
{
    m_query = query;
    connect( m_query, SIGNAL( querySolved( Meta::PlaydarTrackPtr ) ),
             this, SLOT( collectSolution( Meta::PlaydarTrackPtr ) ) );
}

void
Playdar::ProxyResolver::collectSolution( Meta::PlaydarTrackPtr track )
{
    if( !m_collection.isNull() )
        track->addToCollection( m_collection );
    
    m_proxyTrack->updateTrack( Meta::TrackPtr::staticCast( track ) );
    
    deleteLater();
}
