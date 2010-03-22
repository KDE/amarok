/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#define DEBUG_PREFIX "Playlist::NonlinearTrackNavigator"

#include "NonlinearTrackNavigator.h"

#include "Debug.h"


Playlist::NonlinearTrackNavigator::NonlinearTrackNavigator()
    : m_currentItem( 0 )
{
    // Connect to the QAbstractItemModel signals of the source model.
    //   Ignore SIGNAL dataChanged: changes in metadata etc. don't affect the random play order.
    //   Ignore SIGNAL layoutChanged: rows moving around doesn't affect the random play order.
    connect( model(), SIGNAL( modelReset() ), this, SLOT( slotModelReset() ) );
    connect( model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( slotRowsInserted( const QModelIndex &, int, int ) ) );
    connect( model(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ), this, SLOT( slotRowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );

    // Connect to the Playlist::AbstractModel signals of the source model.
    connect( model(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SLOT( slotActiveTrackChanged( const quint64 ) ) );
}


//!***** Keeping in-sync with the source model

void
Playlist::NonlinearTrackNavigator::slotModelReset()
{
    DEBUG_BLOCK

    m_insertedItems.clear();
    m_removedItems += allItemsSet();

    int lastRowInModel = m_model->rowCount() - 1;
    if ( lastRowInModel >= 0 )
        slotRowsInserted( QModelIndex(), 0, lastRowInModel );

    doItemListsMaintenance();

    if ( !currentItem() )
        setCurrentItem( m_model->activeId() );
}

// This function can get called thousands of times during a single FilterProxy change.
// Be very efficient here! (e.g. no DEBUG_BLOCK)
void
Playlist::NonlinearTrackNavigator::slotRowsInserted( const QModelIndex& parent, int startRow, int endRow )
{
    Q_UNUSED( parent );

    for ( int row = startRow; row <= endRow; row++ )
    {
        quint64 itemId = m_model->idAt( row );

        m_insertedItems.insert( itemId );
        m_removedItems.remove( itemId );
    }
}

// This function can get called thousands of times during a single FilterProxy change.
// Be very efficient here! (e.g. no DEBUG_BLOCK)
void
Playlist::NonlinearTrackNavigator::slotRowsAboutToBeRemoved( const QModelIndex& parent, int startRow, int endRow )
{
    Q_UNUSED( parent );

    for ( int row = startRow; row <= endRow; row++ )
    {
        quint64 itemId = m_model->idAt( row );

        m_insertedItems.remove( itemId );
        m_removedItems.insert( itemId );
    }
}

// A general note on this function: thousands of rows can be inserted/removed by a single
// FilterProxy change. However, this function gets to process them in a big batch.
//
// So: O(n * log n) performance is good enough, but O(n^2) is not.
// (that's also why we need the 'listRemove()' helper function)
void
Playlist::NonlinearTrackNavigator::doItemListsMaintenance()
{
    DEBUG_BLOCK

    // Move batch instructions to local storage immediately, because we may get called recursively.
    QSet<quint64> tmpInsertedItems = m_insertedItems;
    m_insertedItems.clear();

    QSet<quint64> tmpRemovedItems = m_removedItems;
    m_removedItems.clear();

    // Handle the removed items
    if ( !tmpRemovedItems.isEmpty() )
    {
        QSet<quint64> knownRemovedItems = tmpRemovedItems & allItemsSet();    // Filter out items inserted+removed between calls to us.

        Item::listRemove( m_allItemsList, tmpRemovedItems );
        Item::listRemove( m_historyItems, tmpRemovedItems );
        Item::listRemove( m_replayedItems, tmpRemovedItems );
        Item::listRemove( m_plannedItems, tmpRemovedItems );

        notifyItemsRemoved( knownRemovedItems );

        if ( tmpRemovedItems.contains( currentItem() ) )    // After 'notifyItemsRemoved()', so that they get a chance to choose a new one.
            setCurrentItem( 0 );
    }

    // Handle the newly inserted items
    if ( !tmpInsertedItems.isEmpty() )
    {
        QSet<quint64> unknownInsertedItems = tmpInsertedItems - allItemsSet();    // Filter out items removed+reinserted between calls to us.

        m_allItemsList.append( unknownInsertedItems.toList() );
        m_plannedItems.clear();    // Could do this more subtly in each child class, but this is good enough.

        notifyItemsInserted( unknownInsertedItems );
    }

    // Prune history size
    while ( m_historyItems.size() > MAX_HISTORY_SIZE )
        m_historyItems.removeFirst();
}


//!***** Current playlist item

quint64
Playlist::NonlinearTrackNavigator::currentItem()
{
    doItemListsMaintenance();
    return m_currentItem;
}

void
Playlist::NonlinearTrackNavigator::setCurrentItem( const quint64 newItem, bool goingBackward )
{
    DEBUG_BLOCK

    doItemListsMaintenance();

    // Remember that we've played the old item.
    if ( m_currentItem )
    {
        if ( goingBackward )
            m_replayedItems.prepend( m_currentItem );
        else
            m_historyItems.append( m_currentItem );
    }

    m_currentItem = newItem;

    // If the new current item happens to also be the next planned item, consider that
    // plan "done". Can happen e.g. when the user manually plays our next planned item.
    if ( m_currentItem )
        if ( !m_plannedItems.isEmpty() && m_plannedItems.first() == m_currentItem )
            m_plannedItems.removeFirst();
}

// In the normal case this signal slot is redundant, because 'requestNext|LastTrack()'
// already called 'setCurrentItem()' long before this function gets called.
//
// This signal slot takes care of some special cases, like the user clicking on
// an arbitrary item in the playlist.
void
Playlist::NonlinearTrackNavigator::slotActiveTrackChanged( const quint64 id )
{
    DEBUG_BLOCK

    doItemListsMaintenance();

    if ( currentItem() != id )    // If the new item is not what we expected:
    {
        // Heuristic: if this new "current item" does not look like we're going back/fwd in
        // history, then cancel "visit history" mode.
        // Not important, just a small nicety. It's what the user probably wants.
        if ( ( m_historyItems.isEmpty() || m_historyItems.last() != id ) &&
             ( !m_replayedItems.contains( id ) ) )
        {
            m_historyItems.append( m_replayedItems );
            m_replayedItems.clear();
        }

        // Ditch the plan. The unexpected "current item" might change what we want to do next.
        m_plannedItems.clear();

        // The main thing we need to do.
        setCurrentItem( id );
    }
}


//!***** Choosing next playlist item

Playlist::ItemList*
Playlist::NonlinearTrackNavigator::nextItemChooseDonorList()
{
    DEBUG_BLOCK

    if ( !m_queue.isEmpty() )    // User-specified queue has highest priority.
        return &m_queue;
    else if ( !m_replayedItems.isEmpty() )    // If the user pressed "previous" once or more, first re-play those items again when we go forward again.
        return &m_replayedItems;
    else
    {
        if ( m_plannedItems.isEmpty() )
            planOne();
        if ( !m_plannedItems.isEmpty() )    // The normal case.
            return &m_plannedItems;
    }

    return 0;
}

quint64
Playlist::NonlinearTrackNavigator::likelyNextTrack()
{
    doItemListsMaintenance();

    ItemList *donor = nextItemChooseDonorList();
    return donor ? donor->first() : 0;
}

// We could just call 'likelyNextTrack()' and assume that we'll get a 'slotActiveTrackChanged'
// callback later. But let's follow our API strictly: update the donor list immediately.
quint64
Playlist::NonlinearTrackNavigator::requestNextTrack()
{
    doItemListsMaintenance();

    ItemList *donor = nextItemChooseDonorList();
    quint64 nextItem = donor ? donor->takeFirst() : 0;

    setCurrentItem( nextItem );
    return m_currentItem;
}


//!***** Choosing previous playlist item

quint64
Playlist::NonlinearTrackNavigator::likelyLastTrack()
{
    doItemListsMaintenance();

    return m_historyItems.isEmpty() ? 0 : m_historyItems.last();
}

quint64
Playlist::NonlinearTrackNavigator::requestLastTrack()
{
    doItemListsMaintenance();

    quint64 lastItem = m_historyItems.isEmpty() ? 0 : m_historyItems.takeLast();

    setCurrentItem( lastItem, true );
    return m_currentItem;
}
