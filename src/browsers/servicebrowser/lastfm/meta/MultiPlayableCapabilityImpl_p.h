/*
   Copyright (C) 2008 Shane King <kde@dontletsstart.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H
#define AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H

#include "Meta.h"
#include "meta/MultiPlayableCapability.h"

class MultiPlayableCapabilityImpl : public Meta::MultiPlayableCapability, public Meta::Observer
{
    Q_OBJECT
    public:
        MultiPlayableCapabilityImpl( LastFm::Track *track )
            : Meta::MultiPlayableCapability()
            , m_url( track->playableUrl() )
            , m_track( track )
        {
            Meta::TrackPtr trackptr( track );
            subscribeTo( trackptr );
        }

        virtual ~MultiPlayableCapabilityImpl() 
        {}

        virtual void fetchFirst() { m_track->playCurrent(); }
        virtual void fetchNext() { m_track->playNext(); }

        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::Track *track )
        {
            KUrl url = track->playableUrl();
            if( url.isEmpty() || url != m_url ) // always should let empty url through, since otherwise we swallow an error getting first track
            {
                m_url = url;
                emit playableUrlFetched( url );
            }
        }

    private:
        KUrl m_url;
        LastFm::TrackPtr m_track;
};

#endif
