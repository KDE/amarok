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

#ifndef AMAROK_CUSTOMACTIONSCAPABILITY_H
#define AMAROK_CUSTOMACTIONSCAPABILITY_H

#include "amarok_export.h"
#include "core/capabilities/Capability.h"

#include <QAction>
#include <QList>

namespace Capabilities
{
    /**
     * This capability allows different meta types to display custom actions in the right click menu in the tree view
     * or anywhere else where the actions are shown. This is useful for purchasing from stores, downloading from services
     * banning a genre or whatever we can think of in the future
     *
     *         @author Nikolaj Hald Nielsen <nhn@kde.org>
     */

    class AMAROK_EXPORT CustomActionsCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:
            
            /**
             * Constructor
             */
            CustomActionsCapability();
            
            /**
             * Constructor
             * @param actions A list of actions to use.
             */
            CustomActionsCapability( const QList< QAction* > &actions );
            /**
             * Destructor
             */
            virtual ~CustomActionsCapability();

            /**
             * Get the custom actions for this capablility
             * @return The list of custom actions
             */
            virtual QList<QAction *> customActions() const;

            /**
             * Get the capabilityInterfaceType of this capability
             * @return The capabilityInterfaceType ( always Capabilities::Capability::CustomActions; )
             */
            static Type capabilityInterfaceType() { return Capabilities::Capability::CustomActions; }

        protected:
            QList< QAction* > m_actions;
    };
}

#endif
