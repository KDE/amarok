/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "core/support/Amarok.h"
#include "amarokconfig.h"


Playlist::StandardTrackNavigator::StandardTrackNavigator()
{
    m_repeatPlaylist = ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist );
}

quint64
Playlist::StandardTrackNavigator::likelyLastTrack()
{
    int lastRow = m_model->activeRow() - 1;

    // Even though the user is explicitly asking us to go back, still only do wrap-around
    // in mode 'm_repeatPlaylist'. Reason: many users of the standard navigator like to
    // move to the start of the playlist by blindly pressing "Prev" a lot of times.
    if( lastRow < 0 )
        if( m_repeatPlaylist )
            lastRow = m_model->qaim()->rowCount() - 1;    // This row is still invalid if 'rowCount() == 0'.

    if( m_model->rowExists( lastRow ) )
        return m_model->idAt( lastRow );
    else
        return 0;
}

quint64
Playlist::StandardTrackNavigator::requestNextTrack()
{
    if( !m_queue.isEmpty() )
        return m_queue.takeFirst();

    return chooseNextTrack( m_repeatPlaylist );
}

quint64
Playlist::StandardTrackNavigator::requestUserNextTrack()
{
    if( !m_queue.isEmpty() )
        return m_queue.takeFirst();

    // Don't make wrap-around conditional on 'm_repeatPlaylist': the user is explicitly asking for this.
    return chooseNextTrack( true );
}

quint64
Playlist::StandardTrackNavigator::requestLastTrack()
{
    return likelyLastTrack();
}

quint64
Playlist::StandardTrackNavigator::chooseNextTrack( bool repeatPlaylist )
{
    if( !m_queue.isEmpty() )
        return m_queue.first();

    int nextRow = m_model->activeRow() + 1;    // 'activeRow()' may be -1.

    if( nextRow >= m_model->qaim()->rowCount() )
        if( repeatPlaylist )
            nextRow = 0;    // This row is still invalid if 'rowCount() == 0'.

    if( m_model->rowExists( nextRow ) )
        return m_model->idAt( nextRow );
    else
        return 0;
}
