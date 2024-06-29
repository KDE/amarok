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

#include <QObject>
#include <QUrl>
#include <QUrlQuery>

Playdar::ProxyResolver::ProxyResolver( Collections::PlaydarCollection *collection,
                                       const QUrl &url, const MetaProxy::TrackPtr &track )
    : m_collection( collection )
    , m_proxyTrack( track )
    , m_controller( new Playdar::Controller( true ) )
    , m_query()
{
    connect( m_controller, &Playdar::Controller::playdarError,
             this, &Playdar::ProxyResolver::slotPlaydarError );
    connect( m_controller, &Playdar::Controller::queryReady,
             this, &Playdar::ProxyResolver::collectQuery );
    m_controller->resolve( QUrlQuery(url).queryItemValue( QStringLiteral("artist") ),
                           QUrlQuery(url).queryItemValue( QStringLiteral("album") ),
                           QUrlQuery(url).queryItemValue( QStringLiteral("title") ) );
}

Playdar::ProxyResolver::~ProxyResolver()
{
    delete m_query;
    delete m_controller;
}

void
Playdar::ProxyResolver::slotPlaydarError( Playdar::Controller::ErrorState error )
{
    Q_EMIT playdarError( error );
    this->deleteLater();
}

void
Playdar::ProxyResolver::collectQuery( Playdar::Query *query )
{
    m_query = query;
    connect( m_query, &Playdar::Query::querySolved,
             this, &Playdar::ProxyResolver::collectSolution );
    connect( m_query, &Playdar::Query::queryDone,
             this, &Playdar::ProxyResolver::slotQueryDone );
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

