/*
   Mostly taken from Daap code:
   Copyright (C) 2006 Ian Monroe <ian@monroe.nu>
   Copyright (C) 2006 Seb Ruiz <ruiz@kde.org>
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#define DEBUG_PREFIX "IpodCollection"

#include "IpodCollection.h"
#include "IpodMeta.h"

#include "amarokconfig.h"
#include "Debug.h"

#include "MediaDeviceCache.h"
#include "MemoryQueryMaker.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>


#include <QStringList>


AMAROK_EXPORT_PLUGIN( IpodCollectionFactory )

IpodCollectionFactory::IpodCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

IpodCollectionFactory::~IpodCollectionFactory()
{

}

void
IpodCollectionFactory::init()
{
    DEBUG_BLOCK

    IpodCollection *coll = 0;

    /* Refresh cache */
    MediaDeviceCache::instance()->refreshCache();
    QStringList udiList = MediaDeviceCache::instance()->getAll();

    /* poll udi list for ipod */
    foreach(const QString &udi, udiList )
        {
            /* if iPod found, make collection */
            if( isIpod( udi ) )
            {
                coll = new IpodCollection(MediaDeviceCache::instance()->volumeMountPoint(udi));
                if ( coll )
                {
                    emit newCollection( coll );
                    debug() << "emitting new ipod collection";
                }
            }

        }

    // connect to device cache
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceAdded( const QString& ) ),
              SLOT(  deviceAdded( const QString& ) ) );
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceRemoved( const QString& ) ),
              SLOT(  deviceRemoved( const QString& ) ) );

    return;
}

bool
IpodCollectionFactory::isIpod( const QString &udi )
{

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

    /* if iPod found, return true */
    return (device.product() == "iPod");

}

void
IpodCollectionFactory::deviceAdded(  const QString &udi )
{
    DEBUG_BLOCK

        debug() << "New device added, testing if Ipod";

    IpodCollection *coll = 0;
    /* if iPod found, make collection */
    if( isIpod( udi ) )
    {
        coll = new IpodCollection(MediaDeviceCache::instance()->volumeMountPoint(udi));
        if ( coll )
        {
            emit newCollection( coll );
            debug() << "New Ipod Found, collection created!";
        }
    }

    return;
}

void
IpodCollectionFactory::deviceRemoved( const QString &udi )
{
    return;
}

void
IpodCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
        IpodCollection *collection = dynamic_cast<IpodCollection*>(  sender() );
    if (  collection )
    {
        debug() << "emitting ipod collection newcollection";
        emit newCollection(  collection );
    }
}

//IpodCollection

IpodCollection::IpodCollection( const QString &mountPoint )
    : Collection()
    , MemoryCollection()
    , m_mountPoint( mountPoint )
    , m_handler( 0 )
{
    DEBUG_BLOCK

    m_handler = new Ipod::IpodHandler( this, m_mountPoint, this );

    m_handler->printTracks();
    m_handler->parseTracks();

    emit collectionReady();
}



IpodCollection::~IpodCollection()
{

}

void
IpodCollection::startFullScan()
{
    //ignore
}

QueryMaker*
IpodCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
IpodCollection::collectionId() const
{
     return m_mountPoint;
}

QString
IpodCollection::prettyName() const
{
    return "Ipod at " + m_mountPoint;
}

#include "IpodCollection.moc"

