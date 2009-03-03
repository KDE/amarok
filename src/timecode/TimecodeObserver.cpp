/***************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
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

#include "TimecodeObserver.h"

#include "Debug.h"
#include "meta/capabilities/TimecodeWriteCapability.h"

int TimecodeObserver::m_threshold = 600; //!< 600s = 10 minutes

TimecodeObserver::TimecodeObserver()    : EngineObserver ( The::engineController() )
        , m_trackTimecodeable ( false )
        , m_currentTrack ( 0 )
        , m_currPos ( 0 )
{
}

TimecodeObserver::~TimecodeObserver()
{
}

void
TimecodeObserver::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if ( currentTrack == m_currentTrack ) // no change, so do nothing
        return;

    if ( m_currentTrack ) // this is really the track _just_ played
    {
        if ( m_trackTimecodeable && m_currPos != m_currentTrack->length() && m_currentTrack->length() > m_threshold )
        {
            Meta::TimecodeWriteCapability *tcw = m_currentTrack->as<Meta::TimecodeWriteCapability>();
            if( tcw )
                tcw->writeTimecode ( m_currPos ); // save the timecode
        }
    }

    // now update to the new track
    debug() << "curent track name: " << currentTrack->prettyName();
    if ( currentTrack->hasCapabilityInterface ( Meta::Capability::WriteTimecode ) )
    {
        m_trackTimecodeable = true;
        debug() << "Track timecodeable";
    }
    m_currentTrack = currentTrack;
    m_currPos = 0;
}

void
TimecodeObserver::enginePlaybackEnded ( int finalPosition, int trackLength, EngineObserver::PlaybackEndedReason reason )
{
    Q_UNUSED ( reason )
    DEBUG_BLOCK

    if ( m_trackTimecodeable && finalPosition != trackLength && trackLength > m_threshold )
    {
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
        Meta::TimecodeWriteCapability *tcw = currentTrack->as<Meta::TimecodeWriteCapability>();
        if( tcw )
            tcw->writeTimecode ( finalPosition ); // save the timecode
    }
}

void TimecodeObserver::engineTrackPositionChanged ( long position, bool userSeek )
{
    Q_UNUSED ( userSeek )
    m_currPos = position / 1000; // <------ ARGH
    // god damn the fucking controller and
    // it's fucking schizo behavior when it
    // comes to milliseconds and seconds </rant>
}
