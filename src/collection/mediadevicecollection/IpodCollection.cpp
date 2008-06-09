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
//#include "ipodmediadevice.h"
#include "MediaDeviceCache.h"
#include "MemoryQueryMaker.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>

//#include <QDir>
#include <QStringList>


AMAROK_EXPORT_PLUGIN( IpodCollectionFactory )

IpodCollectionFactory::IpodCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

IpodCollectionFactory::~IpodCollectionFactory()
{
  //    delete m_browser;
}

void
IpodCollectionFactory::init()
{
  DEBUG_BLOCK

    /* Cache stuff */
      MediaDeviceCache::instance()->refreshCache();
  QStringList udiList = MediaDeviceCache::instance()->getAll();

  /* Solid stuff */
  Solid::Device device;

  /* Collection stuff */

  IpodCollection *coll = 0;



  foreach(const QString &udi, udiList )
      {
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

	if(device.product() == "iPod")
            coll = new IpodCollection(MediaDeviceCache::instance()->volumeMountPoint(udi));

      }



    //    debug() << "IpodCollection found " << udiList;

    /* test ipodcollection constructor */
    //    coll = new IpodCollection();
    if(coll)
        delete coll;

    return;
}

//IpodCollection

IpodCollection::IpodCollection( const QString &mountPoint )
    : Collection()
    , MemoryCollection()
    , m_mountPoint(mountPoint)
{
    DEBUG_BLOCK



    /* test out using libgpod */

    m_itdb = 0;
    m_itdb = itdb_new();
    if(m_itdb) {
        debug() << "Itunes database created!";
        itdb_free(m_itdb);
    }
    else
        debug() << "Itunes database not created!";


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
     return "filler";
}

QString
IpodCollection::prettyName() const
{
    return "prettyfiller";
}

#include "IpodCollection.moc"

