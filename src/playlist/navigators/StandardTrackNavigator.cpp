/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 * Copyright (c) 2011 Sandeep Raghuraman <sandy.8925@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "StandardTrackNavigator.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "playlist/PlaylistModelStack.h"

Playlist::StandardTrackNavigator::StandardTrackNavigator()
{
    m_repeatPlaylist = ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist );
    m_onlyQueue = ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::OnlyQueue );
}

quint64
Playlist::StandardTrackNavigator::chooseLastTrack( bool repeatPlaylist )
{
    Meta::TrackPtr track;
    bool playableTrackFound = false;
    int lastRow;

    // reminder: if that function is modified, it's important to research if changes must also
    // be applied to similar code in `Playlist::StandardTrackNavigator::chooseNextTrack()`

    // search for a playable track in order from right before the currently active track till the start
    for ( lastRow = m_model->activeRow() - 1  ; lastRow >= 0 ; lastRow-- ) // 'activeRow()' may be -1
    {
        track = m_model->trackAt(lastRow);
        if ( track->isPlayable() )
        {
            playableTrackFound = true;
            break;
        }
    }

    // Even though the user is explicitly asking us to go back, still only do wrap-around
    // in mode 'm_repeatPlaylist'. Reason: many users of the standard navigator like to
    // move to the start of the playlist by blindly pressing "Prev" a lot of times.
    // If no playable track was found, the playlist needs to be repeated, it's not empty
    // and there's an active track: search from end of playlist till currently active track.
    if ( !playableTrackFound && repeatPlaylist && m_model->qaim()->rowCount() > 0 && m_model->activeRow() >= 0)
    {
        for ( lastRow = m_model->qaim()->rowCount() - 1 ; lastRow >= m_model->activeRow() ; lastRow--)
        {
            track = m_model->trackAt( lastRow );
            if ( track->isPlayable() )
            {
                playableTrackFound = true;
                break;
            }
        }
    }

    if ( playableTrackFound && m_model->rowExists( lastRow ) )
        return m_model->idAt( lastRow );
    else
        return 0;
}

quint64
Playlist::StandardTrackNavigator::requestNextTrack()
{
    if( !m_queue.isEmpty() ) {
        quint64 ret = m_queue.takeFirst();
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
        return ret;
    }

    return chooseNextTrack( m_repeatPlaylist );
}

quint64
Playlist::StandardTrackNavigator::requestUserNextTrack()
{
    if( !m_queue.isEmpty() ) {
        quint64 ret = m_queue.takeFirst();
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
        return ret;
    }

    // Don't make wrap-around conditional on 'm_repeatPlaylist': the user is explicitly asking for this.
    return chooseNextTrack( true );
}

quint64
Playlist::StandardTrackNavigator::requestLastTrack()
{
    if( !m_queue.isEmpty() ) {
        quint64 ret = m_queue.takeFirst();
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
        return ret;
    }

    return chooseLastTrack( m_repeatPlaylist );
}

quint64
Playlist::StandardTrackNavigator::chooseNextTrack( bool repeatPlaylist )
{
    if( !m_queue.isEmpty() )
        return m_queue.first();

    if( m_onlyQueue )
        return 0;

    Meta::TrackPtr track;
    bool playableTrackFound = false;
    int nextRow;

    // reminder: if that function is modified, it's important to research if changes must also
    // be applied to similar code in `Playlist::StandardTrackNavigator::chooseLastTrack()`

    //search for a playable track in order from right after the currently active track till the end
    for( nextRow = m_model->activeRow() + 1  ; nextRow < m_model->qaim()->rowCount() ; nextRow++ ) // 'activeRow()' may be -1.
    {
        track = m_model->trackAt(nextRow);
        if( track->isPlayable() )
        {
            playableTrackFound = true;
            break;
        }
    }

    //if no playable track was found and the playlist needs to be repeated, search from top of playlist till currently active track
    if( !playableTrackFound && repeatPlaylist )
    {
        //nextRow=0; This row is still invalid if 'rowCount() == 0'.
        for( nextRow = 0 ; nextRow <= m_model->activeRow() ; nextRow++)
        {
            track = m_model->trackAt( nextRow );
            if( track->isPlayable() )
            {
                playableTrackFound = true;
                break;
            }
        }
    }

    if( playableTrackFound && m_model->rowExists( nextRow ) )
        return m_model->idAt( nextRow );
    else
        return 0;
}
