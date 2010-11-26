/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "Bias"

#include "Bias.h"

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "CustomBias.h"
#include "core/support/Debug.h"
#include "DynamicBiasWidgets.h"
#include "DynamicModel.h"
#include "core/meta/support/MetaConstants.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"

#include <QMutexLocker>
#include <QDateTime>

#include <cmath>

// defined in gsl/gauss.c
extern "C" {
double gsl_cdf_gaussian_P( const double x, const double sigma );
}

double sqr( double x )
{
    return x*x;
}

Dynamic::Bias*
Dynamic::Bias::fromXml( QDomElement e )
{
    DEBUG_BLOCK

    if( e.tagName() != "bias" )
        return 0;

    QString type = e.attribute( "type" );

    if( type == "global" )
    {
        return Dynamic::GlobalBias::fromXml( e );
    }
    else if( type == "custom" )
    {
        // handle whichever type this actually is, pass it off to the CustomBias builder
        return Dynamic::CustomBias::fromXml( e );
    }
    else if( type == "normal" )
    {
        QDomElement meanElement = e.firstChildElement( "mean" );
        QDomElement scaleElement = e.firstChildElement( "scale" );
        QDomElement fieldElement = e.firstChildElement( "field" );

        double mean = meanElement.attribute( "value", "0" ).toDouble();
        double scale = scaleElement.attribute( "value", "0.5" ).toDouble();
        qint64 field = fieldElement.attribute( "value", "0" ).toLongLong();

        Dynamic::NormalBias* nbias = new Dynamic::NormalBias();
        nbias->setScale( scale );
        nbias->setField( field );
        nbias->setValue( mean );

        return nbias;
    }
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
        Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context ) const
{
    Q_UNUSED( oldEnergy );
    // completely reevaluate by default
    Meta::TrackList newPlaylist( oldPlaylist );
    newPlaylist[newTrackPos] = newTrack;
    return energy( newPlaylist, context );
}


Dynamic::CollectionDependantBias::CollectionDependantBias()
    : m_collection(0)
    , m_needsUpdating( true )
{
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
            this, SLOT(collectionUpdated()) );
}

Dynamic::CollectionDependantBias::CollectionDependantBias( Collections::Collection* coll )
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

Dynamic::GlobalBias::GlobalBias( double weight, MetaQueryWidget::Filter filter )
{
    setWeight( weight );
    setFilter( filter );
}

Dynamic::GlobalBias::GlobalBias( Collections::Collection* coll, double weight, MetaQueryWidget::Filter filter )
    : CollectionDependantBias( coll )
{
    setWeight( weight );
    setFilter( filter );
}

Dynamic::GlobalBias::~GlobalBias()
{
    delete m_qm.data();
}

QString
Dynamic::GlobalBias::filterConditionToString( MetaQueryWidget::FilterCondition cond )
{
    switch( cond )
    {
    case MetaQueryWidget::Equals:
        return "equals";
    case MetaQueryWidget::GreaterThan:
        return "greater";
    case MetaQueryWidget::LessThan:
        return "less";
    case MetaQueryWidget::Between:
        return "between";
    case MetaQueryWidget::OlderThan:
        return "older";
    case MetaQueryWidget::Contains:
        return "contains";
    default:
        ;// the other conditions are only for the advanced playlist generator
    }
    return QString();
}

Dynamic::GlobalBias*
Dynamic::GlobalBias::fromXml( QDomElement e )
{
    double weight = 0.0;
    MetaQueryWidget::Filter filter;

    QDomElement queryElement = e.firstChildElement( "query" );
    if( !queryElement.isNull() )
    {
        QDomElement filtersElement = queryElement.firstChildElement( "filters" );
        if( !filtersElement.isNull() )
        {
            QDomElement includeElement = filtersElement.firstChildElement( "include" );
            if( !includeElement.isNull() )
            {
                QString field = includeElement.attribute("field");
                filter.field = Meta::fieldForName( field );
                filter.value = includeElement.attribute( "value", "" );
                filter.numValue = filter.value.toLongLong();
                filter.numValue2 = includeElement.attribute( "value2", 0 ).toLongLong();

                QString condition = includeElement.attribute( "compare", "" );
                filter.condition = MetaQueryWidget::Contains;
                for( int i=0; i<5; i++ )
                {
                    if( condition == filterConditionToString( (MetaQueryWidget::FilterCondition)i ) )
                        filter.condition = (MetaQueryWidget::FilterCondition)i;
                }
            }
        }
    }

    QDomElement weightElement = e.firstChildElement( "weight" );
    if( !weightElement.isNull() )
    {
        weight = weightElement.attribute("value").toDouble();
    }

    return new Dynamic::GlobalBias( weight, filter );
}

