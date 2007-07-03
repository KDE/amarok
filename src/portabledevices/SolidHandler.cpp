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
#include "solid/devicenotifier.h"

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
                             , m_portableList()
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
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString & ) ),
             this, SLOT( deviceAdded( const QString & ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString & ) ),
             this, SLOT( deviceRemoved( const QString & ) ) );
    QList<Solid::Device> deviceList = Solid::Device::listFromQuery( "is PortableMediaPlayer" );
    Solid::Device temp;
    foreach( Solid::Device device, deviceList )
    {
        debug() << "Found Solid::DeviceInterface::PortableMediaPlayer with udi = " << device.udi() << endl;
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor() << endl;
        m_portableList << device.udi();
    }
}

void
SolidHandler::deviceAdded( const QString &udi )
{
    if( m_portableList.contains( udi ) )
    {
        debug() << "Error: duplicate UDI trying to be added from Solid." << endl;
        return;
    }
    m_portableList << udi;
    emit addDevice( udi );
}

void
SolidHandler::deviceRemoved( const QString &udi )
{
    if( m_portableList.contains( udi ) )
    {
        m_portableList.removeAll( udi );
        emit removeDevice( udi );
        return;
    }
}

namespace The {
    PortableDevicesNS::SolidHandler* SolidHandler() { return PortableDevicesNS::SolidHandler::instance(); }
}

#include "SolidHandler.moc"

