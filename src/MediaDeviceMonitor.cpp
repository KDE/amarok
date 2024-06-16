/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "MediaDeviceMonitor"

#include "MediaDeviceMonitor.h"

#include "MediaDeviceCache.h"
#include "core-impl/collections/mediadevicecollection/support/ConnectionAssistant.h"
#include "core/support/Debug.h"

#include <Solid/DeviceNotifier>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/PortableMediaPlayer>
#include <Solid/OpticalDrive>

#include <QTimer>

MediaDeviceMonitor* MediaDeviceMonitor::s_instance = nullptr;

MediaDeviceMonitor::MediaDeviceMonitor() : QObject()
 , m_udiAssistants()
 , m_assistants()
 , m_waitingassistants()
 , m_nextassistant( 0 )
 // NOTE: commented out, needs porting to new device framework
 //, m_currentCdId( QString() )
{
    DEBUG_BLOCK
    s_instance = this;
    init();
}

MediaDeviceMonitor::~MediaDeviceMonitor()
{
    s_instance = nullptr;
}

void
MediaDeviceMonitor::init()
{
    DEBUG_BLOCK

    // connect to device cache so new devices are tested too
    connect(  MediaDeviceCache::instance(),  &MediaDeviceCache::deviceAdded,
              this, &MediaDeviceMonitor::deviceAdded );
    connect(  MediaDeviceCache::instance(),  &MediaDeviceCache::deviceRemoved,
              this, &MediaDeviceMonitor::slotDeviceRemoved );
    connect(  MediaDeviceCache::instance(), &MediaDeviceCache::accessibilityChanged,
              this, &MediaDeviceMonitor::slotAccessibilityChanged );
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
    DEBUG_BLOCK

    // First let the higher priority devices check

    for( ConnectionAssistant* assistant : m_assistants )
    {
        checkOneDevice( assistant, udi );
    }

    // Then let the assistants that can wait check

    for( ConnectionAssistant* assistant : m_waitingassistants )
    {
        checkOneDevice( assistant, udi );
    }

}

void MediaDeviceMonitor::checkOneDevice( ConnectionAssistant* assistant, const QString& udi )
{
    // Ignore already identified devices
    if( m_udiAssistants.keys().contains( udi ) )
    {
        debug() << "Device already identified with udi: " << udi;
        return;
    }

    if( assistant->identify( udi ) )
    {
        debug() << "Device identified with udi: " << udi;
        // keep track of which assistant deals with which device
        m_udiAssistants.insert( udi, assistant );
        // inform factory of new device identified
        assistant->tellIdentified( udi );
        return;
    }
}

void MediaDeviceMonitor::checkDevicesFor( ConnectionAssistant* assistant )
{
    DEBUG_BLOCK

    QStringList udiList = getDevices();

    for( const QString &udi : udiList )
    {
        checkOneDevice( assistant, udi );
    }

}

void
MediaDeviceMonitor::registerDeviceType( ConnectionAssistant* assistant )
{
    DEBUG_BLOCK

    // If the device wants to wait and give other device types
    // a chance to recognize devices, put it in a queue for
    // later device checking

    if ( assistant->wait() )
    {
        // keep track of this type of device from now on
        m_waitingassistants << assistant;

        QTimer::singleShot( 1000, this, &MediaDeviceMonitor::slotDequeueWaitingAssistant );
    }
    else
    {
        // keep track of this type of device from now on
        m_assistants << assistant;

        // start initial check for devices of this type
        checkDevicesFor( assistant );
    }

}

void
MediaDeviceMonitor::deviceAdded( const QString &udi )
{
    DEBUG_BLOCK

    // check if device is a known device
    checkDevice( udi );
}

void
MediaDeviceMonitor::slotDeviceRemoved( const QString &udi )
{
    DEBUG_BLOCK

    if ( m_udiAssistants.contains( udi ) )
    {

        m_udiAssistants.value( udi )->tellDisconnected( udi );

        m_udiAssistants.remove( udi );
    }


//    Q_EMIT deviceRemoved( udi );
}

void
MediaDeviceMonitor::slotAccessibilityChanged( bool accessible, const QString & udi)
{
    // TODO: build a hack to force a device to become accessible or not
    // This means auto-mounting of Ipod, and ejecting of it too

    DEBUG_BLOCK
            debug() << "Accessibility changed to: " << ( accessible ? "true":"false" );
    if ( !accessible )
        deviceRemoved( udi );
    else
        deviceAdded( udi );
}

void
MediaDeviceMonitor::slotDequeueWaitingAssistant()
{
    checkDevicesFor( m_waitingassistants.at( m_nextassistant++ ) );
}