QDomElement
Dynamic::GlobalBias::xml() const
{
    QDomDocument doc =
        PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc();

    QDomElement e = doc.createElement( "bias" );
    e.setAttribute( "type", "global" );

    QDomElement weight = doc.createElement( "weight" );
    weight.setAttribute( "value", QString::number( m_weight ) );
    e.appendChild( weight );

    QDomElement queryElement = doc.createElement( "query" );
    QDomElement filtersElement = doc.createElement( "filters" );
    QDomElement includeElement = doc.createElement( "include" );
    includeElement.setAttribute( "field", Meta::nameForField( m_filter.field ) );
    if( m_filter.condition == MetaQueryWidget::Contains )
        includeElement.setAttribute( "value", m_filter.value );
    else
        includeElement.setAttribute( "value", QString::number( m_filter.numValue ) );
    includeElement.setAttribute( "value2", QString::number( m_filter.numValue2 ) );
    includeElement.setAttribute( "compare", filterConditionToString( m_filter.condition ) );

    filtersElement.appendChild( includeElement );
    queryElement.appendChild( filtersElement );
    e.appendChild( queryElement );

    return e;
}


PlaylistBrowserNS::BiasWidget*
Dynamic::GlobalBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasGlobalWidget( this, parent );
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

bool
Dynamic::GlobalBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability* Dynamic::GlobalBias::collectionFilterCapability()
{
    return new Dynamic::GlobalBiasFilterCapability( this );
}



MetaQueryWidget::Filter
Dynamic::GlobalBias::filter() const
{
    return m_filter;
}

void
Dynamic::GlobalBias::setFilter( const MetaQueryWidget::Filter &filter)
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    if (m_qm)
        delete m_qm.data();

    // the whole bias/collection management is not very robust.
    // here we do our best to get a collection and a query maker
    // when in principle a bias should be collection independent.
    if( !m_collection )
        m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        m_qm = CollectionManager::instance()->queryMaker();
    else
        m_qm = m_collection->queryMaker();

    switch( filter.condition )
    {
    case MetaQueryWidget::Equals:
    case MetaQueryWidget::GreaterThan:
    case MetaQueryWidget::LessThan:
        m_qm.data()->addNumberFilter( filter.field, filter.numValue,
                               (Collections::QueryMaker::NumberComparison)filter.condition );
        break;
    case MetaQueryWidget::Between:
        m_qm.data()->beginAnd();
        m_qm.data()->addNumberFilter( filter.field, qMin(filter.numValue, filter.numValue2)-1,
                               Collections::QueryMaker::GreaterThan );
        m_qm.data()->addNumberFilter( filter.field, qMax(filter.numValue, filter.numValue2)+1,
                               Collections::QueryMaker::LessThan );
        m_qm.data()->endAndOr();
        break;
    case MetaQueryWidget::OlderThan:
        m_qm.data()->addNumberFilter( filter.field, QDateTime::currentDateTime().toTime_t() - filter.numValue,
                               Collections::QueryMaker::LessThan );
        break;

    case MetaQueryWidget::Contains:
        if( filter.field == 0 )
        {
            // simple search
            // TODO: split different words and make seperate searches
            m_qm.data()->beginOr();
            m_qm.data()->addFilter( Meta::valArtist,  filter.value );
            m_qm.data()->addFilter( Meta::valTitle,   filter.value );
            m_qm.data()->addFilter( Meta::valAlbum,   filter.value );
            m_qm.data()->addFilter( Meta::valGenre,   filter.value );
            m_qm.data()->addFilter( Meta::valUrl,     filter.value );
            m_qm.data()->addFilter( Meta::valComment, filter.value );
            m_qm.data()->addFilter( Meta::valLabel,   filter.value );
            m_qm.data()->endAndOr();
        }
        else
        {
            m_qm.data()->addFilter( filter.field, filter.value );
        }
        break;

    default:
        ;// the other conditions are only for the advanced playlist generator
    }

    m_qm.data()->setQueryType( Collections::QueryMaker::Custom );
    m_qm.data()->addReturnValue( Meta::valUniqueId );
    m_qm.data()->orderByRandom(); // as to not affect the amortized time

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
            SLOT(updateReady( QString, QStringList )), Qt::DirectConnection );
    connect( m_qm.data(), SIGNAL(queryDone()), SLOT(updateFinished()), Qt::DirectConnection );

    m_filter = filter;
    locker.unlock(); // because collectionUpdate also want's a lock

    collectionUpdated(); // force an update
}



