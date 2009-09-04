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

#ifndef AMAROK_DECORATORCAPABILITY_H
#define AMAROK_DECORATORCAPABILITY_H

#include "amarok_export.h"
#include "meta/Capability.h"
#include "Meta.h"


#include <QAction>
#include <QList>
#include <QObject>

namespace Meta
{
    class AMAROK_EXPORT DecoratorCapability : public Meta::Capability
    {
        Q_OBJECT

        public:
            virtual ~DecoratorCapability();

            static Type capabilityInterfaceType() { return Meta::Capability::Decorator; }
            virtual QAction* decoratorAction() = 0;
    };
}

#endif
