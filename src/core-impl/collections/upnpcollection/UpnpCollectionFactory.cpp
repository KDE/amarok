/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#define DEBUG_PREFIX "UpnpCollectionFactory"
#include "UpnpCollectionFactory.h"

#include <kio/jobclasses.h>
#include <kio/scheduler.h>
#include <kio/netaccess.h>
#include <kdirlister.h>
#include <kurl.h>

#include "core/support/Debug.h"
#include "UpnpBrowseCollection.h"
#include "deviceinfo.h"
#include "dbuscodec.h"

#include <QtDBus>

namespace Collections {

AMAROK_EXPORT_COLLECTION( UpnpCollectionFactory, upnpcollection )

UpnpCollectionFactory::UpnpCollectionFactory( QObject *parent, const QVariantList &args )
    : Collections::CollectionFactory()
{
    qDBusRegisterMetaType<DeviceTypeMap>();
    qDBusRegisterMetaType<DeviceInfo>();
    setParent( parent );
    Q_UNUSED( args );
}

UpnpCollectionFactory::~UpnpCollectionFactory()
{
}

// TODO need to monitor upnp devices somehow

void UpnpCollectionFactory::init()
{
    DEBUG_BLOCK

        QDBusConnection bus = QDBusConnection::sessionBus();

    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi",
                 "org.kde.Cagibi",
                 "devicesAdded",
                 this,
                 SLOT( slotDevicesAdded( const DeviceTypeMap & ) ) );

    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi",
                 "org.kde.Cagibi",
                 "devicesRemoved",
                 this,
                 SLOT( slotDevicesRemoved( const DeviceTypeMap & ) ) );

    m_iface = new QDBusInterface("org.kde.Cagibi",
                                               "/org/kde/Cagibi",
                                               "org.kde.Cagibi",
                                               bus,
                                               this );
    Q_ASSERT(m_iface->isValid());
    QDBusReply<DeviceTypeMap> reply = m_iface->call( "allDevices" );
    if( !reply.isValid() ) {
        debug() << "ERROR" << reply.error().message();
        //Q_ASSERT(false);
    }
    else {
        slotDevicesAdded( reply.value() );
    }
}

void UpnpCollectionFactory::slotDevicesAdded( const DeviceTypeMap &map )
{
    foreach( QString udn, map.keys() ) {
        QString type = map[udn];
        if( type.startsWith("urn:schemas-upnp-org:device:MediaServer") ) {
            QDBusReply<DeviceInfo> reply = m_iface->call( "deviceDetails", udn );
            if( !reply.isValid() ) {
                debug() << "Invalid reply from deviceDetails for" << udn << ". Skipping";
                debug() << "Error" << reply.error().message();
                continue;
            }
            DeviceInfo info = reply.value();
            udn.replace("uuid:", "");
            m_devices[udn] = new UpnpBrowseCollection( udn, info.friendlyName() );
            emit newCollection( m_devices[udn] );
        }
    }
}

void UpnpCollectionFactory::slotDevicesRemoved( const DeviceTypeMap &map )
{
    foreach( QString udn, map.keys() ) {
        udn.replace("uuid:", "");
        if( m_devices.contains(udn) ) {
            m_devices[udn]->removeCollection();
            m_devices.remove(udn);
        }
    }
}

void UpnpCollectionFactory::createCollection( const QString &udn )
{
}

}
