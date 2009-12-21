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

#include <QTimer>

MediaDeviceMonitor* MediaDeviceMonitor::s_instance = 0;

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
    DEBUG_BLOCK

    // First let the higher priority devices check

    foreach( ConnectionAssistant* assistant, m_assistants )
    {
        checkOneDevice( assistant, udi );
    }

    // Then let the assistants that can wait check

    foreach( ConnectionAssistant* assistant, m_waitingassistants )
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

    foreach( const QString &udi, udiList )
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

        QTimer::singleShot( 1000, this, SLOT( slotDequeueWaitingAssistant() ) );
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


//    emit deviceRemoved( udi );
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

/// TODO: all stuff below here is cd-related, needs porting to new framework
#if 0

void
MediaDeviceMonitor::checkDevicesForCd()
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


bool
MediaDeviceMonitor::isAudioCd( const QString & udi )
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

QString
MediaDeviceMonitor::isCdPresent()
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

void
MediaDeviceMonitor::ejectCd( const QString & udi )
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

QString
MediaDeviceMonitor::currentCdId()
{
    return m_currentCdId;
}

void
MediaDeviceMonitor::setCurrentCdId( const QString & id )
{
    m_currentCdId = id;
}


#endif
