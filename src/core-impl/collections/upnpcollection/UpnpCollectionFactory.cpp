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
#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/storageaccess.h>

#include "core/support/Debug.h"
#include "UpnpBrowseCollection.h"
#include "UpnpSearchCollection.h"

namespace Collections {

AMAROK_EXPORT_COLLECTION( UpnpCollectionFactory, upnpcollection )

UpnpCollectionFactory::UpnpCollectionFactory( QObject *parent, const QVariantList &args )
    : Collections::CollectionFactory()
{
    setParent( parent );
    Q_UNUSED( args );
}

UpnpCollectionFactory::~UpnpCollectionFactory()
{
}

void UpnpCollectionFactory::init()
{
    DEBUG_BLOCK

    /*QDBusConnection bus = QDBusConnection::sessionBus();

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
    QDBusReply<DeviceTypeMap> reply = m_iface->call( "allDevices" );
    if( !reply.isValid() ) {
        debug() << "ERROR" << reply.error().message();
        //Q_ASSERT(false);
    }
    else {
        slotDevicesAdded( reply.value() );
    }*/

    Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    connect( notifier, SIGNAL(deviceAdded(QString)), this, SLOT(slotDeviceAdded(QString)) );
    connect( notifier, SIGNAL(deviceRemoved(QString)), this, SLOT(slotDeviceRemoved(QString)) );

    foreach( Solid::Device device, Solid::Device::allDevices() ) {
        slotDeviceAdded(device.udi());
    }
}

void UpnpCollectionFactory::slotDeviceAdded( const QString &udi )
{
    Solid::Device dev( udi );
    debug() << "CHECKING" << dev.udi();
    if( dev.udi().startsWith( "/org/kde/upnp" ) && dev.is<Solid::StorageAccess>() ) {
        createCollection( dev );
    }
}

void UpnpCollectionFactory::slotDeviceRemoved( const QString &udi )
{
    if( m_devices.contains(udi) ) {
        m_devices[udi]->removeCollection();
        m_devices.remove(udi);
    }
}

void UpnpCollectionFactory::createCollection( Solid::Device dev )
{
    QString udn = dev.udi().replace("/org/kde/upnp/uuid:", "");

    KIO::ListJob *job = KIO::listDir( "upnp-ms://" + udn + "/?searchcapabilities=1" );
    connect( job, SIGNAL(entries( KIO::Job *, const KIO::UDSEntryList & )),
            this, SLOT(slotSearchEntries( KIO::Job *, const KIO::UDSEntryList & )) );
    connect( job, SIGNAL(result(KJob *)), this, SLOT(slotSearchCapabilitiesDone(KJob*)) );
}

void UpnpCollectionFactory::slotSearchEntries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    Q_UNUSED( job );
    KIO::ListJob *lj = static_cast<KIO::ListJob*>( job );
    foreach( KIO::UDSEntry entry, list )
        m_capabilities[lj->url().host()] << entry.stringValue( KIO::UDSEntry::UDS_NAME );
}

void UpnpCollectionFactory::slotSearchCapabilitiesDone( KJob *job )
{
    KIO::ListJob *lj = static_cast<KIO::ListJob*>( job );
    QStringList searchCaps = m_capabilities[lj->url().host()];

    if( !job->error() ) {
        Solid::Device dev( "/org/kde/upnp/uuid:" + lj->url().host() );

        if( searchCaps.contains( "upnp:class" )
            && searchCaps.contains( "dc:title" )
            && searchCaps.contains( "upnp:artist" )
            && searchCaps.contains( "upnp:album" ) ) {
            kDebug() << "Supports all search meta-data required, using UpnpSearchCollection";
            m_devices[dev.udi()] = new UpnpSearchCollection( dev, searchCaps );
        }
        else {
            kDebug() << "Supported Search() meta-data" << searchCaps << "not enough. Using UpnpBrowseCollection";
            m_devices[dev.udi()] = new UpnpBrowseCollection( dev );
        }
        emit newCollection( m_devices[dev.udi()] );
    }
}

}
