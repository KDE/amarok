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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TRANSITION_H
#define AMAROK_TRANSITION_H

namespace Amarok {
    class State;
    
    class Transition : public QObject{
        Q_OBJECT
    public:
        void activate();
        void deactivate();
        bool isActive() const;
        /**
         * priority transition will be executed first in case multiple states transitions have been activated at the same time
         */
        virtual bool isPriorityTransition() const;
        
    public signals:
        void transitionRequested( State* targetState );
        
    protected:
        Transition( State* targetState );
        /**
         * reimplement this method to add behaviour when statemanager activates the transition
         */
        virtual void doActivate();
        /**
         * reimplement this method to add behaviour when statemanager deactivates the transition
         */
        virtual void doDeactivate();
        /**
         * call this method from subclasses to request a transition of Amarok's application
         * state to the transition's target state.
         */
        void requestTransition();
    private:
        bool m_active;
        State* m_targetState;
    };
}

#endif
