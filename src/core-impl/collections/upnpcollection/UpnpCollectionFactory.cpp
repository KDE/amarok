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

#include <QUrl>
#include <QDBusMetaType>
#include <QDBusInterface>
#include <QDBusReply>

#include <KDirLister>
#include <KIO/ListJob>
#include <KIO/Scheduler>

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/StorageAccess>

#include "core/support/Debug.h"
#include "UpnpBrowseCollection.h"
#include "UpnpSearchCollection.h"

#include "dbuscodec.h"

namespace Collections {


UpnpCollectionFactory::UpnpCollectionFactory()
    : Collections::CollectionFactory()
{
    qRegisterMetaType<DeviceInfo>();
    qDBusRegisterMetaType< QHash<QString, QString> >();
    qDBusRegisterMetaType<DeviceInfo0_1_0>();
    qDBusRegisterMetaType<DeviceDetailsMap>();
}

UpnpCollectionFactory::~UpnpCollectionFactory()
{
}

void UpnpCollectionFactory::init()
{
    DEBUG_BLOCK

    if(    !cagibi0_1_0Init( QDBusConnection::sessionBus() )
        && !cagibi0_1_0Init( QDBusConnection::systemBus() )
        && !cagibi0_2_0Init( QDBusConnection::sessionBus() )
        && !cagibi0_2_0Init( QDBusConnection::systemBus() ) )
    {
        // we had problems with Cagibi
        return;
    }
}

bool UpnpCollectionFactory::cagibi0_1_0Init( QDBusConnection bus )
{
    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi",
                 "org.kde.Cagibi",
                 "devicesAdded",
                 this,
                 SLOT(slotDeviceAdded(DeviceTypeMap)) );

    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi",
                 "org.kde.Cagibi",
                 "devicesRemoved",
                 this,
                 SLOT(slotDeviceRemoved(DeviceTypeMap)) );

    m_iface = new QDBusInterface( "org.kde.Cagibi",
                                  "/org/kde/Cagibi",
                                  "org.kde.Cagibi",
                                  bus,
                                  this );

    QDBusReply<DeviceTypeMap> reply = m_iface->call( "allDevices" );
    if( !reply.isValid() )
    {
        debug() << "ERROR" << reply.error().message();
        return false;
    }
    else
    {
        slotDeviceAdded( reply.value() );
    }

    //Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    //connect( notifier, SIGNAL(deviceAdded(QString)), this, SLOT(slotDeviceAdded(QString)) );
    //connect( notifier, SIGNAL(deviceRemoved(QString)), this, SLOT(slotDeviceRemoved(QString)) );

    //foreach( Solid::Device device, Solid::Device::allDevices() ) {
    //    slotDeviceAdded(device.udi());
    //}

    m_initialized = true;
    return true;
}

bool UpnpCollectionFactory::cagibi0_2_0Init( QDBusConnection bus )
{
    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi/DeviceList",
                 "org.kde.Cagibi.DeviceList",
                 "devicesAdded",
                 this,
                 SLOT(slotDeviceAdded(DeviceTypeMap)) );

    bus.connect( "org.kde.Cagibi",
                 "/org/kde/Cagibi/DeviceList",
                 "org.kde.Cagibi.DeviceList",
                 "devicesRemoved",
                 this,
                 SLOT(slotDeviceRemoved(DeviceTypeMap)) );

    m_iface = new QDBusInterface( "org.kde.Cagibi",
                                  "/org/kde/Cagibi/DeviceList",
                                  "org.kde.Cagibi.DeviceList",
                                  bus,
                                  this );

    QDBusReply<DeviceTypeMap> reply = m_iface->call( "allDevices" );
    if( !reply.isValid() )
    {
        debug() << "ERROR" << reply.error().message();
        debug() << "Maybe cagibi is not installed.";
        return false;
    }
    else
    {
        slotDeviceAdded( reply.value() );
    }

    //Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    //connect( notifier, SIGNAL(deviceAdded(QString)), this, SLOT(slotDeviceAdded(QString)) );
    //connect( notifier, SIGNAL(deviceRemoved(QString)), this, SLOT(slotDeviceRemoved(QString)) );

    //foreach( Solid::Device device, Solid::Device::allDevices() ) {
    //    slotDeviceAdded(device.udi());
    //}

    m_initialized = true;
    return true;
}

