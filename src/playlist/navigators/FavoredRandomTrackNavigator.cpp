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
        QList< qreal > weights;

        AbstractModel* model = Playlist::ModelStack::instance()->top();
        Meta::TrackList tracks = model->tracks();

        switch( AmarokConfig::favorTracks() )
        {
        case AmarokConfig::EnumFavorTracks::HigherScores:
            foreach( Meta::TrackPtr t, tracks )
            {
                int score = t->score();
                qreal weight = score? score * 0.1 : 5.0;
                weights << weight;
            }
            break;
        case AmarokConfig::EnumFavorTracks::HigherRatings:
            foreach( Meta::TrackPtr t, tracks )
            {
                int rating = t->rating();
                qreal weight = rating? rating : 5.0;
                weights << weight;
            }
            break;
        case AmarokConfig::EnumFavorTracks::LessRecentlyPlayed:
            foreach( Meta::TrackPtr t, tracks )
            {
                int lastplayed = t->lastPlayed();
                qreal weight = lastplayed?
                QDateTime::fromTime_t( lastplayed ).secsTo( QDateTime::currentDateTime() ) / 100.0
                : 400.0;
                weights << weight;
            }
            break;
        }

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
