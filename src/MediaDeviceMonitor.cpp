/* 
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

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

#define DEBUG_PREFIX "MediaDeviceMonitor"

#include "MediaDeviceMonitor.h"

#include "Debug.h"

#include "MediaDeviceCache.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/portablemediaplayer.h>

MediaDeviceMonitor* MediaDeviceMonitor::s_instance = 0;

MediaDeviceMonitor::MediaDeviceMonitor() : QObject()
{
    DEBUG_BLOCK
    s_instance = this;
    init();
}

MediaDeviceMonitor::~MediaDeviceMonitor()
{
    s_instance = 0;
}

void
MediaDeviceMonitor::init()
{
    DEBUG_BLOCK

    // connect to device cache so new devices are tested too
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceAdded( const QString& ) ),
              SLOT(  deviceAdded( const QString& ) ) );
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceRemoved( const QString& ) ),
              SLOT(  slotDeviceRemoved( const QString& ) ) );
    connect(  MediaDeviceCache::instance(), SIGNAL( accessibilityChanged( bool, const QString & ) ),
              SLOT(  slotAccessibilityChanged( bool, const QString & ) ) );
}

QStringList
MediaDeviceMonitor::getDevices()
{
    DEBUG_BLOCK
    /* get list of devices */
    MediaDeviceCache::instance()->refreshCache();
    return MediaDeviceCache::instance()->getAll();

}

void
MediaDeviceMonitor::checkDevices()
{
    DEBUG_BLOCK
    /* poll udi list for supported devices */

    checkDevicesForMtp();
    checkDevicesForIpod();
}

void
MediaDeviceMonitor::checkDevicesForIpod()
{
    QStringList udiList = getDevices();

    /* poll udi list for supported devices */
    foreach(const QString &udi, udiList )
    {
        /* if ipod device found, emit signal */
        if( isIpod( udi ) )
        {
            emit ipodDetected( MediaDeviceCache::instance()->volumeMountPoint(udi), udi );
        }
    }
}

void
MediaDeviceMonitor::checkDevicesForMtp()
{
    QStringList udiList = getDevices();

    /* poll udi list for supported devices */
    foreach(const QString &udi, udiList )
    {
        if( isMtp( udi ) )
        {
            Solid::PortableMediaPlayer* pmp = Solid::Device( udi ).as<Solid::PortableMediaPlayer>();
            QString serial = pmp->driverHandle( "mtp" ).toString();
            debug() << "Serial is: " << serial;
            emit mtpDetected( serial, udi );
        }
    }
}



void
MediaDeviceMonitor::deviceAdded(  const QString &udi )
{
    DEBUG_BLOCK

    QStringList udiList;

    debug() << "New device added, testing...";

    udiList.append( udi );
    checkDevices();
}

void
MediaDeviceMonitor::slotDeviceRemoved( const QString &udi )
{
    DEBUG_BLOCK

    // NOTE: perhaps a simple forwarding of signals would do
    // via a connect

    emit deviceRemoved( udi );
}

void
MediaDeviceMonitor::slotAccessibilityChanged( bool accessible, const QString & udi)
{
    DEBUG_BLOCK
            debug() << "Accessibility changed to: " << ( accessible ? "true":"false" );
    if ( !accessible )
        deviceRemoved( udi );
    else
        deviceAdded( udi );
}

bool
MediaDeviceMonitor::isIpod( const QString &udi )
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
    debug() << "Device name: " << MediaDeviceCache::instance()->deviceName(udi);
    debug() << "Mount point: " << MediaDeviceCache::instance()->volumeMountPoint(udi);
    if ( device.isValid() )
    {
        debug() << "vendor: " << device.vendor() << ", product: " << device.product();
    }

    /* if iPod found, return true */
    return device.product() == "iPod";
}

bool
MediaDeviceMonitor::isMtp( const QString &udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device( udi );
    if( !device.is<Solid::PortableMediaPlayer>() )
        return false;

    Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();

    foreach( QString protocol, pmp->supportedProtocols() )
    {
        if( protocol == "mtp" )
        {
            debug() << "MTP device detected!";
            return true;
        }
    }

    return false;
}

void
MediaDeviceMonitor::connectIpod( const QString &mountpoint, const QString &udi )
{
    emit ipodReadyToConnect( mountpoint, udi );
}

void
MediaDeviceMonitor::disconnectIpod( const QString &udi )
{
    emit ipodReadyToDisconnect( udi );
}

void
MediaDeviceMonitor::connectMtp( const QString &serial, const QString &udi )
{
    emit mtpReadyToConnect( serial, udi );
}

void
MediaDeviceMonitor::disconnectMtp( const QString &udi )
{
    emit mtpReadyToDisconnect( udi );
}

