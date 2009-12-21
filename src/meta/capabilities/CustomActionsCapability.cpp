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

#include "CustomActionsCapability.h"

Meta::CustomActionsCapability::CustomActionsCapability()
    : Meta::Capability()
{
    //nothing to do
}

Meta::CustomActionsCapability::CustomActionsCapability( const QList<QAction*> &actions )
    : Meta::Capability()
    , m_actions( actions )
{
    //nothing to do
}

Meta::CustomActionsCapability::~CustomActionsCapability()
{
    //nothing to do.
    //TODO are we responsible for deleting the actions?
}

QList<QAction *>
Meta::CustomActionsCapability::customActions() const
{
    return m_actions;
}

#include "CustomActionsCapability.moc"
