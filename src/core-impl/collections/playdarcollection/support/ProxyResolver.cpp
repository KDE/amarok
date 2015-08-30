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

#include "ProxyResolver.h"

#include "Controller.h"
#include "../PlaydarCollection.h"
#include "../PlaydarMeta.h"
#include "core-impl/meta/proxy/MetaProxy.h"

#include <QUrl>

#include <QObject>

Playdar::ProxyResolver::ProxyResolver( Collections::PlaydarCollection *collection,
                                       const QUrl &url, MetaProxy::TrackPtr track )
    : m_collection( collection )
    , m_proxyTrack( track )
    , m_controller( new Playdar::Controller( true ) )
    , m_query()
{
    connect( m_controller, SIGNAL(playdarError(Playdar::Controller::ErrorState)),
             this, SLOT(slotPlaydarError(Playdar::Controller::ErrorState)) );
    connect( m_controller, SIGNAL(queryReady(Playdar::Query*)),
             this, SLOT(collectQuery(Playdar::Query*)) );
    m_controller->resolve( QUrlQuery(url).queryItemValue( "artist" ),
                           QUrlQuery(url).queryItemValue( "album" ),
                           QUrlQuery(url).queryItemValue( "title" ) );
}

Playdar::ProxyResolver::~ProxyResolver()
{
    delete m_query;
    delete m_controller;
}

void
Playdar::ProxyResolver::slotPlaydarError( Playdar::Controller::ErrorState error )
{
    emit playdarError( error );
    this->deleteLater();
}

void
Playdar::ProxyResolver::collectQuery( Playdar::Query *query )
{
    m_query = query;
    connect( m_query, SIGNAL(querySolved(Meta::PlaydarTrackPtr)),
             this, SLOT(collectSolution(Meta::PlaydarTrackPtr)) );
    connect( m_query, SIGNAL(queryDone(Playdar::Query*,Meta::PlaydarTrackList)),
             this, SLOT(slotQueryDone(Playdar::Query*,Meta::PlaydarTrackList)) );
}

void
Playdar::ProxyResolver::collectSolution( Meta::PlaydarTrackPtr track )
{
    if( !m_proxyTrack->isPlayable() )
    {
        Meta::TrackPtr realTrack;
        
        if( !m_collection.isNull() )
        {
            track->addToCollection( m_collection );
            realTrack = m_collection->trackForUrl( QUrl(track->uidUrl()) );
        }
        else
            realTrack = Meta::TrackPtr::staticCast( track );
        
        m_proxyTrack->updateTrack( realTrack );
    }
}

void
Playdar::ProxyResolver::slotQueryDone( Playdar::Query* query, const Meta::PlaydarTrackList& tracks )
{
    Q_UNUSED( query );
    Q_UNUSED( tracks );
    this->deleteLater();
}

