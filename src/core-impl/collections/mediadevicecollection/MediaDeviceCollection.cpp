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

#include "MediaDeviceMonitor.h"
#include "core/capabilities/ActionsCapability.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"
#include "core-impl/collections/mediadevicecollection/support/MediaDeviceInfo.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

using namespace Collections;

MediaDeviceCollectionFactoryBase::MediaDeviceCollectionFactoryBase( ConnectionAssistant* assistant )
    : Collections::CollectionFactory()
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
    connect( m_assistant, &ConnectionAssistant::identified, this, &MediaDeviceCollectionFactoryBase::slotDeviceDetected );

    // When assistant told to disconnect, Factory will disconnect
    // the device, and have the Collection destroyed
    connect( m_assistant, &ConnectionAssistant::disconnected, this, &MediaDeviceCollectionFactoryBase::slotDeviceDisconnected );

    // Register the device type with the Monitor
    MediaDeviceMonitor::instance()->registerDeviceType( m_assistant );

    m_initialized = true;
}

void MediaDeviceCollectionFactoryBase::slotDeviceDetected(MediaDeviceInfo* info)
{
    MediaDeviceCollection* coll = nullptr;
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
            connect( coll, &Collections::MediaDeviceCollection::collectionReady,
                     this, &MediaDeviceCollectionFactoryBase::newCollection );
            connect( coll, &Collections::MediaDeviceCollection::collectionDisconnected,
                     this, &MediaDeviceCollectionFactoryBase::slotDeviceDisconnected );
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
    , m_ejectAction( nullptr )
    , m_mc( new MemoryCollection() )
{
    connect( this, &MediaDeviceCollection::attemptConnectionDone,
             this, &MediaDeviceCollection::slotAttemptConnectionDone );
}

MediaDeviceCollection::~MediaDeviceCollection()
{
    DEBUG_BLOCK
}

QueryMaker*
MediaDeviceCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString MediaDeviceCollection::collectionId() const
{
    return m_udi;
}

void
MediaDeviceCollection::startFullScanDevice()
{
    DEBUG_BLOCK
    // If handler successfully connected to device

    m_handler->parseTracks();
    //Q_EMIT collectionReady( this );
}

Meta::MediaDeviceHandler*
MediaDeviceCollection::handler()
{
    return m_handler;
}

void
MediaDeviceCollection::eject()
{
    DEBUG_BLOCK
    // Do nothing special here.
    Q_EMIT collectionDisconnected( m_udi );
}

void
MediaDeviceCollection::deleteCollection()
{
    DEBUG_BLOCK
    Q_EMIT deletingCollection();
    Q_EMIT remove();
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
        Q_EMIT collectionDisconnected( m_udi );
    }
}

/// CollectionCapability for Disconnect Action

bool
MediaDeviceCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            return true;

        default:
            return false;
    }
}

Capabilities::Capability*
MediaDeviceCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            {
                QList< QAction* > actions;
                actions << m_handler->collectionActions();
                actions << ejectAction();
                return new Capabilities::ActionsCapability( actions );
            }
        default:
            return nullptr;
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
    return m_handler->usedcapacity();
}

float
MediaDeviceCollection::totalCapacity() const
{
    return m_handler->totalcapacity();
}

void
MediaDeviceCollection::emitCollectionReady()
{
    Q_EMIT collectionReady( this );
}

QAction *
MediaDeviceCollection::ejectAction() const
{
    if( !m_ejectAction )
    {
        m_ejectAction = new QAction( QIcon::fromTheme( QStringLiteral("media-eject") ), i18n( "&Disconnect Device" ),
                                     const_cast<MediaDeviceCollection*>(this) );
        m_ejectAction->setProperty( "popupdropper_svg_id", QStringLiteral("eject") );

        connect( m_ejectAction, &QAction::triggered, this, &MediaDeviceCollection::eject );
    }
    return m_ejectAction;
}

