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

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "DynamicBiasWidgets.h"
#include "DynamicModel.h"
#include "MetaQueryMaker.h"
#include "QueryMaker.h"
#include "collection/support/XmlQueryWriter.h"

#include <QMutexLocker>




Dynamic::Bias*
Dynamic::Bias::fromXml( QDomElement e )
{
    DEBUG_BLOCK

    if( e.tagName() != "bias" )
        return 0;

    QString type = e.attribute( "type" );

    if( type == "global" )
    {
        double weight = 0.0;
        XmlQueryReader::Filter filter;


        QDomElement queryElement = e.firstChildElement( "query" );
        if( !queryElement.isNull() )
        {
            // I don't actually need a qm from XmlQueryReader, I just want the filters.
            QueryMaker* dummyQM = new MetaQueryMaker( QList<QueryMaker*>() );

            QString rawXml;
            QTextStream rawXmlStream( &rawXml );
            queryElement.save( rawXmlStream, 0 );
            XmlQueryReader reader( dummyQM, XmlQueryReader::IgnoreReturnValues );
            reader.read( rawXml );
            if( reader.getFilters().size() > 0 )
                filter = reader.getFilters().first();

            delete dummyQM;
        }

        QDomElement weightElement = e.firstChildElement( "weight" );
        if( !weightElement.isNull() )
        {
            weight = weightElement.attribute("value").toDouble();
        }

        return new Dynamic::GlobalBias( weight, filter );
    }

    // TODO: other types of biases
    else
    {
        error() << "Unknown bias type.";
        return 0;
    }
}




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
    : m_collection(0)
    , m_needsUpdating( true )
{
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collection*)),
            this, SLOT(collectionUpdated()) );
}

Dynamic::CollectionDependantBias::CollectionDependantBias( Collection* coll )
    : m_collection(coll)
    , m_needsUpdating( true )
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

Dynamic::GlobalBias::GlobalBias( double weight, XmlQueryReader::Filter filter )
    : m_qm(0)
{
    setWeight( weight );
    setQuery( filter );
}

Dynamic::GlobalBias::GlobalBias( Collection* coll, double weight, XmlQueryReader::Filter filter )
    : CollectionDependantBias( coll )
    , m_qm(0)
{
    setWeight( weight );
    setQuery( filter );
}

Dynamic::GlobalBias::~GlobalBias()
{
    delete m_qm;
}

QDomElement
Dynamic::GlobalBias::xml() const
{
    QDomDocument doc =
        PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc();

    QDomElement e = doc.createElement( "bias" );
    e.setAttribute( "type", "global" );

    QDomElement weight = doc.createElement( "weight" );
    weight.setAttribute( "value", m_weight );

    e.appendChild( weight );
    e.appendChild( m_qm->getDomElement() );

    return e;
}


PlaylistBrowserNS::BiasWidget*
Dynamic::GlobalBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasGlobalWidget( this, parent );
}

const XmlQueryReader::Filter&
Dynamic::GlobalBias::filter() const
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
Dynamic::GlobalBias::setQuery( XmlQueryReader::Filter filter )
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    QueryMaker* qm;
    if( m_collection )
        qm = m_collection->queryMaker();
    else
        qm = CollectionManager::instance()->primaryCollection()->queryMaker();

    m_qm = new XmlQueryWriter( qm,
            PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc() );

    if( filter.field != 0 )
    {
        if( filter.compare == -1 )
            m_qm->addFilter( filter.field, filter.value );
        else
            m_qm->addNumberFilter( filter.field, filter.value.toLongLong(),
                    (QueryMaker::NumberComparison)filter.compare );
    }

    m_qm->setQueryType( QueryMaker::Track );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL(newResultReady( QString, Meta::TrackList )),
            SLOT(updateReady( QString, Meta::TrackList )), Qt::DirectConnection );
    connect( m_qm, SIGNAL(queryDone()), SLOT(updateFinished()), Qt::DirectConnection );

    m_filter = filter;
    collectionUpdated(); // force an update
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
    QMutexLocker locker( &m_mutex );
    return m_property.contains( t );
}


void Dynamic::GlobalBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

void 
Dynamic::GlobalBias::updateReady( QString collectionId, Meta::TrackList tracks )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)

    QMutexLocker locker( &m_mutex );

    m_property.clear();
    m_property.reserve( tracks.size() );
    foreach( Meta::TrackPtr t, tracks )
        m_property.insert( t );        

}

void
Dynamic::GlobalBias::updateFinished()
{
    DEBUG_BLOCK
    m_mutex.lock();
    m_needsUpdating = false;
    m_mutex.unlock(); //do not keep locks when emitting signals

    emit biasUpdated( this );
}

