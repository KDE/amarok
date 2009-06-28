/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef SERVICECUSTOMACTIONSCAPABILITY_H
#define SERVICECUSTOMACTIONSCAPABILITY_H

#include "amarok_export.h"
#include "meta/capabilities/CustomActionsCapability.h"

class CustomActionsProvider;

/**
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT ServiceCustomActionsCapability : public Meta::CustomActionsCapability
{
    Q_OBJECT

    public:
        ServiceCustomActionsCapability( CustomActionsProvider * customActionsProvider  );
        virtual ~ServiceCustomActionsCapability();
        virtual QList< PopupDropperAction * > customActions() const;

    private:
        CustomActionsProvider * m_customActionsProvider;
};

#endif

