/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"

#include <QQueue>

Playlist::TrackNavigator::TrackNavigator()
{
    m_model = The::playlist();

    // Connect to the QAbstractItemModel signals of the source model.
    //   Ignore SIGNAL dataChanged: we don't need to know when a playlist item changes.
    //   Ignore SIGNAL layoutChanged: we don't need to know when rows are moved around.
    connect( m_model->qaim(), &QAbstractItemModel::modelReset, this, &TrackNavigator::slotModelReset );
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::rowsAboutToBeRemoved,
             this, &TrackNavigator::slotRowsAboutToBeRemoved );
    //   Ignore SIGNAL rowsInserted.
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

bool
Playlist::TrackNavigator::queueMoveUp( const quint64 id )
{
    return queueMoveTo( id, m_queue.indexOf( id ) - 1 );
}

bool
Playlist::TrackNavigator::queueMoveDown( const quint64 id )
{
    return queueMoveTo( id, m_queue.indexOf( id ) + 1 );
}

bool
Playlist::TrackNavigator::queueMoveTo( const quint64 id, const int pos )
{
    const int idx = m_queue.indexOf( id );
    if( idx == -1 || idx == pos || pos < 0 || pos > m_queue.count() - 1 )
        return false;
    m_queue.move( idx, pos );
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

void
Playlist::TrackNavigator::slotModelReset()
{
    DEBUG_BLOCK
    m_queue.clear();    // We should check 'm_model's new contents, but this is unlikely to bother anyone.
}

void
Playlist::TrackNavigator::slotRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED( parent );

    for ( int row = start; row <= end; ++row )
    {
        const quint64 itemId = Playlist::ModelStack::instance()->bottom()->idAt( row );
        m_queue.removeAll( itemId );
    }
}

quint64
Playlist::TrackNavigator::bestFallbackItem()
{
    quint64 item = m_model->activeId();

    if ( !item )
        if ( m_model->qaim()->rowCount() > 0 )
            item = m_model->idAt( 0 );

    return item;
}
