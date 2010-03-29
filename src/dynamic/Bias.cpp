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
#include "core-implementations/collections/support/CollectionManager.h"
#include "CustomBias.h"
#include "core/support/Debug.h"
#include "DynamicBiasWidgets.h"
#include "DynamicModel.h"
#include "core/meta/support/MetaConstants.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"
#include "core-implementations/collections/support/XmlQueryWriter.h"

#include <QMutexLocker>

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
        double weight = 0.0;
        XmlQueryReader::Filter filter;


        QDomElement queryElement = e.firstChildElement( "query" );
        if( !queryElement.isNull() )
        {
            // I don't actually need a qm from XmlQueryReader, I just want the filters.
            Collections::QueryMaker* dummyQM = new Collections::MetaQueryMaker( QList<Collections::QueryMaker*>() );

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
        Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context )
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

Dynamic::GlobalBias::GlobalBias( double weight, XmlQueryReader::Filter filter )
    : m_qm(0)
{
    setWeight( weight );
    setQuery( filter );
}

Dynamic::GlobalBias::GlobalBias( Collections::Collection* coll, double weight, XmlQueryReader::Filter filter )
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
    weight.setAttribute( "value", QString::number( m_weight ) );

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

bool
Dynamic::GlobalBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability* Dynamic::GlobalBias::collectionFilterCapability()
{
    return new Dynamic::GlobalBiasFilterCapability( this );
}


void
Dynamic::GlobalBias::setQuery( XmlQueryReader::Filter filter )
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    Collections::QueryMaker* qm;

    if( !m_collection )
        m_collection = CollectionManager::instance()->primaryCollection();

    qm = m_collection->queryMaker();

    m_qm = new Collections::XmlQueryWriter( qm,
            QDomDocument() );

    if( filter.field != 0 )
    {
        if( filter.compare == -1 )
            m_qm->addFilter( filter.field, filter.value );
        else
            m_qm->addNumberFilter( filter.field, filter.value.toLongLong(),
                    (Collections::QueryMaker::NumberComparison)filter.compare );
    }

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL(newResultReady( QString, QStringList )),
            SLOT(updateReady( QString, QStringList )), Qt::DirectConnection );
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

    // we only want the uid part:
    const QString uidString = t->uidUrl().mid( t->uidUrl().lastIndexOf( '/' ) );
    const QByteArray uid = uidString.toAscii();

    return m_property.contains( uid );
}


void Dynamic::GlobalBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

void
Dynamic::GlobalBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)

    QMutexLocker locker( &m_mutex );

    int protocolLength =
        (QString(m_collection->uidUrlProtocol()) + "://").length();

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
Dynamic::NormalBias::energy( const Meta::TrackList& playlist, const Meta::TrackList& context )
{
    Q_UNUSED(context)


    QList<double> fields;

    foreach( Meta::TrackPtr t, playlist )
        fields += relevantField(t) - m_mu;

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
Dynamic::NormalBias::relevantField( Meta::TrackPtr track )
{
    if( !track )
        return m_mu;
    if( m_field == Meta::valYear && track->year() )
        return (double)track->year()->name().toInt();
    if( m_field == Meta::valPlaycount )
        return (double)track->playCount();
    if( m_field == Meta::valRating )
        return (double)track->rating();
    if( m_field == Meta::valScore )
        return track->score();
    if( m_field == Meta::valLength )
        return (double)track->length();
    if( m_field == Meta::valTrackNr )
        return (double)track->trackNumber();
    if( m_field == Meta::valDiscNr )
        return (double)track->discNumber();
    if( m_field == Meta::valFirstPlayed )
        return (double)track->firstPlayed();
    if( m_field == Meta::valLastPlayed )
        return (double)track->lastPlayed();

    return m_mu;
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
    else if( m_field == Meta::valPlaycount )
    {
        minStdDev = 0.5;
        maxStdDev = 50.0;
    }
    else if( m_field == Meta::valRating )
    {
        minStdDev = 0.5;
        maxStdDev = 2.5;
    }
    else if( m_field == Meta::valScore )
    {
        minStdDev = 1.0;
        maxStdDev = 50.0;
    }
    else if( m_field == Meta::valLength )
    {
        minStdDev = 10.0;
        maxStdDev = 240.0;
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
    else if( m_field == Meta::valFirstPlayed )
    {
        minStdDev = 3600.0;   // one hour
        maxStdDev = 604800.0; // one week
    }
    else if( m_field == Meta::valLastPlayed )
    {
        minStdDev = 3600.0;   // one hour
        maxStdDev = 604800.0; // one week
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


