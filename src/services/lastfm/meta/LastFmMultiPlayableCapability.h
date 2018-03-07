/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
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

#ifndef AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H
#define AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H

#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "LastFmMeta.h"
#include "core/meta/forward_declarations.h"
#include "core/capabilities/MultiPlayableCapability.h"

#include <KLocalizedString>

#include <Track.h>
#include <RadioStation.h>
#include <RadioTuner.h>
#include <ws.h>

class LastFmMultiPlayableCapability : public Capabilities::MultiPlayableCapability
{
    Q_OBJECT

    public:
        LastFmMultiPlayableCapability( LastFm::Track *track );
        virtual ~LastFmMultiPlayableCapability();

        // Capabilities::MultiPlayableCapability methods
        virtual void fetchFirst();
        virtual void fetchNext();

    private Q_SLOTS:
        void slotTrackPlaying( const Meta::TrackPtr &track );
        void slotNewTrackAvailable();
        void skip();
        void error( lastfm::ws::Error e );

    private:
        QUrl m_url;
        LastFm::TrackPtr m_track;

        lastfm::Track m_currentTrack;
        lastfm::RadioTuner *m_tuner;
};

#endif
