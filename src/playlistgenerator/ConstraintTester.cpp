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

#define DEBUG_PREFIX "APG::ConstraintTester"

#include "ConstraintTester.h"

#include "Constraint.h"
#include "ConstraintNode.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <KRandom>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <threadweaver/ThreadWeaver.h>
#include <cmath>

APG::ConstraintTester::ConstraintTester( Collections::Collection* c, ConstraintNode* r )
        : m_readyToRun( false )
        , m_abortRequested( false )
{
    m_serialNumber = KRandom::random();

    if ( c ) {
        debug() << "New ConstraintTester with serial number" << m_serialNumber;
        m_qm = c->queryMaker();
        m_qm->setQueryType( Collections::QueryMaker::Track );
        m_qm->orderByRandom();
        // TODO: limit number of tracks returned
        connect( m_qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( receiveQueryMakerData( QString, Meta::TrackList ) ), Qt::QueuedConnection );
        connect( m_qm, SIGNAL( queryDone() ), this, SLOT( receiveQueryMakerDone() ), Qt::QueuedConnection );
        m_qm->run();
    } else {
        debug() << "No collection was passed to the ConstraintTester.  No results will be returned.";
        m_readyToRun = true;
        m_abortRequested = true;
    }

    findConstraints( r );
}

APG::ConstraintTester::~ConstraintTester()
{
    debug() << "ConstraintTester with serial number" << m_serialNumber << "has finished and is being deleted";
    if ( m_qm ) {
        m_qm->abortQuery();
        m_qm->deleteLater();
        m_qm = 0;
    }
}

bool
APG::ConstraintTester::canBeExecuted()
{

    /* This is a hopefully superfluous check to ensure that the Solver job
     * doesn't get run until it's ready (ie, when QueryMaker has finished).
     * This shouldn't ever return false, because hopefully the job won't even
     * get queued until it's ready to run.  See the comments in
     * Preset::queueSolver() for more information. -- sth */

    return m_readyToRun;
}

void
APG::ConstraintTester::requestAbort()
{
    if ( m_qm ) {
        m_qm->abortQuery();
        m_qm->deleteLater();
        m_qm = 0;
    }
    m_abortRequested = true;
}

bool
APG::ConstraintTester::success() const
{
    return !m_abortRequested;
}

void
APG::ConstraintTester::run()
{
    // TODO: handle abort requests

    if (m_domain.size() < 1)
        return;

    m_activePlaylist = m_domain.mid( 0, 100 );

    foreach ( ConstraintNode* c, m_constraints ) {
        int errorCount = testConstraint( c->getName(), c );
        debug() << errorCount << "errors encountered in" << c->getName();
    }
}

void
APG::ConstraintTester::receiveQueryMakerData( QString collId, Meta::TrackList results )
{
    Q_UNUSED( collId );
    m_domainMutex.lock();
    m_domain += results;
    m_domainMutex.unlock();
}

void
APG::ConstraintTester::receiveQueryMakerDone()
{
    m_readyToRun = true;
    m_qm->deleteLater();
    m_qm = 0;
    emit readyToRun();
}

void
APG::ConstraintTester::findConstraints( ConstraintNode* n )
{
    if ( n->getNodeType() == ConstraintNode::ConstraintType ) {
        m_constraints.append( n );
    } else if ( n->getNodeType() == ConstraintNode::ConstraintGroupType ) {
        for (int i=0; i < n->getRowCount(); i++) {
            findConstraints( n->getChild( i ) );
        }
    }
}

