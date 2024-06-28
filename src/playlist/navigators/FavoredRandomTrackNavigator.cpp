/****************************************************************************************
 * Copyright (c) 2009 Edward Toroshchin <edward.hades@gmail.com>                        *
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

#include "FavoredRandomTrackNavigator.h"

#include "playlist/PlaylistModelStack.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Debug.h"

#include <QRandomGenerator>


Playlist::FavoredRandomTrackNavigator::FavoredRandomTrackNavigator()
{
    loadFromSourceModel();
}

void
Playlist::FavoredRandomTrackNavigator::planOne()
{
    DEBUG_BLOCK

    if ( m_plannedItems.isEmpty() && !allItemsList().isEmpty() )
    {
        int avoidRecentlyPlayedSize = AVOID_RECENTLY_PLAYED_MAX;    // Start with being very picky.

        // Don't over-constrain ourself:
        //   - Keep enough headroom to be unpredictable.
        //   - Make sure that 'chooseRandomItem()' doesn't need to find a needle in a haystack.
        avoidRecentlyPlayedSize = qMin( avoidRecentlyPlayedSize, allItemsList().size() / 2 );

        QSet<quint64> avoidSet = getRecentHistory( avoidRecentlyPlayedSize );
        QList<qreal> weights;

        quint64 chosenItem = std::numeric_limits<quint64>::max();
        do
        {
            weights = rowWeights( avoidSet );

            // Choose a weighed random row.
            if( !weights.isEmpty() )
            {
                qreal totalWeight = 0.0;
                for ( qreal weight : weights )
                    totalWeight += weight;

                qreal randomCumulWeight = ( QRandomGenerator::global()->generate() /
                                (double)std::numeric_limits<quint32>::max() ) * totalWeight;

                int row = 0;
                qreal rowCumulWeight = weights[ row ];
                while ( randomCumulWeight > rowCumulWeight + 0.0000000001 )
                    rowCumulWeight += weights[ ++row ];

                chosenItem = m_model->idAt( row );
                avoidSet.insert( chosenItem );
            } // Avoid deadlock by verifying we are giving playable tracks to nextItemChooseDonorList()
        } while ( !weights.isEmpty() && !m_model->trackForId( chosenItem )->isPlayable()
                                     && avoidSet.size() < allItemsList().size() );
        if( chosenItem != std::numeric_limits<quint64>::max() && m_model->trackForId( chosenItem )->isPlayable() )
            m_plannedItems.append( chosenItem );
    }
}

QList<qreal>
Playlist::FavoredRandomTrackNavigator::rowWeights(const QSet<quint64> &avoidSet )
{
    QList<qreal> weights;

    int favorType = AmarokConfig::favorTracks();
    int rowCount = m_model->qaim()->rowCount();

    for( int row = 0; row < rowCount; row++ )
    {
        qreal weight = 0.0;
        Meta::StatisticsPtr statistics = m_model->trackAt( row )->statistics();

        if ( !avoidSet.contains( m_model->idAt( row ) ) )
        {
            switch( favorType )
            {
                case AmarokConfig::EnumFavorTracks::HigherScores:
                {
                    int score = statistics->score();
                    weight = score ? score : 50.0;    // "Unknown" weight: in the middle, 50%
                    break;
                }

                case AmarokConfig::EnumFavorTracks::HigherRatings:
                {
                    int rating = statistics->rating();
                    weight = rating ? rating : 5.0;
                    break;
                }

                case AmarokConfig::EnumFavorTracks::LessRecentlyPlayed:
                {
                    QDateTime lastPlayed = statistics->lastPlayed();
                    if ( lastPlayed.isValid() )
                    {
                        weight = lastPlayed.secsTo( QDateTime::currentDateTime() );
                        if ( weight < 0 )    // If 'lastPlayed()' is nonsense, or the system clock has been set back:
                            weight = 1 * 60 * 60;    // "Nonsense" weight: 1 hour.
                    }
                    else
                        weight = 365 * 24 * 60 * 60;    // "Never" weight: 1 year.
                    break;
                }
            }
        }

        weights.append( weight );
    }

    return weights;
}

QSet<quint64>
Playlist::FavoredRandomTrackNavigator::getRecentHistory( int size )
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