void UpnpCollectionFactory::slotDeviceAdded( const DeviceTypeMap &map )
{
    foreach( const QString &udn, map.keys() ) {
        QString type = map[udn];
        debug() << "|||| DEVICE" << udn << type;
        if( type.startsWith("urn:schemas-upnp-org:device:MediaServer") )
            createCollection( udn );
    }
}

void UpnpCollectionFactory::slotDeviceRemoved( const DeviceTypeMap &map )
{
    foreach( QString udn, map.keys() ) {
        udn.remove("uuid:");
        if( m_devices.contains(udn) ) {
            m_devices[udn]->removeCollection();
            m_devices.remove(udn);
        }
    }
}

void UpnpCollectionFactory::createCollection( const QString &udn )
{
    debug() << "|||| Creating collection " << udn;
    DeviceInfo info;
    if(    !cagibi0_1_0DeviceDetails( udn, &info )
        && !cagibi0_2_0DeviceDetails( udn, &info ) )
    {
        return;
    }
    debug() << "|||| Creating collection " << info.uuid();
    KIO::ListJob *job = KIO::listDir( QUrl( "upnp-ms://" + info.uuid() + "/?searchcapabilities=1" ) );
    job->setProperty( "deviceInfo", QVariant::fromValue( info ) );
    connect( job, &KIO::ListJob::entries, this, &UpnpCollectionFactory::slotSearchEntries );
    connect( job, &KJob::result, this, &UpnpCollectionFactory::slotSearchCapabilitiesDone );
}

bool UpnpCollectionFactory::cagibi0_1_0DeviceDetails( const QString &udn, DeviceInfo *info )
{
    QDBusReply<DeviceInfo0_1_0> reply = m_iface->call( "deviceDetails", udn );
    if( !reply.isValid() ) {
        debug() << "Invalid reply from deviceDetails for" << udn << ". Skipping";
        debug() << "Error" << reply.error().message();
        return false;
    }
    *info = reply.value();
    return true;
}

bool UpnpCollectionFactory::cagibi0_2_0DeviceDetails( const QString &udn, DeviceInfo *info )
{
    QDBusReply<DeviceDetailsMap> reply = m_iface->call( "deviceDetails", udn );
    if( !reply.isValid() ) {
        debug() << "Invalid reply from deviceDetails for" << udn << ". Skipping";
        debug() << "Error" << reply.error().message();
        return false;
    }

    foreach( const QString &k, reply.value().keys() )
        debug() << k << reply.value()[k];
    DeviceInfo0_2_0 v( reply.value() );
    *info = v;
    return true;
}

void UpnpCollectionFactory::slotSearchEntries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    Q_UNUSED( job );
    KIO::ListJob *lj = static_cast<KIO::ListJob*>( job );
    foreach( const KIO::UDSEntry &entry, list )
        m_capabilities[lj->url().host()] << entry.stringValue( KIO::UDSEntry::UDS_NAME );
}

void UpnpCollectionFactory::slotSearchCapabilitiesDone( KJob *job )
{
    KIO::ListJob *lj = static_cast<KIO::ListJob*>( job );
    QStringList searchCaps = m_capabilities[lj->url().host()];

    if( !job->error() ) {
        DeviceInfo dev = job->property( "deviceInfo" ).value<DeviceInfo>();

        if( searchCaps.contains( "upnp:class" )
            && searchCaps.contains( "dc:title" )
            && searchCaps.contains( "upnp:artist" )
            && searchCaps.contains( "upnp:album" ) ) {
            qDebug() << "Supports all search meta-data required, using UpnpSearchCollection";
            m_devices[dev.uuid()] = new UpnpSearchCollection( dev, searchCaps );
        }
        else {
            qDebug() << "Supported Search() meta-data" << searchCaps << "not enough. Using UpnpBrowseCollection";
            m_devices[dev.uuid()] = new UpnpBrowseCollection( dev );
        }
        emit newCollection( m_devices[dev.uuid()] );
    }
}

}
