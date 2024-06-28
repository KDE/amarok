/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "APG::ConstraintSolver"


#include "ConstraintSolver.h"

#include "Constraint.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/logger/Logger.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistModel.h"

#include <KLocalizedString>

#include <QHash>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QStringList>
#include <QTimer>
#include <ThreadWeaver/ThreadWeaver>

#include <algorithm> // STL algorithms
#include <cmath>
#include <typeinfo>

const int APG::ConstraintSolver::QUALITY_RANGE = 10;

APG::ConstraintSolver::ConstraintSolver( ConstraintNode* r, int qualityFactor )
        : QObject()
        , ThreadWeaver::Job()
        , m_satisfactionThreshold( 0.95 )
        , m_finalSatisfaction( 0.0 )
        , m_constraintTreeRoot( r )
        , m_domainReductionFailed( false )
        , m_readyToRun( false )
        , m_abortRequested( false )
        , m_maxGenerations( 100 )
        , m_populationSize( 40 )
        , m_suggestedPlaylistSize( 15 )
{
    Q_UNUSED( qualityFactor); // FIXME

    m_serialNumber = QRandomGenerator::global()->generate();

    if ( !m_constraintTreeRoot ) {
        error() << "No constraint tree was passed to the solver.  Aborting.";
        m_readyToRun = true;
        m_abortRequested = true;
        return;
    }

    m_qm = CollectionManager::instance()->queryMaker();
    if ( m_qm ) {
        debug() << "New ConstraintSolver with serial number" << m_serialNumber;
        m_qm->setQueryType( Collections::QueryMaker::Track );
        connect( m_qm, &Collections::QueryMaker::newTracksReady,
                 this, &APG::ConstraintSolver::receiveQueryMakerData, Qt::QueuedConnection );
        connect( m_qm, &Collections::QueryMaker::queryDone,
                 this, &APG::ConstraintSolver::receiveQueryMakerDone, Qt::QueuedConnection );
        m_constraintTreeRoot->initQueryMaker( m_qm );
        m_qm->run();
    } else {
        debug() << "The ConstraintSolver could not find any queryable collections.  No results will be returned.";
        m_readyToRun = true;
        m_abortRequested = true;
    }
}

APG::ConstraintSolver::~ConstraintSolver()
{
    if ( m_qm ) {
        m_qm->abortQuery();
        m_qm->deleteLater();
        m_qm = nullptr;
    }
}

Meta::TrackList
APG::ConstraintSolver::getSolution() const
{
    return m_solvedPlaylist;
}

bool
APG::ConstraintSolver::satisfied() const
{
    return m_finalSatisfaction > m_satisfactionThreshold;
}

bool
APG::ConstraintSolver::canBeExecuted()
{

    /* This is a hopefully superfluous check to ensure that the Solver job
     * doesn't get run until it's ready (ie, when QueryMaker has finished).
     * This shouldn't ever return false, because hopefully the job won't even
     * get queued until it's ready to run.  See the comments in
     * Preset::queueSolver() for more information. -- sth */

    return m_readyToRun;
}

void
APG::ConstraintSolver::requestAbort()
{
    if ( m_qm ) {
        m_qm->abortQuery();
        m_qm->deleteLater();
        m_qm = nullptr;
    }
    m_abortRequested = true;
}

bool
APG::ConstraintSolver::success() const
{
    return !m_abortRequested;
}

void
APG::ConstraintSolver::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);

    if ( !m_readyToRun ) {
        error() << "DANGER WILL ROBINSON!  A ConstraintSolver (serial no:" << m_serialNumber << ") tried to run before its QueryMaker finished!";
        m_abortRequested = true;
        return;
    }

    if ( m_domain.empty() ) {
        debug() << "The QueryMaker returned no tracks";
        return;
    } else {
        debug() << "Domain has" << m_domain.size() << "tracks";
    }

    debug() << "Running ConstraintSolver" << m_serialNumber;

    Q_EMIT totalSteps( m_maxGenerations );

    // GENETIC ALGORITHM LOOP
    Population population;
    quint32 generation = 0;
    Meta::TrackList* best = nullptr;
    while ( !m_abortRequested && ( generation < m_maxGenerations ) ) {
        quint32 s = m_constraintTreeRoot->suggestPlaylistSize();
        m_suggestedPlaylistSize = (s > 0) ? s : m_suggestedPlaylistSize;
        fill_population( population );
        best = find_best( population );
        if ( population.value( best ) < m_satisfactionThreshold ) {
            select_population( population, best );
            mutate_population( population );
            generation++;
            Q_EMIT incrementProgress();
        } else {
            break;
        }
    }

    if( best )
    {
        debug() << "solution at" << (void*)(best);

        m_solvedPlaylist = best->mid( 0 );
        m_finalSatisfaction = m_constraintTreeRoot->satisfaction( m_solvedPlaylist );
    }

    /* clean up */
    Population::iterator it = population.begin();
    while ( it != population.end() ) {
        delete it.key();
        it = population.erase( it );
    }

    Q_EMIT endProgressOperation( this );
}

