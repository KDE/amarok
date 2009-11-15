/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
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

#include "TrackNavigator.h"
#include "Amarok.h"
#include "amarokconfig.h"
#include "playlist/PlaylistModelStack.h"

#include <QQueue>

Playlist::TrackNavigator::TrackNavigator()
{
    m_model = Playlist::ModelStack::instance()->top();
    m_repeatPlaylist = ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist );
    connect( model(), SIGNAL( removedIds( const QList<quint64>& ) ),
             this, SLOT( dequeueIds( const QList<quint64>& ) ) );
}

bool
Playlist::TrackNavigator::queueId( const quint64 id )
{
    QList<quint64> ids;
    ids << id;
    return queueIds( ids );
}

bool
Playlist::TrackNavigator::queueIds( const QList<quint64> &ids )
{
    foreach( quint64 id, ids )
    {
        if( !m_queue.contains( id ) )
            m_queue.enqueue( id );
    }
    return true;
}

bool
Playlist::TrackNavigator::dequeueId( const quint64 id )
{
    QList<quint64> ids;
    ids << id;
    return dequeueIds( ids );
}

bool
Playlist::TrackNavigator::dequeueIds( const QList<quint64> &ids )
{
    foreach( quint64 id, ids )
        m_queue.removeAll( id );
    return true;
}

int
Playlist::TrackNavigator::queuePosition( const quint64 id ) const
{
    return m_queue.indexOf( id );
}

QQueue<quint64> Playlist::TrackNavigator::queue()
{
    return m_queue;
}

