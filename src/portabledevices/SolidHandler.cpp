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

#include "solid/device.h"

#include <QList>

#include "debug.h"
#include "SolidHandler.h"

using namespace PortableDevicesNS;

SolidHandler* SolidHandler::s_instance = 0;

SolidHandler*
SolidHandler::instance()
{
    static SolidHandler sh;
    return &sh;
}

SolidHandler::SolidHandler() : QObject()
{
    DEBUG_BLOCK
    s_instance = this;
}

SolidHandler::~SolidHandler()
{
}

void
SolidHandler::Initialize()
{
    DEBUG_BLOCK
    //m_portableList = Solid::Device::listFromType( Solid::DeviceInterface::OpticalDrive );
    m_portableList = Solid::Device::listFromQuery( "is OpticalDrive" );  
    Solid::Device temp;
    for  (int i = 0; i < m_portableList.size(); ++i) {
        temp = m_portableList.at( i );
        debug() << "Found Solid::DeviceInterface::PortableMediaPlayer with udi = " << temp.udi() << endl;
        debug() << "Device name is = " << temp.product() << " and was made by " << temp.vendor() << endl;
    }
}

namespace The {
    PortableDevicesNS::SolidHandler* SolidHandler() { return PortableDevicesNS::SolidHandler::instance(); }
}

#include "SolidHandler.moc"

