/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#include "Constraint.h"
#include "ConstraintSolver.h"

#include "core/collections/MetaQueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistModel.h"

#include <KRandom>
#include <QHash>
#include <QMutexLocker>
#include <QStringList>
#include <QTimer>
#include <threadweaver/ThreadWeaver.h>
#include <cmath>
#include <typeinfo>

const int APG::ConstraintSolver::QUALITY_RANGE = 10;

APG::ConstraintSolver::ConstraintSolver( ConstraintNode* r, int qualityFactor )
        : m_constraintTreeRoot( r )
        , m_domainReductionFailed( false )
        , m_readyToRun( false )
        , m_abortRequested( false )
        , m_playlistEntropy( 0.0 )
        , m_finalSatisfaction( 0.0 )
{
    m_serialNumber = KRandom::random();

    // TODO: really adjust according to qualityFactor
    double x = (double)qualityFactor/(double)QUALITY_RANGE;
    debug() << "Constraint Solver quality factor:" << x;
    m_satisfactionThreshold = 0.95;
    m_qualityFactor = 1.0;
    m_maxCoolingIterations = 500;
    m_maxMutationIterations = 30;
    m_maxSwapIterations = 30;

    m_minPlaylistSize = 5;
    m_maxPlaylistSize = 200;

    if ( !m_constraintTreeRoot ) {
        error() << "No constraint tree was passed to the solver.  Aborting.";
        m_readyToRun = true;
        m_abortRequested = true;
        return;
    }

    m_qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    if ( m_qm ) {
        debug() << "New ConstraintSolver with serial number" << m_serialNumber;
        m_qm->setQueryType( Collections::QueryMaker::Track );
        m_qm->orderByRandom();
        connect( m_qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( receiveQueryMakerData( QString, Meta::TrackList ) ), Qt::QueuedConnection );
        connect( m_qm, SIGNAL( queryDone() ), this, SLOT( receiveQueryMakerDone() ), Qt::QueuedConnection );
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
        m_qm = 0;
    }
}

Meta::TrackList
APG::ConstraintSolver::getSolution() const
{
    return m_solvedPlaylist;
}

double
APG::ConstraintSolver::finalSatisfaction() const
{
    return m_finalSatisfaction;
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
        m_qm = 0;
    }
    m_abortRequested = true;
}

bool
APG::ConstraintSolver::success() const
{
    return !m_abortRequested;
}

void
APG::ConstraintSolver::run()
{
    if ( !m_readyToRun ) {
        error() << "DANGER WILL ROBINSON!  A ConstraintSolver (serial no:" << m_serialNumber << ") tried to run before its QueryMaker finished!";
        m_abortRequested = true;
        return;
    }

    if ( m_domain.empty() ) {
        debug() << "The QueryMaker returned no tracks";
        return;
    }

    debug() << "Running ConstraintSolver" << m_serialNumber;

    // set up a random initial playlist
    int pls = m_constraintTreeRoot->suggestInitialPlaylistSize();
    if ( pls > 0 ) {
        m_solvedPlaylist += m_domain.mid( 0, pls );
    } else {
        m_solvedPlaylist += m_domain.mid( 0, 30 );
    }

    // SIMULATED ANNEALING LOOP
    int cooliter = 0;
    double satisfaction = m_constraintTreeRoot->satisfaction( m_solvedPlaylist );
    double temperature = m_qualityFactor * ( 1.0 - satisfaction );
    while ( ( satisfaction < m_satisfactionThreshold ) && ( cooliter++ < m_maxCoolingIterations ) && ( !m_abortRequested ) ) {
        if ( ( cooliter % 25 ) == 0 ) {
            debug() << "step" << cooliter << "satisfaction" << satisfaction << "temperature" << temperature << "entropy" << m_playlistEntropy;
        }

        int mutationiter = 0;
        while ( ( satisfaction < m_satisfactionThreshold ) && ( mutationiter++ < m_maxMutationIterations ) && ( !m_abortRequested ) ) {
            if ( ( KRandom::random() % 10 ) < 8 ) // TODO: vary voting weight according to satisfaction
                satisfaction += mutateRandom( temperature );
            else
                satisfaction += mutateByVote( temperature );
        }

        int swapiter = 0;
        while ( ( satisfaction < m_satisfactionThreshold ) && ( swapiter++ < m_maxSwapIterations ) && ( !m_abortRequested ) ) {
            satisfaction += improveBySwapping();
        }

        // internal safety check
        // failure means that (at least) one of constraints has incorrect math in the delta functions or the internal state update functions
        double oldsatisfaction = satisfaction;
        satisfaction = m_constraintTreeRoot->satisfaction( m_solvedPlaylist );
        if ( fabs( oldsatisfaction - satisfaction ) > 1e-9 ) {
            warning() << "satisfaction disparity! expected:" << oldsatisfaction << "but true:" << satisfaction;
        }

        // cool the temperature
        if ( m_playlistEntropy > 0.0 ) {
            temperature = m_qualityFactor * (( 1.0 - satisfaction ) / m_playlistEntropy );
        } else {
            temperature = m_qualityFactor * ( 1.0 - satisfaction );
        }

        emit incrementProgress();
    }

#ifndef KDE_NO_DEBUG_OUTPUT
    m_constraintTreeRoot->audit( m_solvedPlaylist );
#endif

    if ( !m_abortRequested ) {
        debug() << "ConstraintSolver" << m_serialNumber << "finished with satisfaction" << satisfaction;
        m_finalSatisfaction = satisfaction;
    } else {
        debug() << "ConstraintSolver" << m_serialNumber << "aborted";
        m_finalSatisfaction = 0.0;
        m_solvedPlaylist.clear();
    }
}

