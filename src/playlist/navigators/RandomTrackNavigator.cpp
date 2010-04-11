/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#define DEBUG_PREFIX "Playlist::RandomTrackNavigator"

#include "RandomTrackNavigator.h"

#include "core/support/Debug.h"

#include <QtGlobal> // For 'qrand()'
#include <math.h> // For 'round()'


Playlist::RandomTrackNavigator::RandomTrackNavigator()
{
    loadFromSourceModel();
}

void
Playlist::RandomTrackNavigator::planOne()
{
    DEBUG_BLOCK

    if ( m_plannedItems.isEmpty() )
    {
        if ( !allItemsList().isEmpty() )
        {
            quint64 chosenItem;

            int avoidRecentlyPlayedSize = AVOID_RECENTLY_PLAYED_MAX;    // Start with being very picky.

            // Don't over-constrain ourself:
            //   - Keep enough headroom to be unpredictable.
            //   - Make sure that 'chooseRandomItem()' doesn't need to find a needle in a haystack.
            avoidRecentlyPlayedSize = qMin( avoidRecentlyPlayedSize, allItemsList().size() / 2 );

            QSet<quint64> avoidSet = getRecentHistory( avoidRecentlyPlayedSize );
            chosenItem = chooseRandomItem( avoidSet );

            m_plannedItems.append( chosenItem );
        }
    }
}

QSet<quint64>
Playlist::RandomTrackNavigator::getRecentHistory( int size )
{
    QList<quint64> allHistory = historyItems();
    QSet<quint64> recentHistory;

    if ( size > 0 ) {    // If '== 0', we even need to consider playing the same item again.
        recentHistory.insert( currentItem() );    // Might be '0'
        size--;
    }

    for ( int i = allHistory.size() - 1; ( i >= 0 ) && ( i >= allHistory.size() - size ); i-- )
        recentHistory.insert( allHistory.at( i ) );

    return recentHistory;
}

quint64
Playlist::RandomTrackNavigator::chooseRandomItem( QSet<quint64> avoidSet )
{
    quint64 chosenItem;

    do
    {
        int maxPosition = allItemsList().size() - 1;
        int randomPosition = round( ( qrand() / (float)RAND_MAX ) * maxPosition );
        chosenItem = allItemsList().at( randomPosition );
    } while ( avoidSet.contains( chosenItem ) );

    return chosenItem;
}
