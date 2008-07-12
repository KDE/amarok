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

#include "BlockingQuery.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "DynamicBiasWidgets.h"
#include "QueryMaker.h"


Dynamic::Bias::Bias()
    : m_active(true)
{
}

QString
Dynamic::Bias::description() const
{
    return m_description;
}

void
Dynamic::Bias::setDescription( const QString& description )
{
    m_description = description;
}

PlaylistBrowserNS::BiasWidget*
Dynamic::Bias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( this, parent );
}


void
Dynamic::Bias::setActive( bool active )
{
    m_active = active;    
}

bool
Dynamic::Bias::active()
{
    return m_active;
}

double
Dynamic::Bias::reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist,
        Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context )
{
    Q_UNUSED( oldEnergy );
    // completely reevaluate by default
    Meta::TrackList oldPlaylistCopy( oldPlaylist );
    oldPlaylistCopy[newTrackPos] = newTrack;
    return energy( oldPlaylist, context );
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

Dynamic::GlobalBias::GlobalBias( double weight, QueryMaker* qm,
        XmlQueryReader::Filter filter )
    : m_propertyQuery(0)
    , m_filter( filter )
{
    setWeight( weight );
    setQuery( qm );
}

Dynamic::GlobalBias::GlobalBias( Collection* coll, double weight, QueryMaker* qm,
        XmlQueryReader::Filter filter )
    : CollectionDependantBias( coll )
    , m_propertyQuery(0)
    , m_filter(filter)
{
    setWeight( weight );
    setQuery( qm );
}

PlaylistBrowserNS::BiasWidget*
Dynamic::GlobalBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasGlobalWidget( this, parent );
}

XmlQueryReader::Filter&
Dynamic::GlobalBias::filter()
{
    return m_filter;
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

void
Dynamic::GlobalBias::setQuery( QueryMaker* qm )
{
    DEBUG_BLOCK
    qm->setQueryType( QueryMaker::Track );
    delete m_propertyQuery;
    m_propertyQuery = new BlockingQuery( qm );
    collectionUpdated();
}


double
Dynamic::GlobalBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context )
{
    Q_UNUSED( context );

    double satisfiedCount = 0;
    foreach( Meta::TrackPtr t, playlist )
    {
        if( trackSatisfies( t ) )
            satisfiedCount++;
    }

    return  m_weight - (satisfiedCount / (double)playlist.size());
}


double Dynamic::GlobalBias::reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist,
        Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context )
{
    Q_UNUSED( context );

    double offset = 1.0 / (double)oldPlaylist.size();

    bool prevSatisfied = trackSatisfies( oldPlaylist[newTrackPos] );

    if( trackSatisfies( newTrack ) && !prevSatisfied )
        return oldEnergy - offset;
    else if( !trackSatisfies( newTrack ) && prevSatisfied )
        return oldEnergy + offset;
    else
        return oldEnergy;
}


bool Dynamic::GlobalBias::trackSatisfies( Meta::TrackPtr t )
{
    return m_property.contains( t );
}


void Dynamic::GlobalBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_propertyQuery->resetResults();
    m_propertyQuery->startQuery();

    m_property.clear();
    QList<Meta::TrackList> trackLists = m_propertyQuery->tracks().values();
    foreach( Meta::TrackList ts, trackLists )
    {
        foreach( Meta::TrackPtr t, ts )
        {
            m_property.insert( t );
        }
    }

    m_needsUpdating = false;
}

