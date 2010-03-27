/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "core/capabilities/CustomActionsCapability.h"

Capabilities::CustomActionsCapability::CustomActionsCapability()
    : Capabilities::Capability()
{
    //nothing to do
}

Capabilities::CustomActionsCapability::CustomActionsCapability( const QList<QAction*> &actions )
    : Capabilities::Capability()
    , m_actions( actions )
{
    //nothing to do
}

Capabilities::CustomActionsCapability::~CustomActionsCapability()
{
    //nothing to do.
    //TODO are we responsible for deleting the actions?
}

QList<QAction *>
Capabilities::CustomActionsCapability::customActions() const
{
    return m_actions;
}

#include "CustomActionsCapability.moc"