double
Dynamic::GlobalBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context ) const
{
    Q_UNUSED( context );

    // TODO: for lastPlayed we should check the context and the songs already in the playlist

    double satisfiedCount = 0;
    foreach( Meta::TrackPtr t, playlist )
    {
        if( trackSatisfies( t ) )
            satisfiedCount++;
    }

    return  m_weight - (satisfiedCount / (double)playlist.size());
}


double Dynamic::GlobalBias::reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist,
        Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context ) const
{
    Q_UNUSED( context );

    // TODO: for lastPlayed we should check the context and the songs already in the playlist
    double offset = 1.0 / (double)oldPlaylist.size();

    bool prevSatisfied = trackSatisfies( oldPlaylist[newTrackPos] );

    if( trackSatisfies( newTrack ) && !prevSatisfied )
        return oldEnergy - offset;
    else if( !trackSatisfies( newTrack ) && prevSatisfied )
        return oldEnergy + offset;
    else
        return oldEnergy;
}


bool Dynamic::GlobalBias::trackSatisfies( Meta::TrackPtr t ) const
{
    // we only want the uid part:
    const QString uidString = t->uidUrl().mid( t->uidUrl().lastIndexOf( '/' ) + 1 );
    const QByteArray uid = uidString.toAscii();

    QMutexLocker locker( &((const_cast<GlobalBias *>(this))->m_mutex) );
    return m_property.contains( uid );
}


void Dynamic::GlobalBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm.data()->run();
}

void
Dynamic::GlobalBias::collectionUpdated()
{
    QMutexLocker locker( &m_mutex );
    m_property.clear();

    CollectionDependantBias::collectionUpdated();
}

void
Dynamic::GlobalBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)

    QMutexLocker locker( &m_mutex );

    const int protocolLength = QString( m_collection->uidUrlProtocol() + "://" ).length();

    m_property.clear();
    m_property.reserve( uids.size() );
    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = uidString.mid( protocolLength ).toAscii();
        m_property.insert( uid );
    }
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




Dynamic::NormalBias::NormalBias()
    : m_scale(0.0)
    , m_mu(0.0)
    , m_sigma(0.0)
    , m_field(0)
{
}


QDomElement
Dynamic::NormalBias::xml() const
{
    QDomDocument doc =
        PlaylistBrowserNS::DynamicModel::instance()->savedPlaylistDoc();

    QDomElement e = doc.createElement( "bias" );
    e.setAttribute( "type", "normal" );

    QDomElement fieldElement = doc.createElement( "field" );
    fieldElement.setAttribute( "value", QString::number( m_field ) );

    QDomElement meanElement = doc.createElement( "mean" );
    meanElement.setAttribute( "value", QString::number( m_mu ) );

    QDomElement scaleElement = doc.createElement( "scale" );
    scaleElement.setAttribute( "value", QString::number( m_scale ) );

    e.appendChild( fieldElement );
    e.appendChild( meanElement );
    e.appendChild( scaleElement );

    return e;
}

PlaylistBrowserNS::BiasWidget*
Dynamic::NormalBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasNormalWidget( this, parent );
}


void
Dynamic::NormalBias::setValue( double value )
{
    m_mu = value;
}

double
Dynamic::NormalBias::value() const
{
    return m_mu;
}


void
Dynamic::NormalBias::setField( qint64 field )
{
    m_field = field;
    setDefaultMu();
}

qint64
Dynamic::NormalBias::field() const
{
    return m_field;
}


