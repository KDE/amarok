/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

quint64
Playlist::StandardTrackNavigator::requestNextTrack()
{
    if( !m_queue.isEmpty() )
        return m_queue.takeFirst();
    int updateRow = m_model->activeRow() + 1;
    if ( m_repeatPlaylist )
        updateRow = ( updateRow >= m_model->rowCount() ) ? 0 : updateRow;
    return m_model->idAt( updateRow );
}

quint64
Playlist::StandardTrackNavigator::requestLastTrack()
{
    int updateRow = m_model->activeRow() - 1;
    if ( m_repeatPlaylist )
        updateRow = ( updateRow < 0 ) ? m_model->rowCount() - 1 : updateRow;
    return m_model->idAt( updateRow );
}

