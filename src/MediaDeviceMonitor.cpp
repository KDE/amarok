/*
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>
   Copyright (C) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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
#include <solid/opticaldisc.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/portablemediaplayer.h>
#include <solid/opticaldrive.h>

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
    checkDevicesForCd();
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
            // HACK: Usability: Force auto-connection of device upon detection
            connectIpod( MediaDeviceCache::instance()->volumeMountPoint(udi), udi );
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
            // HACK: Usability: Force auto-connection of device upon detection
            connectMtp( serial, udi );
            emit mtpDetected( serial, udi );
        }
    }
}

void MediaDeviceMonitor::checkDevicesForCd()
{
    DEBUG_BLOCK

    QStringList udiList = getDevices();

    /* poll udi list for supported devices */
    foreach(const QString &udi, udiList )
    {
        debug() << "udi: " << udi;
        if ( isAudioCd( udi ) )
        {
            emit audioCdDetected( udi );
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

    /* if iPod or iPhone found, return true */
    return device.product() == "iPod" || device.product() == "iPhone";
}

bool
MediaDeviceMonitor::isMtp( const QString &udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device( udi );
    if( !device.is<Solid::PortableMediaPlayer>() )
    {
        debug() << "Not a PMP";
        return false;
    }

    Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();

    debug() << "Supported Protocols: " << pmp->supportedProtocols();

    return pmp->supportedProtocols().contains( "mtp" );
}

bool MediaDeviceMonitor::isAudioCd( const QString & udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device( udi );
    if( device.is<Solid::OpticalDisc>() )
    {
        debug() << "OpticalDisc";
        Solid::OpticalDisc * opt = device.as<Solid::OpticalDisc>();
        if ( opt->availableContent() & Solid::OpticalDisc::Audio )
        {
            debug() << "AudioCd";
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

QString MediaDeviceMonitor::isCdPresent()
{
    DEBUG_BLOCK

    QStringList udiList = getDevices();

    /* poll udi list for supported devices */
    foreach( const QString &udi, udiList )
    {
        debug() << "udi: " << udi;
        if ( isAudioCd( udi ) )
        {
            return udi;
        }
    }

    return QString();
}

void MediaDeviceMonitor::ejectCd( const QString & udi )
{
    DEBUG_BLOCK
    debug() << "trying to eject udi: " << udi;
    Solid::Device device = Solid::Device( udi ).parent();

    if ( !device.isValid() ) {
        debug() << "invalid device, cannot eject";
        return;
    }


    debug() << "lets tryto get an OpticalDrive out of this thing";
    if( device.is<Solid::OpticalDrive>() )
    {
        debug() << "claims to be an OpticalDrive";
        Solid::OpticalDrive * drive = device.as<Solid::OpticalDrive>();
        if ( drive )
        {
            debug() << "ejecting the bugger";
            drive->eject();
        }
    }
}




