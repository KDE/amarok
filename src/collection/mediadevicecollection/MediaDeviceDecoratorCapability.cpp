/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "MediaDeviceDecoratorCapability.h"
#include "MediaDeviceCollection.h"

#include "core/capabilities/CollectionCapability.h"

#include <QAction>

using namespace Capabilities;

MediaDeviceDecoratorCapability::MediaDeviceDecoratorCapability( MediaDeviceCollection *coll )
    : DecoratorCapability()
    , m_coll( coll )
{
}


QList<QAction*>
MediaDeviceDecoratorCapability::decoratorActions()
{
    QList<QAction*> actions;
    CollectionCapability *collCap =
            dynamic_cast<CollectionCapability *>(
                    m_coll->createCapabilityInterface( Capabilities::Capability::Collection ) );
    if( collCap )
        actions << collCap->collectionActions();
    else
        actions << m_coll->ejectAction(); //fallback

    return actions;
}

#include "MediaDeviceDecoratorCapability.moc"