int
APG::ConstraintTester::testConstraint( const QString& name, ConstraintNode* c )
{
    const int testCount = 10;
    int iteration = 0;
    int errors = 0;

    while ( ( !m_abortRequested ) && ( iteration++ < testCount ) && ( errors < 20 ) ) {
        debug() << name << "test cycle" << iteration << "of" << testCount;

        double s = c->satisfaction( m_activePlaylist );
        if ( ( s < 0.0 ) || ( s > 1.0 ) ) {
            debug() << name << "computed invalid satisfaction:" << s;
            errors++;
        }

        // this loop tests the delta functions
        int loops = ( KRandom::random() % 100 ) + 100;
        for ( int i = 0; i < loops; i++ ) {
            // make sure the playlist has at least 5 tracks in it
            if ( m_activePlaylist.size() < 5 ) {
                for (int i = 0; i < 95; i++ )
                    m_activePlaylist.append( randomTrackFromDomain() );
            }

            Meta::TrackPtr t = randomTrackFromDomain();
            int op = KRandom::random() % 4;
            int place = KRandom::random() % ( m_activePlaylist.size() );
            int other = KRandom::random() % ( m_activePlaylist.size() );
            switch ( op ) {
                case 0:
                    place = KRandom::random() % ( m_activePlaylist.size() + 1 ); // need to be able to append, too
                    s += c->deltaS_insert( m_activePlaylist, t, place );
                    m_activePlaylist.insert( place, t );
                    break;
                case 1:
                    s += c->deltaS_replace( m_activePlaylist, t, place );
                    m_activePlaylist.replace( place, t );
                    break;
                case 2:
                    s += c->deltaS_delete( m_activePlaylist, place );
                    m_activePlaylist.removeAt( place );
                    break;
                case 3:
                    s += c->deltaS_swap( m_activePlaylist, place, other );
                    m_activePlaylist.swap( place, other );
                    break;
            }
            double newS = c->satisfaction( m_activePlaylist );
            if ( qAbs( newS - s ) > 1e-9 ) { // ignore floating-point precision errors
                errors++;
                switch ( op ) {
                    case 0:
                        debug() << name << "insert delta failed" << t->prettyName() << place << m_activePlaylist.size();
                        break;
                    case 1:
                        debug() << name << "replace delta failed" << t->prettyName() << place << m_activePlaylist.size();
                        break;
                    case 2:
                        debug() << name << "delete delta failed" << place << m_activePlaylist.size();
                        break;
                    case 3:
                        debug() << name << "swap delta failed" << place << other << m_activePlaylist.size();
                        break;
                }
                debug() << "\tsatisfaction expected:" << s << "true:" << newS;
                s = newS;
            }
        }

        // make sure the playlist has at least 5 tracks in it
        if ( m_activePlaylist.size() < 5 ) {
            for (int i = 0; i < 95; i++ )
                m_activePlaylist.append( randomTrackFromDomain() );
        }
        // test the incremental updaters individually
        double iuS = s;
        loops = ( KRandom::random() % 50 ) + 50;
        for (int i = 0; i < loops; i++ ) {
            Meta::TrackPtr t = randomTrackFromDomain();
            int place = KRandom::random() % ( m_activePlaylist.size() + 1 );
            s += c->deltaS_insert( m_activePlaylist, t, place );
            c->insertTrack( m_activePlaylist, t, place );
            m_activePlaylist.insert( place, t );
        }
        iuS = c->satisfaction( m_activePlaylist );
        if ( qAbs( iuS - s ) > 1e-8 ) {
            debug() << name << "incremental updater for insert is broken: expected" << s << "true" << iuS;
            s = iuS;
        }
        for (int i = 0; i < loops; i++ ) {
            Meta::TrackPtr t = randomTrackFromDomain();
            int place = KRandom::random() % ( m_activePlaylist.size() );
            s += c->deltaS_replace( m_activePlaylist, t, place );
            c->replaceTrack( m_activePlaylist, t, place );
            m_activePlaylist.replace( place, t );
        }
        iuS = c->satisfaction( m_activePlaylist );
        if ( qAbs( iuS - s ) > 1e-8 ) {
            debug() << name << "incremental updater for replace is broken: expected" << s << "true" << iuS;
            s = iuS;
        }
        for (int i = 0; i < loops; i++ ) {
            int place = KRandom::random() % ( m_activePlaylist.size() );
            s += c->deltaS_delete( m_activePlaylist, place );
            c->deleteTrack( m_activePlaylist, place );
            m_activePlaylist.removeAt( place );
        }
        iuS = c->satisfaction( m_activePlaylist );
        if ( qAbs( iuS - s ) > 1e-8 ) {
            debug() << name << "incremental updater for delete is broken: expected" << s << "true" << iuS;
            s = iuS;
        }
        for (int i = 0; i < loops; i++ ) {
            int place = KRandom::random() % ( m_activePlaylist.size() );
            int other = KRandom::random() % ( m_activePlaylist.size() );
            s += c->deltaS_swap( m_activePlaylist, place, other );
            c->swapTracks( m_activePlaylist, place, other );
            m_activePlaylist.swap( place, other );
        }
        iuS = c->satisfaction( m_activePlaylist );
        if ( qAbs( iuS - s ) > 1e-8 ) {
            debug() << name << "incremental updater for swap is broken: expected" << s << "true" << iuS;
            s = iuS;
        }

        // this loops tests mixed incremental update operations
        loops = ( KRandom::random() % 100 ) + 100;
        int insertCount = 0;
        int replaceCount = 0;
        int deleteCount = 0;
        int swapCount = 0;
        for ( int i = 0; i < loops; i++ ) {
            // make sure the playlist has at least 5 tracks in it
            if ( m_activePlaylist.size() < 5 ) {
                for (int i = 0; i < 95; i++ )
                    m_activePlaylist.append( randomTrackFromDomain() );
            }

            Meta::TrackPtr t = randomTrackFromDomain();
            int op = KRandom::random() % 4;
            int place = KRandom::random() % ( m_activePlaylist.size() );
            int other = KRandom::random() % ( m_activePlaylist.size() );
            switch ( op ) {
                case 0:
                    insertCount++;
                    place = KRandom::random() % ( m_activePlaylist.size() + 1 ); // need to be able to append, too
                    s += c->deltaS_insert( m_activePlaylist, t, place );
                    c->insertTrack( m_activePlaylist, t, place );
                    m_activePlaylist.insert( place, t );
                    break;
                case 1:
                    replaceCount++;
                    s += c->deltaS_replace( m_activePlaylist, t, place );
                    c->replaceTrack( m_activePlaylist, t, place );
                    m_activePlaylist.replace( place, t );
                    break;
                case 2:
                    deleteCount++;
                    s += c->deltaS_delete( m_activePlaylist, place );
                    c->deleteTrack( m_activePlaylist, place );
                    m_activePlaylist.removeAt( place );
                    break;
                case 3:
                    swapCount++;
                    s += c->deltaS_swap( m_activePlaylist, place, other );
                    c->swapTracks( m_activePlaylist, place, other );
                    m_activePlaylist.swap( place, other );
                    break;
            }
        }
        double finalS = c->satisfaction( m_activePlaylist );
        if ( qAbs( finalS - s ) > 1e-6 ) { // ignore floating-point precision errors
            errors++;
            debug() << name << "final satisfaction across" << loops << "loops failed: expected" << s << "true" << finalS;
            debug() << "\tthis usually means that one of the incremental update functions within the constraint is broken";
            debug() << "\t" << insertCount << "inserts";
            debug() << "\t" << replaceCount << "replacements";
            debug() << "\t" << deleteCount << "deletes";
            debug() << "\t" << swapCount << "swaps";
        }

    }
    return errors;
}

Meta::TrackPtr
APG::ConstraintTester::randomTrackFromDomain() const
{
    return m_domain.at( KRandom::random() % m_domain.size() );
}
