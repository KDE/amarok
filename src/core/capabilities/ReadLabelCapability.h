/*
    Copyright (C) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef READLABELCAPABILITY_H
#define READLABELCAPABILITY_H

#include "core/capabilities/Capability.h"
#include "core/meta/Meta.h"
#include "shared/amarok_export.h"

namespace Capabilities
{

class AMAROK_EXPORT ReadLabelCapability : public Capabilities::Capability
{
    Q_OBJECT
    public:
        static Type capabilityInterfaceType() { return Capabilities::Capability::ReadLabel; }

        //Implementors
        virtual void fetchLabels() = 0;
        virtual void fetchGlobalLabels() = 0;
        virtual QStringList labels() = 0;

    signals:
        void labelsFetched( QStringList );

};

}
#endif // READLABELCAPABILITY_H