void
APG::ConstraintSolver::receiveQueryMakerData( QString collId, Meta::TrackList results )
{
    Q_UNUSED( collId );
    m_domainMutex.lock();
    m_domain += results;
    m_domainMutex.unlock();
}

void
APG::ConstraintSolver::receiveQueryMakerDone()
{
    if (( m_domain.size() > 0 ) || m_domainReductionFailed ) {
        if ( m_domain.size() <= 0 ) {
            Amarok::Components::logger()->shortMessage( i18n("The playlist generator failed to load any tracks from the collection.") );
        }
        m_readyToRun = true;
        m_qm->deleteLater();
        m_qm = 0;
        emit readyToRun();
    } else {
        Amarok::Components::logger()->longMessage( i18n("There are no tracks that match all constraints.  The playlist generator will find the tracks that match best, but you may want to consider loosening the constraints to find more tracks.") );
        m_domainReductionFailed = true;
        m_qm->reset();
        m_qm->setQueryType( Collections::QueryMaker::Track );
        m_qm->orderByRandom();
        m_qm->run();
    }
}

Meta::TrackPtr
APG::ConstraintSolver::randomTrackFromDomain() const
{
    return m_domain.at( KRandom::random() % m_domain.size() );
}

double
APG::ConstraintSolver::mutateRandom( const double temperature )
{
    int op = KRandom::random() % 3;
    Meta::TrackPtr t = randomTrackFromDomain();
    int place = 0;
    double satisfactionDelta = 0.0;
    bool changeFailed = false;
    // Test the change
    switch ( op ) {
        case 0:
            if ( m_solvedPlaylist.size() < m_maxPlaylistSize ) {
                place = KRandom::random() % ( m_solvedPlaylist.size() + 1 );
                satisfactionDelta = m_constraintTreeRoot->deltaS_insert( m_solvedPlaylist, t, place );
            } else {
                changeFailed = true;
            }
            break;
        case 1:
            place = KRandom::random() % ( m_solvedPlaylist.size() );
            satisfactionDelta = m_constraintTreeRoot->deltaS_replace( m_solvedPlaylist, t, place );
            break;
        case 2:
            if ( m_solvedPlaylist.size() > m_minPlaylistSize ) {
                place = KRandom::random() % ( m_solvedPlaylist.size() );
                satisfactionDelta = m_constraintTreeRoot->deltaS_delete( m_solvedPlaylist, place );
            } else {
                changeFailed = true;
            }
            break;
    }

    if ( changeFailed ) {
        return 0.0;
    }

    // test if it's acceptable
    double decisionFactor = ( double )KRandom::random() / ( double )RAND_MAX;
    double acceptance = qMin( 1.0, exp( satisfactionDelta / temperature ) );

    // make the change if it's acceptable
    if ( decisionFactor < acceptance ) {
        switch ( op ) {
        case ConstraintNode::OperationInsert:
            m_constraintTreeRoot->insertTrack( m_solvedPlaylist, t, place );
            m_solvedPlaylist.insert( place, t );
            break;
        case ConstraintNode::OperationReplace:
            m_constraintTreeRoot->replaceTrack( m_solvedPlaylist, t, place );
            m_solvedPlaylist.replace( place, t );
            break;
        case ConstraintNode::OperationDelete:
            m_constraintTreeRoot->deleteTrack( m_solvedPlaylist, place );
            m_solvedPlaylist.removeAt( place );
            break;
        }
        m_playlistEntropy -= log( acceptance );
        return satisfactionDelta;
    } else {
        return 0.0;
    }
}

