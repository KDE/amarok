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

#define DEBUG_PREFIX "MediaDeviceCollection"

#include "MediaDeviceCollection.h"
#include "MediaDeviceInfo.h"
#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include "MediaDeviceMonitor.h"

#include "amarokconfig.h"
//#include "Debug.h"
#include "MediaDeviceCache.h"
#include "MemoryQueryMaker.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>


//AMAROK_EXPORT_PLUGIN( MediaDeviceCollectionFactory )

MediaDeviceCollectionFactoryBase::MediaDeviceCollectionFactoryBase( ConnectionAssistant* assistant )
    : Amarok::CollectionFactory()
    , m_assistant( assistant )
    , m_collectionMap()
{
}


MediaDeviceCollectionFactoryBase::~MediaDeviceCollectionFactoryBase()
{
  //    delete m_browser;
}

void
MediaDeviceCollectionFactoryBase::init()
{
    //DEBUG_BLOCK

    // When assistant identifies a device, Factory will attempt to build Collection
    connect( m_assistant, SIGNAL( identified(MediaDeviceInfo*) )
    , SLOT( deviceDetected( MediaDeviceInfo* ) ) );

    // Register the device type with the Monitor
    MediaDeviceMonitor::instance()->registerDeviceType( m_assistant );

}

void MediaDeviceCollectionFactoryBase::deviceDetected(MediaDeviceInfo* info) {
    Amarok::Collection* coll = 0;
    if( !m_collectionMap.contains( info->udi() ) )
    {
        //coll = createCollection( info );
        coll = createCollection( info );
        if( coll )
        {
            // TODO: connect to MediaDeviceMonitor signals
 //           connect( coll, SIGNAL( collectionDisconnected( const QString &) ),
//                     this, SLOT( slotCollectionDisconnected( const QString & ) ) );
            m_collectionMap.insert( info->udi(), coll );
            //debug() << "emitting new ipod collection";
            emit newCollection( coll );
        }
    }
}



void
MediaDeviceCollectionFactoryBase::deviceRemoved( const QString &udi )
{
    //DEBUG_BLOCK
    if( m_collectionMap.contains( udi ) )
    {
        Amarok::Collection* coll = m_collectionMap[ udi ];
        if( coll )
        {
            m_collectionMap.remove( udi ); // remove from map
            coll->deviceRemoved();  //collection will be deleted by collectionmanager
        }
//        else
            //warning() << "collection already null";
    }
    //else
        //warning() << "removing non-existent device";

    return;
}


//MediaDeviceCollection

MediaDeviceCollection::MediaDeviceCollection()
: m_handler( new MediaDeviceHandler( 
{

}


MediaDeviceCollection::~MediaDeviceCollection()
{
}

QueryMaker*
MediaDeviceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

void
MediaDeviceCollection::startFullScan()
{
    m_handler = new MediaDevice::MediaDeviceHandler( this, m_mountPoint, this );

    if( m_handler->succeeded() )
    {
        m_handler->parseTracks();
        emit collectionReady();
    }
}

#include "MediaDeviceCollection.moc"

