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
#include "Debug.h"

#include <KRandom>


Playlist::FavoredRandomTrackNavigator::FavoredRandomTrackNavigator()
{
    loadFromSourceModel();
}

void
Playlist::FavoredRandomTrackNavigator::planOne()
{
    DEBUG_BLOCK

    if ( m_plannedItems.isEmpty() )
    {
        QList<qreal> weights = rowWeights();

        // Choose a weighed random row.
        if( !weights.isEmpty() )
        {
            qreal totalWeight = 0.0;
            foreach ( qreal weight, weights )
                totalWeight += weight;

            qreal randomCumulWeight = ( KRandom::random() / qreal( RAND_MAX ) ) * totalWeight;

            int row = 0;
            qreal rowCumulWeight = weights[ row ];
            while ( randomCumulWeight > rowCumulWeight + 0.0000000001 )
                rowCumulWeight += weights[ ++row ];

            m_plannedItems.append( m_model->idAt( row ) );
        }
    }
}

QList<qreal>
Playlist::FavoredRandomTrackNavigator::rowWeights()
{
    QList<qreal> weights;

    int favorType = AmarokConfig::favorTracks();
    int rowCount = m_model->rowCount();

    for( int row = 0; row < rowCount; row++ )
    {
        qreal weight;

        switch( favorType )
        {
            case AmarokConfig::EnumFavorTracks::HigherScores:
            {
                int score = m_model->trackAt( row )->score();
                weight = score ? score : 50.0;    // "Unknown" weight: in the middle, 50%
                break;
            }

            case AmarokConfig::EnumFavorTracks::HigherRatings:
            {
                int rating = m_model->trackAt( row )->rating();
                weight = rating ? rating : 5.0;
                break;
            }

            case AmarokConfig::EnumFavorTracks::LessRecentlyPlayed:
            {
                uint lastPlayed = m_model->trackAt( row )->lastPlayed();
                if ( lastPlayed )
                {
                    weight = QDateTime::fromTime_t( lastPlayed ).secsTo( QDateTime::currentDateTime() );
                    if ( weight < 0 )    // If 'lastPlayed()' is nonsense, or the system clock has been set back:
                        weight = 1 * 60 * 60;    // "Nonsense" weight: 1 hour.
                }
                else
                    weight = 365 * 24 * 60 * 60;    // "Never" weight: 1 year.
                break;
            }
        }

        weights.append( weight );
    }

    return weights;
}
