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

#ifndef PLAYDAR_PROXY_RESOLVER_H
#define PLAYDAR_PROXY_RESOLVER_H

#include "Controller.h"
#include "../PlaydarCollection.h"
#include "../PlaydarMeta.h"
#include "core-impl/meta/proxy/MetaProxy.h"

#include <QObject>

class QUrl;

namespace Playdar
{
    /**
     * ProxyResolver takes a MetaProxy::Track and a playdar:// url,
     * and updates the Track with a PlaydarTrack if we can find it.
     */
    class ProxyResolver : public QObject
    {
        Q_OBJECT
        
        public:
            ProxyResolver( Collections::PlaydarCollection *collection,
                           const QUrl &url, const MetaProxy::TrackPtr &track );
            ~ProxyResolver();
        
        Q_SIGNALS:
            void playdarError( Playdar::Controller::ErrorState );
        
        private Q_SLOTS:
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            void collectQuery( Playdar::Query *query );
            void collectSolution( Meta::PlaydarTrackPtr track );
            void slotQueryDone( Playdar::Query *query, const Meta::PlaydarTrackList & tracks );
            
        private:
            QPointer< Collections::PlaydarCollection > m_collection;
            MetaProxy::TrackPtr m_proxyTrack;
            Playdar::Controller* m_controller;
            Playdar::Query* m_query;
    };
}

#endif
