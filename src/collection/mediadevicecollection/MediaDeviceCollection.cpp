/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2006 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "MediaDeviceCollection"

#include "MediaDeviceCollection.h"
#include "MediaDeviceInfo.h"
#include "MediaDeviceMeta.h"
//#include "MediaDeviceHandler.h"

#include "meta/capabilities/CollectionCapability.h"
#include "CollectionCapabilityMediaDevice.h"

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
}

void
MediaDeviceCollectionFactoryBase::init()
{
    //DEBUG_BLOCK

    // When assistant identifies a device, Factory will attempt to build Collection
    connect( m_assistant, SIGNAL( identified(MediaDeviceInfo*) )
    , SLOT( slotDeviceDetected( MediaDeviceInfo* ) ) );

    // When assistant told to disconnect, Factory will disconnect
    // the device, and have the Collection destroyed
    connect( m_assistant, SIGNAL( disconnected(QString))
    , SLOT( slotDeviceDisconnected(QString)) );

    // Register the device type with the Monitor
    MediaDeviceMonitor::instance()->registerDeviceType( m_assistant );

}

void MediaDeviceCollectionFactoryBase::slotDeviceDetected(MediaDeviceInfo* info)
{
    MediaDeviceCollection* coll = 0;
    // If device not already connected to
    if( !m_collectionMap.contains( info->udi() ) )
    {
        // create the collection using the info provided
        coll = createCollection( info );
        // if collection successfully created,
        // aka device connected to, then...
        if( coll )
        {
            // insert it into the map of known collections
            m_collectionMap.insert( info->udi(), coll );
            connect( coll, SIGNAL( collectionReady( Amarok::Collection* ) ),
                     this, SIGNAL( newCollection(Amarok::Collection*)) );
            connect( coll, SIGNAL( collectionDisconnected( const QString& ) ),
                     this, SLOT( slotDeviceDisconnected( const QString& ) ) );
            coll->init();
        }
    }
}

void
MediaDeviceCollectionFactoryBase::slotDeviceDisconnected( const QString &udi )
{
    DEBUG_BLOCK
    // If device is known about
    if( m_collectionMap.contains( udi ) )
    {
        // Pull collection for the udi out of map
        MediaDeviceCollection* coll = m_collectionMap[ udi ];
        // If collection exists/found
        if( coll )
        {
            // Remove collection from map
            m_collectionMap.remove( udi );
            // Have Collection disconnect device
            // and destroy itself
//            coll->disconnectDevice();
            coll->deleteCollection();
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
: Collection()
, MemoryCollection()
{
    connect( this, SIGNAL( attemptConnectionDone(bool)),
             this, SLOT( slotAttemptConnectionDone(bool)) );
}


MediaDeviceCollection::~MediaDeviceCollection()
{
    DEBUG_BLOCK
}

QueryMaker*
MediaDeviceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString MediaDeviceCollection::collectionId() const
{
    return m_udi;
}


void
MediaDeviceCollection::startFullScan()
{
    DEBUG_BLOCK
    // If handler successfully connected to device

    m_handler->parseTracks();
    emit collectionReady( this );
//    collectionUpdated();
}

Meta::MediaDeviceHandler*
MediaDeviceCollection::handler()
{
    return m_handler;
}

/// disconnectDevice is called when ConnectionAssistant
/// is told it is to be disconnected.  This could be
/// because another part of Amarok (e.g. applet) told it to
/// or because the MediaDeviceMonitor noticed it disconnect
void
MediaDeviceCollection::disconnectDevice()
{
    DEBUG_BLOCK
    // First, attempt to write to database,
    // and regardless of success remove
    // the collection.
    // NOTE: this also calls the handler's destructor
    // which gives it a chance to do last-minute cleanup
//    connect( m_handler, SIGNAL( databaseWritten( bool ) )
//    , SIGNAL( remove() ) );

//    m_handler->writeDatabase();

    emit collectionDisconnected( m_udi );
}

void
MediaDeviceCollection::deleteCollection()
{
    DEBUG_BLOCK
    emit deletingCollection();
    emit remove();
}

void
MediaDeviceCollection::slotAttemptConnectionDone( bool success )
{
    DEBUG_BLOCK
    if( success )
    {
        debug() << "starting full scan";
        // TODO: thread the track parsing?
        startFullScan();
    }
    else
        debug() << "connection failed, not scanning";
}

/// CollectionCapability for Disconnect Action

bool
MediaDeviceCollection::hasCapabilityInterface( Meta::Capability::Type type ) const
{
        DEBUG_BLOCK
                switch( type )
                {
                case Meta::Capability::Collection:
                    return true;

                default:
                    return false;
                }
}

Meta::Capability*
MediaDeviceCollection::createCapabilityInterface( Meta::Capability::Type type )
{
        DEBUG_BLOCK
                switch( type )
                {
                case Meta::Capability::Collection:
                    return new Meta::CollectionCapabilityMediaDevice( this );
                default:
                    return 0;
                }
}

#include "MediaDeviceCollection.moc"

