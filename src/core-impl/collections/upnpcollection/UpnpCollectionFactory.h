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

typedef QHash<QString, QString> DeviceTypeMap;
Q_DECLARE_METATYPE(DeviceTypeMap);

namespace Collections {

class UpnpCollection;

class UpnpCollectionFactory : public Collections::CollectionFactory
{
  Q_OBJECT
  public:
    UpnpCollectionFactory( QObject *parent, const QVariantList &args );
    virtual ~UpnpCollectionFactory();

    void init();


  private:

  private slots:
    void slotDevicesAdded( const DeviceTypeMap &map );
    void slotDevicesRemoved( const DeviceTypeMap &map );
    void createCollection( const QString &udn );

  private:
    QHash<QString, UpnpCollection*> m_devices;
};

} //namespace Collections

#endif
