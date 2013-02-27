/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "DatabaseCollection.h"

#define DEBUG_PREFIX "DatabaseCollection"

#include "core/support/Debug.h"
#include "ScanManager.h"
#include "MountPointManager.h"

using namespace Collections;

DatabaseCollection::DatabaseCollection()
    : Collection()
    , m_mpm( 0 )
    , m_scanManager( 0 )
    , m_blockUpdatedSignalCount( 0 )
    , m_updatedSignalRequested( false )
{
}

DatabaseCollection::~DatabaseCollection()
{
    delete m_mpm;
}

QString
DatabaseCollection::collectionId() const
{
    return QLatin1String( "localCollection" );
}

QString
DatabaseCollection::prettyName() const
{
    return i18n( "Local Collection" );
}

KIcon
DatabaseCollection::icon() const
{
    return KIcon("drive-harddisk");
}

ScanManager*
DatabaseCollection::scanManager() const
{
    Q_ASSERT( m_scanManager );
    return m_scanManager;
}

MountPointManager*
DatabaseCollection::mountPointManager() const
{
    Q_ASSERT( m_mpm );
    return m_mpm;
}

void
DatabaseCollection::setMountPointManager( MountPointManager *mpm )
{
    Q_ASSERT( mpm );

    if( m_mpm )
    {
        disconnect( mpm, SIGNAL( deviceAdded(int) ), this, SLOT( slotDeviceAdded(int) ) );
        disconnect( mpm, SIGNAL( deviceRemoved(int) ), this, SLOT( slotDeviceRemoved(int) ) );
    }

    m_mpm = mpm;
    connect( mpm, SIGNAL( deviceAdded(int) ), this, SLOT( slotDeviceAdded(int) ) );
    connect( mpm, SIGNAL( deviceRemoved(int) ), this, SLOT( slotDeviceRemoved(int) ) );
}


QStringList
DatabaseCollection::collectionFolders() const
{
    return mountPointManager()->collectionFolders();
}

void
DatabaseCollection::setCollectionFolders( const QStringList &folders )
{
    mountPointManager()->setCollectionFolders( folders );
}


void
DatabaseCollection::blockUpdatedSignal()
{
    QMutexLocker locker( &m_mutex );
    m_blockUpdatedSignalCount ++;
}

void
DatabaseCollection::unblockUpdatedSignal()
{
    QMutexLocker locker( &m_mutex );

    Q_ASSERT( m_blockUpdatedSignalCount > 0 );
    m_blockUpdatedSignalCount --;

    // check if meanwhile somebody had updated the collection
    if( m_blockUpdatedSignalCount == 0 && m_updatedSignalRequested )
    {
        m_updatedSignalRequested = false;
        locker.unlock();
        emit updated();
    }
}

void
DatabaseCollection::collectionUpdated()
{
    QMutexLocker locker( &m_mutex );
    if( m_blockUpdatedSignalCount == 0 )
    {
        m_updatedSignalRequested = false;
        locker.unlock();
        emit updated();
    }
    else
    {
        m_updatedSignalRequested = true;
    }
}

bool
DatabaseCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::CollectionImport:
        case Capabilities::Capability::CollectionScan:
            return (bool) m_scanManager;
        default:
            break;
    }
    return Collection::hasCapabilityInterface( type );
}

Capabilities::Capability*
DatabaseCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::CollectionImport:
            return m_scanManager ? new DatabaseCollectionImportCapability( m_scanManager ) : 0;
        case Capabilities::Capability::CollectionScan:
            return m_scanManager ? new DatabaseCollectionScanCapability( m_scanManager ) : 0;
        default:
            break;
    }
    return Collection::createCapabilityInterface( type );
}

// --------- DatabaseCollectionScanCapability -------------

DatabaseCollectionScanCapability::DatabaseCollectionScanCapability( ScanManager* scanManager )
    : m_scanManager( scanManager )
{ }

DatabaseCollectionScanCapability::~DatabaseCollectionScanCapability()
{ }

void
DatabaseCollectionScanCapability::startFullScan()
{
    if( m_scanManager )
        m_scanManager->requestFullScan();
}

void
DatabaseCollectionScanCapability::startIncrementalScan( const QString &directory )
{
    if( m_scanManager )
        m_scanManager->requestIncrementalScan( directory );
}

void
DatabaseCollectionScanCapability::stopScan()
{
    if( m_scanManager )
        m_scanManager->abort( "Abort requested from DatabaseCollection::stopScan()" );
}

// --------- DatabaseCollectionImportCapability -------------

DatabaseCollectionImportCapability::DatabaseCollectionImportCapability( ScanManager* scanManager )
    : m_scanManager( scanManager )
{ }

DatabaseCollectionImportCapability::~DatabaseCollectionImportCapability()
{ }

void
DatabaseCollectionImportCapability::import( QIODevice *input, QObject *listener )
{
    DEBUG_BLOCK
    if( m_scanManager )
    {
        // ok. connecting of the signals is very specific for the SqlBatchImporter.
        // For now this works.

        /*
           connect( m_worker, SIGNAL( trackAdded( Meta::TrackPtr ) ),
           this, SIGNAL( trackAdded( Meta::TrackPtr ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackDiscarded( QString ) ),
           this, SIGNAL( trackDiscarded( QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ),
           this, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ),
           this, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ), Qt::QueuedConnection );
           connect( m_worker, SIGNAL( importError( QString ) ),
           this, SIGNAL( importError( QString ) ), Qt::QueuedConnection );
           */

        connect( m_scanManager, SIGNAL( finished() ),
                 listener, SIGNAL( importSucceeded() ) );
        connect( m_scanManager, SIGNAL( message( QString ) ),
                 listener, SIGNAL( showMessage( QString ) ) );

        m_scanManager->requestImport( input );
    }
}

