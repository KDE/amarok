/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "StateManager.h"

#include "Debug.h"

#include "State.h"
#include "Transition.h"

#include <QMutexLocker>
#include <QTimer>

Amarok::StateManager::StateManager()
    : QObject()
    , m_stateTransitionInProgress( false )
    , m_currentState( 0 )
{
    //TODO: implement setup
}

Amarok::StateManager::~StateManager()
{
    m_transitionQueue.clear();
    m_currentState = 0;
    m_possibleTransitions.clear();
    qDeleteAll( m_transitions )
    qDeleteAll( m_states )
}

void
Amarok::StateManager::run()
{
    QMutexLocker locker( &m_mutex );
    //TODO: add backendstartingstate to transitionqueue
    QTimer::singleShot( 0, this, SLOT( checkForTransitionRequest() ) );
}

QString
Amarok::StateManager::currentState() const
{
    QMutexLocker locker( &m_mutex );
    if( m_currentState )
    {
        return m_currentState->name();
    }
    else
    {
        return "No state";
    }
}

void
Amarok::StateManager::stateReady( State *state )
{
    DEBUG_BLOCK
    debug() << "Reached state " << state->name();
    QMutexLocker locker( &m_mutex );
    m_stateTransitionInProgress = false;
    QTimer::singleShot( 0, this, SLOT( checkForTransitionRequest() ) );
}

void
Amarok::StateManager::transitionRequested( bool priorityTransition, State* targetState )
{
    DEBUG_BLOCK
    if( !targetState )
        return;
    
    debug() << priorityTransition ? "Priority " : "" << "Transition requested from state " << m_currentState->name() << " to state " << targetState->name();
    
    QMutexLocker locker( &m_mutex );
    if( priorityTransition )
    {
        m_transitionQueue.prepend( targetState );
    }
    else
    {
        m_transitionQueue.append( targetState );
    }
    QTimer::singleShot( 0, this, SLOT( checkForTransitionRequest() ) );
}

void
Amarok::StateManager::checkForTransitionRequest()
{
    DEBUG_BLOCK
    m_mutex.lock();
    if( m_transitionQueue.isEmpty() )
    {
        m_mutex.unlock();
        return;
    }
    
    State* targetState = m_transitionQueue.takeFirst();
    
    debug() << "Beginnging transition from state " << m_currentState->name() << " to state " << targetState->name();
    
    foreach( Transition* t, m_possibleTransitions.value( m_currentState ) )
    {
        t->deactivate();
    }
    foreach( Transition* t, m_possibleTransitions.value( targetState ) )
    {
        t->activate();
    }
    m_stateTransitionInProgress = true;
    m_currentState = targetState;
    m_mutex.unlock();
    targetState->activate();
}