void
Dynamic::NormalBias::setScale( double scale )
{
    m_scale = scale;
    m_sigma = sigmaFromScale(scale);
    debug() << "std. dev. = " << m_sigma;
}

double
Dynamic::NormalBias::scale()
{
    return m_scale;
}


double
Dynamic::NormalBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context ) const
{
    Q_UNUSED(context)

    QList<double> fields;

    foreach( Meta::TrackPtr t, playlist )
        fields += Meta::valueForField( m_field, t ).toDouble() - m_mu;

    qSort( fields );

    // this is the Kolmogorov-Smirnov goodness of fit test
    const double n = fields.size();
    double D = 0.0;
    double count = 0.0;

    for( int i = 0; i < fields.size(); ++i )
    {
        count += 1.0;
        if( i < fields.size()-1 && fields[i+1] == fields[i] )
            continue;

        double Dx = qAbs( gsl_cdf_gaussian_P( fields[i], m_sigma ) - (count / n) );
        D = qMax( Dx, D );
    }

    return D;
}

double
Dynamic::NormalBias::sigmaFromScale( double scale )
{
    if( scale < 0.0 )
        scale = 0.0;
    if( scale > 1.0 )
        scale = 1.0;

    double minStdDev = 0.0;
    double maxStdDev = 1.0;

    // Keep in mind: ~95% of values are within two standard deviations of the
    // mean. When scale = 1.0, the std. dev. is minStdDev. When scale = 0.0,
    // it's maxStdDev.
    if( m_field == Meta::valYear )
    {
        minStdDev = 0.5;
        maxStdDev = 10.0;
    }
    else if( m_field == Meta::valTrackNr )
    {
        minStdDev = 0.5;
        maxStdDev = 10.0;
    }
    else if( m_field == Meta::valDiscNr )
    {
        minStdDev = 0.5;
        maxStdDev = 5.0;
    }
    else if( m_field == Meta::valBpm )
    {
        minStdDev = 60;
        maxStdDev = 200.0;
    }
    else if( m_field == Meta::valLength )
    {
        minStdDev = 60;
        maxStdDev = 60.0 * 60.0 * 2.0; // two hours
    }
    else if( m_field == Meta::valBitrate )
    {
        minStdDev = 60;
        maxStdDev = 400.0;
    }
    else if( m_field == Meta::valSamplerate )
    {
        minStdDev = 8000;
        maxStdDev = 48000.0;
    }
    else if( m_field == Meta::valFilesize )
    {
        minStdDev = 0;
        maxStdDev = 200000.0;
    }
    else if( m_field == Meta::valScore )
    {
        minStdDev = 1.0;
        maxStdDev = 50.0;
    }
    else if( m_field == Meta::valRating )
    {
        minStdDev = 0.5;
        maxStdDev = 2.5;
    }
    else if( m_field == Meta::valLength )
    {
        minStdDev = 10.0;
        maxStdDev = 240.0;
    }
    else if( m_field == Meta::valCreateDate ||
             m_field == Meta::valFirstPlayed ||
             m_field == Meta::valLastPlayed )
    {
        minStdDev = 3600.0;   // one hour
        maxStdDev = 604800.0; // one week
    }
    else if( m_field == Meta::valPlaycount )
    {
        minStdDev = 0.5;
        maxStdDev = 50.0;
    }

    // linear interpolation between min and max std. dev.
    return minStdDev + (maxStdDev - minStdDev) * (1.0 - scale);
}

void
Dynamic::NormalBias::setDefaultMu()
{
    if( m_field == Meta::valYear )
        m_mu = 1976.0;
    else if( m_field == Meta::valPlaycount )
        m_mu = 0.0;
    else if( m_field == Meta::valRating )
        m_mu = 0.0;
    else if( m_field == Meta::valScore )
        m_mu = 0.0;
    else if( m_field == Meta::valLength )
        m_mu = 180.0;
    else if( m_field == Meta::valTrackNr )
        m_mu = 1.0;
    else if( m_field == Meta::valDiscNr )
        m_mu = 1.0;
    else if( m_field == Meta::valFirstPlayed )
        m_mu = 0.0;
    else if( m_field == Meta::valLastPlayed )
        m_mu = 0.0;
}


