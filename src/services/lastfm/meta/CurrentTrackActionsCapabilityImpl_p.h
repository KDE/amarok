/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AMAROK_CURRENTTRACKACTIONSCAPABILITYIMPL_P_H
#define AMAROK_CURRENTTRACKACTIONSCAPABILITYIMPL_P_H


#include "meta/capabilities/CurrentTrackActionsCapability.h"

#include "Debug.h"

class CurrentTrackActionsCapabilityImpl : public Meta::CurrentTrackActionsCapability
{
    Q_OBJECT
    public:
        CurrentTrackActionsCapabilityImpl( LastFm::Track *track )
            : Meta::CurrentTrackActionsCapability()
            , m_track( track )
            {}

        virtual ~CurrentTrackActionsCapabilityImpl() {};

        virtual QList< QAction * > customActions() const {
            return m_track->nowPlayingActions();
        }

    private:
        KSharedPtr<LastFm::Track> m_track;
};



#endif