void APG::ConstraintSolver::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void APG::ConstraintSolver::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
APG::ConstraintSolver::receiveQueryMakerData( const Meta::TrackList &results )
{
    m_domainMutex.lock();
    m_domain += results;
    m_domainMutex.unlock();
}

void
APG::ConstraintSolver::receiveQueryMakerDone()
{
    m_qm->deleteLater();
    m_qm = nullptr;

    if (( m_domain.size() > 0 ) || m_domainReductionFailed ) {
        if ( m_domain.size() <= 0 ) {
            Amarok::Logger::shortMessage( i18n("The playlist generator failed to load any tracks from the collection.") );
        }
        m_readyToRun = true;
        Q_EMIT readyToRun();
    } else {
        Amarok::Logger::longMessage(
                    i18n("There are no tracks that match all constraints. " \
                         "The playlist generator will find the tracks that match best, " \
                "but you may want to consider loosening the constraints to find more tracks.") );
        m_domainReductionFailed = true;

        // need a new query maker without constraints
        m_qm = CollectionManager::instance()->queryMaker();
        if ( m_qm ) {
            connect( m_qm, &Collections::QueryMaker::newTracksReady,
                     this, &APG::ConstraintSolver::receiveQueryMakerData, Qt::QueuedConnection );
            connect( m_qm, &Collections::QueryMaker::queryDone,
                     this, &APG::ConstraintSolver::receiveQueryMakerDone, Qt::QueuedConnection );

            m_qm->setQueryType( Collections::QueryMaker::Track );
            m_qm->run();
        }
    }
}

void
APG::ConstraintSolver::fill_population( Population& population )
{
    for ( int i = population.size(); quint32(i) < m_populationSize; i++ ) {
        Meta::TrackList* tl = new Meta::TrackList( sample( m_domain, playlist_size() ) );
        double s = m_constraintTreeRoot->satisfaction( (*tl) );
        population.insert( tl, s );
    }
}

Meta::TrackList* APG::ConstraintSolver::find_best(const APG::ConstraintSolver::Population& population ) const
{
    Population::const_iterator it = std::max_element( population.constBegin(), population.constEnd(), &pop_comp );
    return it.key();
}

void
APG::ConstraintSolver::select_population( APG::ConstraintSolver::Population& population, Meta::TrackList* best )
{
    Population::Iterator it = population.begin();
    while ( it != population.end() ) {
        if ( it.key() == best ) {
            ++it;// Always keep the best solution, no matter how bad it is
            if ( it == population.end() )
                break;
        }
        
        if ( select( it.value() ) ) {
            ++it;
        } else {
            delete it.key();
            it = population.erase( it );
        }
    }
}

void
APG::ConstraintSolver::mutate_population( APG::ConstraintSolver::Population& population )
{
    if ( population.size() < 1 )
        return;
    
    const double mutantPercentage = 0.35; // TODO: tune this parameter
    
    QList<Meta::TrackList*> parents( population.keys() );
    int maxMutants = (int)( mutantPercentage * (double)(m_populationSize) );
    for ( int i = parents.size(); i < maxMutants; i++ ) {
        int idx = QRandomGenerator::global()->generate() % parents.size();
        Meta::TrackList* child = new Meta::TrackList( *(parents.at( idx )) );
        int op = QRandomGenerator::global()->generate() % 5;
        int s = child->size();
        switch (op) {
            case 0:
                child->removeAt( QRandomGenerator::global()->generate() % s );
                break;
            case 1:
                child->insert( QRandomGenerator::global()->generate() % ( s + 1 ), random_track_from_domain() );
                break;
            case 2:
                child->replace( QRandomGenerator::global()->generate() % s, random_track_from_domain() );
                break;
            case 3:
                child->swapItemsAt( QRandomGenerator::global()->generate() % s, QRandomGenerator::global()->generate() % s );
                break;
            case 4:
                child = crossover( child, parents.at( QRandomGenerator::global()->generate() % parents.size() ) );
                break;
            default:
                (void)0; // effectively a no-op. the default is here so that the compiler doesn't complain about missing default in switch
        }
        population.insert( child, m_constraintTreeRoot->satisfaction( *child ) );
    }
    return;
}

