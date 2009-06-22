/*
   Copyright (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "IpodConnectionAssistant.h"
#include "IpodDeviceInfo.h"

#include "Debug.h"

#include <QString>

#include <solid/device.h>

IpodConnectionAssistant::~IpodConnectionAssistant()
{
}

bool
IpodConnectionAssistant::identify( const QString& udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device(udi);
    /* going until we reach a vendor, e.g. Apple */
    while ( device.isValid() && device.vendor().isEmpty() )
    {
        device = Solid::Device( device.parentUdi() );
    }

    debug() << "Device udi: " << udi;
    //debug() << "Device name: " << MediaDeviceCache::instance()->deviceName(udi);
    //debug() << "Mount point: " << MediaDeviceCache::instance()->volumeMountPoint(udi);
    if ( device.isValid() )
    {
        debug() << "vendor: " << device.vendor() << ", product: " << device.product();
    }

    /* if iPod or iPhone found, return true */
    return device.product() == "iPod" || device.product() == "iPhone";
}

MediaDeviceInfo*
IpodConnectionAssistant::deviceInfo( const QString& udi )
{
    Q_UNUSED( udi );
    MediaDeviceInfo* info = new IpodDeviceInfo();
    return info;
}

#include "IpodConnectionAssistant.moc"
