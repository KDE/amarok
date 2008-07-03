/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "Bias.h"

#include "Debug.h"
#include "BlockingQuery.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "QueryMaker.h"

double
Dynamic::Bias::reevaluate( double oldEnergy, Meta::TrackList oldPlaylist,
        Meta::TrackPtr newTrack, int newTrackPos )
{
    Q_UNUSED( oldEnergy );
    // completely reevaluate by default
    oldPlaylist[newTrackPos] = newTrack;
    return energy( oldPlaylist );
}

Dynamic::CollectionDependantBias::CollectionDependantBias()
    : m_needsUpdating( true )
{
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collection*)),
            this, SLOT(collectionUpdated()) );
}

Dynamic::CollectionDependantBias::CollectionDependantBias( Collection* coll )
    : m_needsUpdating( true )
{
    connect( coll, SIGNAL(updated()), this, SLOT(collectionUpdated()) );
}

bool
Dynamic::CollectionDependantBias::needsUpdating()
{
    return m_needsUpdating;
}

void
Dynamic::CollectionDependantBias::collectionUpdated()
{
    m_needsUpdating = true;
}

Dynamic::GlobalBias::GlobalBias( double weight, QueryMaker* propertyQuery )
    : m_propertyQuery( propertyQuery )
{
    setWeight( weight );
}

Dynamic::GlobalBias::GlobalBias( Collection* coll, double weight, QueryMaker* propertyQuery )
    : CollectionDependantBias(coll), m_propertyQuery( propertyQuery )
{
    setWeight( weight );
}

double
Dynamic::GlobalBias::weight() const
{
    return m_weight;
}

void
Dynamic::GlobalBias::setWeight( double weight )
{
    if( weight > 1.0 )
        m_weight = 1.0;
    else if( weight < 0.0 )
        m_weight = 0.0;
    else
        m_weight = weight;
}



double
Dynamic::GlobalBias::energy( Meta::TrackList playlist )
{
   double satisfiedCount = 0;
   foreach( Meta::TrackPtr t, playlist )
   {
       if( trackSatisfies( t ) )
           satisfiedCount++;
   }

   return qAbs( m_weight - (satisfiedCount / (double)playlist.size()) );
}


double Dynamic::GlobalBias::reevaluate( double oldEnergy, Meta::TrackList oldPlaylist,
        Meta::TrackPtr newTrack, int newTrackPos )
{
    // TODO: how can this be done properly ?
    return Dynamic::Bias::reevaluate( oldEnergy, oldPlaylist, newTrack, newTrackPos );
}


bool Dynamic::GlobalBias::trackSatisfies( Meta::TrackPtr t )
{
    return m_property.contains( t );
}


void Dynamic::GlobalBias::update()
{
    if( !m_needsUpdating )
        return;

    m_propertyQuery->startTrackQuery();
    BlockingQuery bq( m_propertyQuery );

    bq.startQuery();

    m_property.clear();
    QList<Meta::TrackList> trackLists = bq.tracks().values();
    foreach( Meta::TrackList ts, trackLists )
    {
        foreach( Meta::TrackPtr t, ts )
        {
            m_property.insert( t );
        }
    }

    m_needsUpdating = false;
}
