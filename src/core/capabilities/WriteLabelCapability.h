/****************************************************************************************
 * Copyright (C) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#ifndef WRITELABELCAPABILITY_H
#define WRITELABELCAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"
#include "core/meta/forward_declarations.h"

namespace Capabilities
{

class AMAROKCORE_EXPORT WriteLabelCapability : public Capabilities::Capability
{
    Q_OBJECT
    public:
        static Type capabilityInterfaceType() { return Capabilities::Capability::WriteLabel; }

        //Implementors
        virtual void setLabels( const QStringList &removedLabels, const QStringList &labels ) = 0;

};

}
#endif // READLABELCAPABILITY_H
