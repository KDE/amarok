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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "IpodConnectionAssistant.h"
#include "IpodDeviceInfo.h"

#include "MediaDeviceCache.h" // for mountpoint

#include "core/support/Debug.h"

#include <QString>

#include <solid/device.h>
#include <solid/portablemediaplayer.h>


IpodConnectionAssistant::~IpodConnectionAssistant()
{
}

/**
 * @brief Return true if device is identified as iPod-compatible using product and vendor.
 *
 * @param device Solid device to identify
 * @return true if the device is iPod-like, false if it cannot be proved.
 **/
static bool
deviceIsRootIpodDevice( const Solid::Device &device )
{
    if( !device.isValid() )
        return false;
    if( !device.vendor().contains( "Apple", Qt::CaseInsensitive ) )
        return false;
    return device.product().startsWith( "iPod" )
        || device.product().startsWith( "iPhone" )
        || device.product().startsWith( "iPad" );
}

/**
 * @brief Returns true if device is identified as iPod-compatible using
 * PortableMediaPlayer interface.
 *
 * @param device Solid device to identify
 * @return true if the device is iPod-like, false if it cannot be proved.
 **/
static bool
deviceIsPMPIpodDevice( const Solid::Device &device )
{
    /* This should be the one and only way to identify iPod-likes, but as of KDE 4.7.2,
     * solid does not attach PortableMediaPlayer to iPods at all and its
     * PortableMediaPlayer implementations is just a stub. This would also need
     * media-device-info package to be installed.
     */
    const Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();
    if( !pmp )
        return false;

    debug() << "Device supported PMP protocols:" << pmp->supportedProtocols();
    return pmp->supportedProtocols().contains( "ipod", Qt::CaseInsensitive );
}

bool
IpodConnectionAssistant::identify( const QString &udi )
{
    DEBUG_BLOCK
    Solid::Device device( udi );

    /* Start with device to identify, opportunistically try to identify it as
     * iPod-compatible. If found not, try its parent. Repeat until parent device is
     * valid.
     *
     * This method DOES return false positives for iPod-like devices which are itself
     * storage drives, but not storage volumes (such as sdc). This is needed for
     * iPhone-like devices that have extra code to mount them in IpodHandler. IpodHandler
     * gracefully fails in case this is old-school iPod, so this shouldn't hurt, only side
     * effect is that other connection assistant are not tried for these false-identified
     * devices. It would be of course great if iPhone-like devices could be distinguished
     * right here - KDE's Solid should be able to help us with it in future, but as of
     * KDE 4.7.2 it tells us nothing.
     *
     * @see MediaDeviceCache::slotAddSolidDevice() for a quirk that is currently also
     * needed for proper identification of iPhone-like devices.
     */
    while ( device.isValid() )
    {
        if( deviceIsPMPIpodDevice( device ) )
        {
            debug() << "Device" << device.udi() << "identified iPod-like using "
                       "PortableMediaPlayer interface";
            return true;
        }
        if( deviceIsRootIpodDevice( device ) )
        {
            debug() << "Device" << device.udi() << "identified iPod-like using "
                       "vendor and product name";
            return true;
        }

        debug() << "Device" << device.udi() << "not identified iPod-like, trying parent device";
        device = device.parent();
    }
    debug() << "Device" << device.udi() << "is invalid, returning false. (i.e. was not iPod-like)";
    return false;
}

MediaDeviceInfo*
IpodConnectionAssistant::deviceInfo( const QString& udi )
{

    QString mountpoint = MediaDeviceCache::instance()->volumeMountPoint(udi);

    MediaDeviceInfo* info = new IpodDeviceInfo( mountpoint, udi, !mountpoint.isEmpty() );
    return info;
}

#include "IpodConnectionAssistant.moc"
