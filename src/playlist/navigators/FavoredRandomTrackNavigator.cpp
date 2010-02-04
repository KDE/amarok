/****************************************************************************************
 * Copyright (c) 2009 Edward Toroshchin <edward.hades@gmail.com>                        *
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

#include "amarokconfig.h"
#include "Debug.h"
#include "playlist/PlaylistModelStack.h"

#include <QList>

#include <KRandom>

quint64
Playlist::FavoredRandomTrackNavigator::likelyNextTrack()
{
    if( !m_queue.isEmpty() )
        return m_queue.first();

    if ( !m_random )
        m_random = KRandom::random();

    const int row = randomRow();
    if ( row < 0 )
        return 0;
    AbstractModel* model = Playlist::ModelStack::instance()->top();
    return model->idAt( row );
}

quint64
Playlist::FavoredRandomTrackNavigator::likelyLastTrack()
{
    if( m_history.isEmpty() )
        return requestNextTrack();
    
    return m_history.first();
}

quint64
Playlist::FavoredRandomTrackNavigator::requestNextTrack()
{
    DEBUG_BLOCK

    if( !m_queue.isEmpty() )
        return m_queue.takeFirst();

    m_random = KRandom::random();

    const int row = randomRow();
    if ( row < 0 )
        return 0;

    AbstractModel* model = Playlist::ModelStack::instance()->top();
    quint64 next = model->idAt( row );
    m_history.prepend( next );
    return next;
}

quint64
Playlist::FavoredRandomTrackNavigator::requestLastTrack()
{
    if( m_history.isEmpty() )
        return requestNextTrack();

    return m_history.takeFirst();
}

int
Playlist::FavoredRandomTrackNavigator::randomRow()
{
    DEBUG_BLOCK

    QList< qreal > weights;
    qreal totalWeight = 0.0;
    
    AbstractModel* model = Playlist::ModelStack::instance()->top();
    Meta::TrackList tracks = model->tracks();
    
    switch( AmarokConfig::favorTracks() )
    {
    case AmarokConfig::EnumFavorTracks::HigherScores:
        foreach( Meta::TrackPtr t, tracks )
        {
            int score = t->score();
            qreal weight = score? score * 0.1 : 5.0;
            totalWeight += weight;
            weights << weight;
        }
        break;
    case AmarokConfig::EnumFavorTracks::HigherRatings:
        foreach( Meta::TrackPtr t, tracks )
        {
            int rating = t->rating();
            qreal weight = rating? rating : 5.0;
            totalWeight += weight;
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
            totalWeight += weight;
            weights << weight;
        }
        break;
    }
    
    if( weights.isEmpty() )
        return -1;
    
    debug() << "Total weight is" << totalWeight;
    
    qreal point = ( m_random / qreal( RAND_MAX ) ) * totalWeight - weights[0];
    int row = 0;
    for(; point > 0.0; row++, point -= weights[row]) ;

    return row;
}

void
Playlist::FavoredRandomTrackNavigator::reset()
{
    m_history.clear();
    m_random = 0;
}
