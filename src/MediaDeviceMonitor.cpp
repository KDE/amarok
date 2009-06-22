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
#include "ConnectionAssistant.h"

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
 , m_assistants()
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

void MediaDeviceMonitor::checkDevice(const QString& udi)
{
    foreach( ConnectionAssistant* assistant, m_assistants )
    {
        // Ignore already identified devices
        if( m_udiAssistants.keys().contains( udi ) )
            return;

        if( assistant->identify( udi ) )
        {
            // keep track of which assistant deals with which device
            m_udiAssistants.insert( udi, assistant );
            // inform factory of new device identified
            assistant->tellIdentified( udi );
            return;
        }
    }

}

void MediaDeviceMonitor::checkDevicesFor( ConnectionAssistant* assistant )
{
    DEBUG_BLOCK

    QStringList udiList = getDevices();

    foreach( const QString &udi, udiList )
    {
        // Ignore already identified devices
        if( m_udiAssistants.keys().contains( udi ) )
            continue;

        if( assistant->identify( udi ) )
        {
            // keep track of which assistant deals with which device
            m_udiAssistants.insert( udi, assistant );
            // inform factory of new device identified
            assistant->tellIdentified( udi );
        }
    }

}

void
MediaDeviceMonitor::registerDeviceType( ConnectionAssistant* assistant )
{
    DEBUG_BLOCK

    // keep track of this type of device from now on
    m_assistants << assistant;

    // start initial check for devices of this type
    checkDevicesFor( assistant );

}

void
MediaDeviceMonitor::deviceAdded(  const QString &udi )
{
    DEBUG_BLOCK
/*
    QStringList udiList;

    debug() << "New device added, testing...";

    udiList.append( udi );
*/
    // check if device is a known device
    checkDevice( udi );
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

/*
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
