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

#ifndef AMAROK_STATEMANAGER_H
#define AMAROK_STATEMANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>
#include <QMutex>

namespace Amarok {
    
    class State;
    class Transition;
    
    /**
     * StateManager is the class that controls Amarok's application state. This means that this class can tell and
     * controls whether e.g. Amarok is currently starting, terminating, or running in (probably) one of various
     * running states like ConservePower or Minimized.
     *
     * State management uses a simple graph. State objects are the nodes in that graph, and Transition objects are the (directed) edges
     * When StateManager activates a state, that state can perform all necessary work to make Amarok actually reach that state.
     * After a state is ready, Statemanager will trigger the next transition, or wait for a new transition to become possible.
     *
     * Transition subclasses have to implement the necessary logic to determine whether a transition to their target state is possible
     * when they are active, e.g. an ExitTransition would have to activate when the user wants to quit Amarok.
     *
     * Advanced topics: multi-threaded startup/shutdown, if somebody can figure out a way around the thread affinity of QObjects...
     */
    class StateManager : public QObject
    {
        Q_OBJECT
    public:
        StateManager();
        ~StateManager();
        
        //gives control to StateManager to begin Amarok's startup and control its application state
        void run();
        
        QString currentState() const;
        
    private slots:
        void stateReady( State* state );
        void transitionRequested( bool priorityTransition, State* targetState );
        void checkForTransitionRequest();
        
    private:
        
        //a flag whether a state was activated and is not ready yet. do not activate multiple states
        bool m_stateTransitionInProgress;
        //the current state that we are in
        State* m_currentState;
        //the transitions possible for each state
        QHash<State*, QSet<Transition*> > m_possibleTransitions;
        //queue a state transition if a transition was requested while
        //another transition is in progress
        QQueue<State*> m_transitionQueue;
        QMutex m_mutex;
        //keeps a list of all states
        QSet<State*> m_states;
        //keeps a list of all transitions
        QSet<Transiton*> m_transitions;
    };
            
}

#endif
