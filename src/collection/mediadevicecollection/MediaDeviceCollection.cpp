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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "MediaDeviceCollection"

#include "MediaDeviceCollection.h"
#include "MediaDeviceInfo.h"
#include "MediaDeviceMeta.h"

#include "meta/capabilities/CollectionCapability.h"
#include "MediaDeviceCollectionCapability.h"
#include "MediaDeviceDecoratorCapability.h"

#include "MediaDeviceMonitor.h"

#include "amarokconfig.h"
#include "Debug.h"
#include "MediaDeviceCache.h"
#include "MemoryQueryMaker.h"

#include <kdiskfreespaceinfo.h>

MediaDeviceCollectionFactoryBase::MediaDeviceCollectionFactoryBase( ConnectionAssistant* assistant )
    : Amarok::CollectionFactory()
    , m_assistant( assistant )
{
}


MediaDeviceCollectionFactoryBase::~MediaDeviceCollectionFactoryBase()
{
}

void
MediaDeviceCollectionFactoryBase::init()
{
    // When assistant identifies a device, Factory will attempt to build Collection
    connect( m_assistant, SIGNAL( identified(MediaDeviceInfo*) ), SLOT( slotDeviceDetected( MediaDeviceInfo* ) ) );

    // When assistant told to disconnect, Factory will disconnect
    // the device, and have the Collection destroyed
    connect( m_assistant, SIGNAL( disconnected(QString)), SLOT( slotDeviceDisconnected(QString)) );

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
            coll->deleteCollection();
        }
    }

    return;
}

//MediaDeviceCollection

MediaDeviceCollection::MediaDeviceCollection()
    : Collection()
    , m_ejectAction( 0 )
    , m_usedCapacity( -1 )
    , m_totalCapacity( -1 )
    , m_mc( new MemoryCollection() )
{
    connect( this, SIGNAL( attemptConnectionDone(bool)),
             this, SLOT( slotAttemptConnectionDone(bool)) );
}

MediaDeviceCollection::~MediaDeviceCollection()
{
    DEBUG_BLOCK
}

void
MediaDeviceCollection::initCapacities()
{
    m_usedCapacity = m_handler->usedcapacity();
    m_totalCapacity = m_handler->totalcapacity();
}

QueryMaker*
MediaDeviceCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

UserPlaylistProvider*
MediaDeviceCollection::userPlaylistProvider()
{
    return m_handler->provider();
}

QString MediaDeviceCollection::collectionId() const
{
    return m_udi;
}


void
MediaDeviceCollection::startFullScan()
{
}

void
MediaDeviceCollection::startFullScanDevice()
{
    DEBUG_BLOCK
    // If handler successfully connected to device

    m_handler->parseTracks();
    //emit collectionReady( this );
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
        startFullScanDevice();
    }
    else
    {
        debug() << "connection failed, not scanning";
        emit collectionDisconnected( m_udi );
    }
}

/// CollectionCapability for Disconnect Action

bool
MediaDeviceCollection::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::Collection:
        case Meta::Capability::Decorator:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
MediaDeviceCollection::createCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::Collection:
            return new Meta::MediaDeviceCollectionCapability( this );
        case Meta::Capability::Decorator:
            return new Meta::MediaDeviceDecoratorCapability( this );
        default:
            return 0;
    }
}

bool
MediaDeviceCollection::hasCapacity() const
{
    return totalCapacity() > 0;
}

float
MediaDeviceCollection::usedCapacity() const
{
    if( m_totalCapacity >= 0 )
        return m_usedCapacity;
    else
        return 0.0;
}

float
MediaDeviceCollection::totalCapacity() const
{
    if( m_totalCapacity < 0 )
        const_cast<MediaDeviceCollection*>(this)->initCapacities();
    return m_totalCapacity;
}

void
MediaDeviceCollection::emitCollectionReady()
{
    emit collectionReady( this );
}

QAction *
MediaDeviceCollection::ejectAction() const
{
    if( !m_ejectAction )
    {
        m_ejectAction = new QAction( KIcon( "media-eject" ), i18n( "&Disconnect Device" ), 0 );
        m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );

        connect( m_ejectAction, SIGNAL( triggered() ), SLOT( disconnectDevice() ) );
    }
    return m_ejectAction;
}

#include "MediaDeviceCollection.moc"