Meta::TrackList*
APG::ConstraintSolver::crossover( Meta::TrackList* top, Meta::TrackList* bot ) const
{
    const double crossoverPt = 0.5; // TODO: choose different values

    int topV = (int)( crossoverPt * (double)top->size() );
    int botV = (int)( crossoverPt * (double)bot->size() );

    Meta::TrackList* newlist = new Meta::TrackList( top->mid( 0, topV ) );
    newlist->append( bot->mid( botV ) );

    delete top;
    return newlist;
}

bool
APG::ConstraintSolver::pop_comp( double a, double b )
{
    return ( a < b );
}

Meta::TrackPtr
APG::ConstraintSolver::random_track_from_domain() const
{
    return m_domain.at( QRandomGenerator::global()->generate() % m_domain.size() );
}

Meta::TrackList
APG::ConstraintSolver::sample( Meta::TrackList domain, const int sampleSize ) const
{
    std::shuffle( domain.begin(), domain.end(), std::default_random_engine( QDateTime::currentMSecsSinceEpoch() ) );
    return domain.mid( 0, sampleSize );
}

quint32
APG::ConstraintSolver::playlist_size() const
{
    return rng_poisson( (double)m_suggestedPlaylistSize );
}

bool
APG::ConstraintSolver::select( const double satisfaction ) const
{
    double x = (double)QRandomGenerator::global()->generate()/(double)std::numeric_limits<quint32>::max();
    const double scale = -30.0; // TODO: make adjustable
    return ( x < 1.0 / ( 1.0 + exp( scale * (satisfaction-0.8) ) ) );
}

void
APG::ConstraintSolver::dump_population( const Population& population ) const
{
    DEBUG_BLOCK
    for ( Population::ConstIterator it = population.constBegin(); it != population.constEnd(); ++it ) {
        Meta::TrackList* tl = it.key();
        debug() << "at" << (void*)(tl) << "satisfaction:" << it.value();
        for ( Meta::TrackPtr t : (*tl) ) {
            debug() << "\ttrack:" << t->prettyName();
        }
    }
}

double
APG::ConstraintSolver::rng_gaussian( const double mu, const double sigma ) const
{
    /* adapted from randist/gauss.c in GNU Scientific Library 1.14 */
    double u, v, x, y, Q;
    const double  s =  0.449871;
    const double  t = -0.386595;
    const double  a =  0.19600;
    const double  b =  0.25472;
    const double r1 =  0.27597;
    const double r2 =  0.27846;

    do {
        u = 1 - rng_uniform();
        v = ( rng_uniform() - 0.5 ) * 1.7156;
        x = u - s;
        y = fabs (v) - t;
        Q = x * x + y * (a * y - b * x);
    } while (Q >= r1 && (Q > r2 || v * v > -4 * u * u * log (u)));

    return mu + ( sigma * (v / u) );
}

quint32
APG::ConstraintSolver::rng_poisson( const double mu ) const
{
    if ( mu >= 25.0 ) {
        double v = rng_gaussian( mu, sqrt( mu ) );
        return ( v < 0.0 ) ? 0 : (quint32)v;
    }

    const double emu = exp( -mu );
    double prod = 1.0;
    quint32 k = 0;

    do {
        prod *= rng_uniform();
        k++;
    }
    while ( prod > emu );

    return k - 1;
}

double
APG::ConstraintSolver::rng_uniform() const
{
    return ( (double)QRandomGenerator::global()->generate() / (double)std::numeric_limits<quint32>::max() );
}
