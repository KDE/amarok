/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_SOLID_HANDLER_H
#define AMAROK_SOLID_HANDLER_H

#include "amarok_export.h"

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace Solid {
    class Device;
}

/**
  * This class interfaces Amarok to Solid, getting portable media player device
  * information and watching for changes.
  * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
  */

namespace PortableDevices {

    class SolidHandler : public QObject
    {
        Q_OBJECT
    
        public:
    
            static SolidHandler* s_instance;

            static SolidHandler* instance();
            
            /**
            * Creates a new SolidHandler.
            * 
            */
            SolidHandler();
            ~SolidHandler();
    
            void Initialize();
            QList<Solid::Device>   m_portableList;
    };
}
#endif /* AMAROK_SOLID_HANDLER_H */

