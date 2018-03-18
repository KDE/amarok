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

#ifndef AMAROK_ACTIONSCAPABILITY_H
#define AMAROK_ACTIONSCAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"

#include <QAction>
#include <QList>

namespace Capabilities
{
    /**
     * This capability allows different meta types to display custom actions in the right click menu in the tree view
     * or anywhere else where the actions are shown. This is useful for purchasing from stores, downloading from services
     * banning a genre or whatever we can think of in the future.
     *
     * If you want to provide this capability for an album, consider using
     * @see AlbumActionsCapability that provides you with common album actions such as
     * show cover etc. for free.
     *
     * @author Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class AMAROK_CORE_EXPORT ActionsCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:
            /**
             * Constructor
             * Note: The actions are not freed after usage
             * @param actions A list of actions to use.
             */
            explicit ActionsCapability( const QList< QAction* > &actions );

            /**
             * Destructor
             */
            virtual ~ActionsCapability();

            /**
             * Get the custom actions for this capablility
             * The caller must free actions that have no parent after use.
             * Actions with a parent are freed by the parent (obviously)
             * @return The list of actions
             */
            virtual QList<QAction *> actions() const;

            /**
             * Get the capabilityInterfaceType of this capability
             * @return The capabilityInterfaceType ( always Capabilities::Capability::Actions; )
             */
            static Type capabilityInterfaceType() { return Capabilities::Capability::Actions; }

        protected:
            /**
             * No-action constructor has sense only for subclasses.
             */
            ActionsCapability();

            QList< QAction* > m_actions;
    };
}

#endif
