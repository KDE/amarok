/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "TimecodeObserver.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"

const qint64 TimecodeObserver::m_threshold = 600 * 1000; // 6000000ms = 10 minutes

TimecodeObserver::TimecodeObserver( QObject *parent )
    : QObject( parent )
    , m_trackTimecodeable ( false )
    , m_currentTrack ( nullptr )
    , m_currPos ( 0 )
{
    EngineController *engine = The::engineController();

    connect( engine, &EngineController::stopped,
             this, &TimecodeObserver::stopped );
    connect( engine, &EngineController::trackPlaying,
             this, &TimecodeObserver::trackPlaying );
    connect( engine, &EngineController::trackPositionChanged,
             this, &TimecodeObserver::trackPositionChanged );
}

TimecodeObserver::~TimecodeObserver()
{}

void
TimecodeObserver::stopped( qint64 finalPosition, qint64 trackLength )
{
    DEBUG_BLOCK

    if( m_trackTimecodeable && finalPosition != trackLength && trackLength > m_threshold && finalPosition > 60 * 1000 )
    {
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
        if( currentTrack )
        {
            Capabilities::TimecodeWriteCapability *tcw = currentTrack->create<Capabilities::TimecodeWriteCapability>();
            if( tcw )
            {
                tcw->writeAutoTimecode ( finalPosition ); // save the timecode
                delete tcw;
            }
        }
    }
}

void
TimecodeObserver::trackPlaying( Meta::TrackPtr track )
{
    if( track == m_currentTrack ) // no change, so do nothing
        return;

    if( m_currentTrack ) // this is really the track _just_ played
    {
        if( m_trackTimecodeable && m_currPos != m_currentTrack->length() && m_currentTrack->length() > m_threshold && m_currPos > 60 * 1000 )
        {
            QScopedPointer<Capabilities::TimecodeWriteCapability> tcw( m_currentTrack->create<Capabilities::TimecodeWriteCapability>() );
            if( tcw )
                tcw->writeAutoTimecode ( m_currPos ); // save the timecode
        }
    }

    // now update to the new track
    if( track && track->has<Capabilities::TimecodeWriteCapability>() )
        m_trackTimecodeable = true;

    m_currentTrack = track;
    m_currPos = 0;
}

void TimecodeObserver::trackPositionChanged( qint64 position, bool userSeek )
{
    Q_UNUSED ( userSeek )

    m_currPos = position;
}