double
APG::ConstraintSolver::mutateByVote( const double temperature )
{
    ConstraintNode::Vote* vote = m_constraintTreeRoot->vote( m_solvedPlaylist, m_domain );
    if ( vote == 0 )
        return 0.0;

    // perform the operation chosen by voting
    bool changeFailed = false;
    double satisfactionDelta = 0.0;
    switch ( vote->operation ) {
        case ConstraintNode::OperationInsert:
            if ( m_solvedPlaylist.size() < m_maxPlaylistSize ) {
                satisfactionDelta = m_constraintTreeRoot->deltaS_insert( m_solvedPlaylist, vote->track, vote->place );
            } else {
                changeFailed = true;
            }
            break;
        case ConstraintNode::OperationReplace:
            satisfactionDelta = m_constraintTreeRoot->deltaS_replace( m_solvedPlaylist, vote->track, vote->place );
            break;
        case ConstraintNode::OperationDelete:
            if ( m_solvedPlaylist.size() > m_minPlaylistSize ) {
                satisfactionDelta = m_constraintTreeRoot->deltaS_delete( m_solvedPlaylist, vote->place );
            } else {
                changeFailed = true;
            }
            break;
    }

    if ( changeFailed )
        return 0.0;

    // test if it's acceptable
    double decisionFactor = ( double )KRandom::random() / ( double )RAND_MAX;
    double acceptance = qMin( 1.0, exp( satisfactionDelta / temperature ) );

    // make the change if it's acceptable
    if ( decisionFactor < acceptance ) {
        switch ( vote->operation ) {
        case ConstraintNode::OperationInsert:
            m_constraintTreeRoot->insertTrack( m_solvedPlaylist, vote->track, vote->place );
            m_solvedPlaylist.insert( vote->place, vote->track );
            break;
        case ConstraintNode::OperationReplace:
            m_constraintTreeRoot->replaceTrack( m_solvedPlaylist, vote->track, vote->place );
            m_solvedPlaylist.replace( vote->place, vote->track );
            break;
        case ConstraintNode::OperationDelete:
            m_constraintTreeRoot->deleteTrack( m_solvedPlaylist, vote->place );
            m_solvedPlaylist.removeAt( vote->place );
            break;
        }
        m_playlistEntropy -= log( acceptance );
        return satisfactionDelta;
    } else {
        return 0.0;
    }
}

double
APG::ConstraintSolver::improveBySwapping()
{
    int i = KRandom::random() % ( m_solvedPlaylist.size() );
    int j = KRandom::random() % ( m_solvedPlaylist.size() );
    if ( m_solvedPlaylist[i] == m_solvedPlaylist[j] ) {
        return 0.0;
    }

    double satisfactionDelta = m_constraintTreeRoot->deltaS_swap( m_solvedPlaylist, i, j );
    if ( satisfactionDelta >= 0.0 ) {
        m_constraintTreeRoot->swapTracks( m_solvedPlaylist, i, j );
        m_solvedPlaylist.swap( i, j );
        return satisfactionDelta;
    }
    return 0.0;
}
