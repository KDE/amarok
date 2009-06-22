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

#ifndef MEDIADEVICECOLLECTION_H
#define MEDIADEVICECOLLECTION_H

#include "ConnectionAssistant.h"

#include "Collection.h"
#include "MemoryCollection.h"

#include "mediadevicecollection_export.h"

#include <KIcon>

#include <QtGlobal>

class MediaDeviceCollection;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollectionFactoryBase : public Amarok::CollectionFactory
{
    Q_OBJECT

       public:
        virtual ~MediaDeviceCollectionFactoryBase();
        virtual void init();
    protected:
        MediaDeviceCollectionFactoryBase( ConnectionAssistant* assistant );

    public slots:
        // convenience slot
        void removeDevice( const QString &udi ) { deviceRemoved( udi ); }

    protected slots:
        virtual void deviceDetected( MediaDeviceInfo* info ); // detected type of device, connect it

    private slots:
                void deviceRemoved( const QString &udi );

    private:

        virtual Amarok::Collection* createCollection( MediaDeviceInfo* info ) = 0;

        ConnectionAssistant* m_assistant;
        QMap<QString, Amarok::Collection*> m_collectionMap;
};

template <class CollType>
class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollectionFactory : public MediaDeviceCollectionFactoryBase
{
    protected:
        MediaDeviceCollectionFactory( ConnectionAssistant *assistant )
        : MediaDeviceCollectionFactoryBase( assistant ) {}
        virtual ~MediaDeviceCollectionFactory() {}
    private:
        virtual Amarok::Collection* createCollection( MediaDeviceInfo* info )
        {
            return new CollType( info );
        }


};
/*
class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
    public:

        MediaDeviceCollection();
        virtual ~MediaDeviceCollection();

        virtual void startFullScan() = 0;
        virtual QueryMaker* queryMaker() = 0;

        virtual QString collectionId() const = 0;
        virtual QString prettyName() const = 0;
        virtual KIcon icon() const { return KIcon("drive-removable-media-usb"); }

        virtual void deviceRemoved() = 0;

};
*/

#endif
