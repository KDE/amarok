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

#include "Transition.h"

#include "State.h"

Amarok::Transition::Transition(State* state)
    : QObject()
    , m_active( false )
    , m_targetState( state )
{
}

void
Amarok::Transition::activate()
{
    if( m_active )
        return;
    m_active = true;
    doActivate();
}

void
Amarok::Transition::deactivate()
{
    if( !m_active )
        return;
    m_active = false;
    doDeactivate();
}

bool
Amarok::Transition::isActive() const
{
    return m_active;
}

bool
Amarok::Transition::isPriorityTransition() const
{
    return false;
}

void
Amarok::Transition::doActivate()
{
    //reimplement in subclasses
}

void
Amarok::Transition::doDeactivate()
{
    //reimplement in subclasses
}

void
Amarok::Transition::requestTransition()
{
    if( m_active )
        emit transitionRequested( targetState );
}
