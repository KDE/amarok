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

#define DEBUG_PREFIX "APG::Preset"

#include "Preset.h"

#include "ConstraintNode.h"
#include "ConstraintFactory.h"
#include "ConstraintSolver.h"
#include "constraints/TrackSpreader.h"

#include "core/collections/MetaQueryMaker.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistModelStack.h"
#include "statusbar/StatusBar.h"

#include <QDomElement>
#include <threadweaver/ThreadWeaver.h>

APG::PresetPtr
APG::Preset::createFromXml( QDomElement& xmlelem )
{
    DEBUG_BLOCK

    if ( xmlelem.isNull() ) {
        PresetPtr t( new Preset( i18n("New playlist preset") ) );
        return t;
    } else {
        PresetPtr t( new Preset( i18n("Unnamed playlist preset"), xmlelem ) );
        return t;
    }
}

APG::PresetPtr
APG::Preset::createNew()
{
    DEBUG_BLOCK

    PresetPtr t( new Preset( i18n("New playlist preset") ) );
    return t;
}

APG::Preset::Preset( const QString& title, QDomElement& xmlelem )
        : m_title( title )
        , m_constraintTreeRoot( 0 )
{

    if ( xmlelem.hasAttribute( "title" ) ) {
        m_title = xmlelem.attribute( "title" );
    } else {
        m_title = i18n("Unnamed playlist preset");
    }
    for ( int i = 0; i < xmlelem.childNodes().count(); i++ ) {
        QDomElement childXmlElem = xmlelem.childNodes().item( i ).toElement();
        if ( !childXmlElem.isNull() ) {
            if ( childXmlElem.tagName() == "constrainttree" ) {
                m_constraintTreeRoot = ConstraintFactory::instance()->createGroup( childXmlElem, 0 );
            } else {
                error() << "unknown child: " << childXmlElem.nodeName();
            }
        }
    }

    if ( !m_constraintTreeRoot ) {
        m_constraintTreeRoot = ConstraintFactory::instance()->createGroup( 0 );
    }
}

APG::Preset::Preset( const QString& title )
        : m_title( title )
{

    m_constraintTreeRoot = ConstraintFactory::instance()->createGroup( 0 );
}

APG::Preset::~Preset()
{
    m_constraintTreeRoot->deleteLater();
}

QDomElement*
APG::Preset::toXml( QDomDocument& xmldoc ) const
{
    QDomElement e = xmldoc.createElement( "generatorpreset" );
    e.setAttribute( "title", m_title );
    m_constraintTreeRoot->toXml( xmldoc, e );
    return new QDomElement( e );
}

void
APG::Preset::generate( int q )
{
    ConstraintSolver* solver = new ConstraintSolver( m_constraintTreeRoot, q );
    connect( solver, SIGNAL( readyToRun() ), this, SLOT( queueSolver() ) );
}

void APG::Preset::queueSolver() {

    /* Workaround for a design quirk of Weavers.  A Weaver will not
     * continuously poll queued but previously unrunnable jobs to see if they
     * are are runnable (and won't start them if they are), so what tends to
     * happen is that if a job is queued before it's ready to run, it fails the
     * canBeExecuted() check, and sits in the queue until something wakes up
     * the Weaver (eg, queueing another job).  Eventually, you get a pileup of
     * obsolete jobs in the queue, and those that do run properly return
     * obsolete results.  So to avoid that problem, we avoid queueing the job
     * until it's ready to run, and then the Weaver will start running it
     * pretty much immediately. -- sth */

    emit lock( true );

    ConstraintSolver* s = static_cast<ConstraintSolver*>( sender() );
    The::statusBar()->newProgressOperation( s, i18n("Generating a new playlist") )->setAbortSlot( s, SLOT( requestAbort() ) );
    connect( s, SIGNAL( incrementProgress() ), The::statusBar(), SLOT( incrementProgress() ) );
    connect( s, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( solverFinished( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
    The::statusBar()->incrementProgressTotalSteps( s, s->iterationCount() );

    m_constraintTreeRoot->addChild( ConstraintTypes::TrackSpreader::createNew( m_constraintTreeRoot ), 0 ); // private mandatory constraint

    ThreadWeaver::Weaver::instance()->enqueue( s );
}

void
APG::Preset::solverFinished( ThreadWeaver::Job* job )
{
    m_constraintTreeRoot->removeChild( 0 ); // remove the TrackSpreader

    ConstraintSolver* solver = static_cast<ConstraintSolver*>( job );
    The::statusBar()->endProgressOperation( solver );
    if ( job->success() ) {
        debug() << "Solver" << solver->serial() << "finished successfully";
        if ( solver->finalSatisfaction() < 0.85 ) {
            The::statusBar()->longMessage( i18n("The playlist generator created a playlist which does not meet all of your constraints.  If you are not satisfied with the results, try loosening or removing some constraints and then generating a new playlist.") );
        }
        The::playlistController()->insertOptioned( solver->getSolution() , Playlist::Replace );
    } else {
        debug() << "Ignoring results from aborted Solver" << solver->serial();
    }
    job->deleteLater();

    emit lock( false );
}
