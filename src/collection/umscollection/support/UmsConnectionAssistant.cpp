/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "UmsConnectionAssistant.h"
#include "UmsDeviceInfo.h"

#include "MediaDeviceCache.h" // for mountpoint

#include "Debug.h"

#include <QString>

#include <solid/device.h>
#include <solid/storagedrive.h>

UmsConnectionAssistant::UmsConnectionAssistant()
    : ConnectionAssistant( true )
{
}

UmsConnectionAssistant::~UmsConnectionAssistant()
{
}

bool
UmsConnectionAssistant::identify( const QString& udi )
{
    DEBUG_BLOCK

    // NOTE: device detection code taken from KDE's device notifier applet

    Solid::Device device;
    Solid::Device parentDevice;

    device = Solid::Device(udi);
    parentDevice = device.parent();
    Solid::StorageDrive *drive = parentDevice.as<Solid::StorageDrive>();

    /* going until we reach a vendor */
/*
    while ( device.isValid() )
    {
        device = Solid::Device( device.parentUdi() );
    }
*/
    debug() << "Device udi: " << udi;
    debug() << "Device name: " << MediaDeviceCache::instance()->deviceName(udi);
    debug() << "Mount point: " << MediaDeviceCache::instance()->volumeMountPoint(udi);

    if ( device.isValid() )
    {
        debug() << "vendor: " << device.vendor() << ", product: " << device.product();
    }

    // TODO: deal with iPod case, since it's also generic

    return ( !MediaDeviceCache::instance()->volumeMountPoint( udi ).isEmpty()
             && drive && (drive->isHotpluggable() || drive->isRemovable()) );
             //&& MediaDeviceCache::instance()->isGenericEnabled( udi ) );

}


MediaDeviceInfo*
UmsConnectionAssistant::deviceInfo( const QString& udi )
{

    QString mountpoint = MediaDeviceCache::instance()->volumeMountPoint(udi);

    MediaDeviceInfo* info = new UmsDeviceInfo( mountpoint, udi );
    return info;
}

#include "UmsConnectionAssistant.moc"
