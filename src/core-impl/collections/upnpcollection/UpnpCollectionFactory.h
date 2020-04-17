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

#ifndef UPNPCOLLECTIONFACTORY_H
#define UPNPCOLLECTIONFACTORY_H

#include "core/collections/Collection.h"
#include "core-impl/collections/upnpcollection/deviceinfo.h"

#include <solid/device.h>
#include <kio/udsentry.h>

#include <QDBusConnection>

namespace KIO {
    class Job;
}
class KJob;

class QDBusInterface;

typedef QHash<QString, QString> DeviceTypeMap;
Q_DECLARE_METATYPE( DeviceTypeMap )

namespace Collections {

class UpnpCollectionBase;

class UpnpCollectionFactory : public Collections::CollectionFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-upnpcollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

  public:
    UpnpCollectionFactory();
    ~UpnpCollectionFactory() override;

    void init() override;

  private:

  private Q_SLOTS:
    void slotDeviceAdded( const DeviceTypeMap &udi );
    void slotDeviceRemoved( const DeviceTypeMap &udi );
    void createCollection( const QString& );

    void slotSearchEntries( KIO::Job *job, const KIO::UDSEntryList &list );
    void slotSearchCapabilitiesDone( KJob * );

  private:
    bool cagibi0_1_0Init( QDBusConnection bus );
    bool cagibi0_2_0Init( QDBusConnection bus );
    bool cagibi0_1_0DeviceDetails( const QString &udn, DeviceInfo *info );
    bool cagibi0_2_0DeviceDetails( const QString &udn, DeviceInfo *info );
    QHash<QString, UpnpCollectionBase*> m_devices;
    QHash<QString, QStringList> m_capabilities;

    QDBusInterface *m_iface;
};

} //namespace Collections

#endif
