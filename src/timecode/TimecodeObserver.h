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

#ifndef TIMECODEOBSERVER_H
#define TIMECODEOBSERVER_H

#include "EngineController.h"


/**
 * This class handles auto timecoding (position bookmarking) of tracks.
 * After the current track's position has crossed an arbitrary threshold
 * when the user stops playing the track (before the ending) a timecode
 * will be created.
 */
class TimecodeObserver : public EngineObserver
{
public:
    TimecodeObserver();
    virtual ~TimecodeObserver();

    virtual void engineNewTrackPlaying();
    virtual void enginePlaybackEnded ( qint64 finalPosition, qint64 trackLength, EngineObserver::PlaybackEndedReason reason );
    virtual void engineTrackPositionChanged( qint64 position, bool userSeek );

private:
    bool m_trackTimecodeable; //!< stores if current track has the writetimecode capability
    static const qint64 m_threshold;  //!< the arbitrary minum tracklength threshold in milliseconds
    Meta::TrackPtr m_currentTrack; //!< The current/just played track
    qint64 m_currPos; //!< the position the current track is at
};

#endif // TIMECODEOBSERVER_H
