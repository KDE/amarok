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

#include "playlist/PlaylistModelStack.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"

#include <QQueue>


Playlist::TrackNavigator::TrackNavigator()
{
    m_model = Playlist::ModelStack::instance()->top();

    // Connect to the QAbstractItemModel signals of the source model.
    //   Ignore SIGNAL dataChanged: we don't need to know when a playlist item changes.
    //   Ignore SIGNAL layoutChanged: we don't need to know when rows are moved around.
    connect( m_model->qaim(), SIGNAL( modelReset() ), this, SLOT( slotModelReset() ) );
    //   Ignore SIGNAL rowsInserted.
    connect( m_model->qaim(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ), this, SLOT( slotRowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
}

void
Playlist::TrackNavigator::queueId( const quint64 id )
{
    QList<quint64> ids;
    ids << id;
    queueIds( ids );
}

void
Playlist::TrackNavigator::queueIds( const QList<quint64> &ids )
{
    foreach( quint64 id, ids )
    {
        if( !m_queue.contains( id ) )
            m_queue.enqueue( id );
    }
}

void
Playlist::TrackNavigator::dequeueId( const quint64 id )
{
    m_queue.removeAll( id );
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

void
Playlist::TrackNavigator::slotModelReset()
{
    DEBUG_BLOCK
    m_queue.clear();    // We should check 'm_model's new contents, but this is unlikely to bother anyone.
}

// This function can get called thousands of times during a single FilterProxy change.
// Be very efficient here! (e.g. no DEBUG_BLOCK)
void
Playlist::TrackNavigator::slotRowsAboutToBeRemoved( const QModelIndex& parent, int startRow, int endRow )
{
    Q_UNUSED( parent );

    for (int row = startRow; row <= endRow; row++)
        dequeueId( m_model->idAt( row ) );
}

quint64
Playlist::TrackNavigator::bestFallbackItem()
{
    quint64 item = m_model->activeId();

    if ( !item )
        if ( m_model->rowCount() > 0 )
            item = m_model->idAt( 0 );

    return item;
}
