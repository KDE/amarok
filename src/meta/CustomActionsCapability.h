/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROK_CUSTOMACTIONSCAPABILITY_H
#define AMAROK_CUSTOMACTIONSCAPABILITY_H

#include "amarok_export.h"
#include "meta/Capability.h"

#include <QAction>
#include <QList>


namespace Meta
{

    /**
     * This capability allows different meta types to display custom actions int the right click menu in the tree view
     * or anywhere else where the actions are shown. This is usefull for purchasing from stores, downloading from services
     * banning a genre or whatever we can think of in the future
     */
    class AMAROK_EXPORT CustomActionsCapability : public Meta::Capability
    {
        Q_OBJECT
        public:
            CustomActionsCapability();
            CustomActionsCapability( const QList< QAction* > &actions );
            virtual ~CustomActionsCapability();

            virtual QList< QAction * > customActions() const;
            
            static Type capabilityInterfaceType() { return Meta::Capability::CustomActions; }

        protected:
            QList< QAction* > m_actions;
    };
}

#endif
