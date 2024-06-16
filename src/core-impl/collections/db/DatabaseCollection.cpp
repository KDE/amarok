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

#define DEBUG_PREFIX "DatabaseCollection"

#include <KLocalizedString>
#include "DatabaseCollection.h"

#include "core/support/Debug.h"
#include "scanner/GenericScanManager.h"
#include "MountPointManager.h"

using namespace Collections;

DatabaseCollection::DatabaseCollection()
    : Collection()
    , m_mpm( nullptr )
    , m_scanManager( nullptr )
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

QIcon
DatabaseCollection::icon() const
{
    return QIcon::fromTheme(QStringLiteral("drive-harddisk"));
}

GenericScanManager*
DatabaseCollection::scanManager() const
{
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
        disconnect( mpm, &MountPointManager::deviceAdded, this, &DatabaseCollection::slotDeviceAdded );
        disconnect( mpm, &MountPointManager::deviceRemoved, this, &DatabaseCollection::slotDeviceRemoved );
    }

    m_mpm = mpm;
    connect( mpm, &MountPointManager::deviceAdded, this, &DatabaseCollection::slotDeviceAdded );
    connect( mpm, &MountPointManager::deviceRemoved, this, &DatabaseCollection::slotDeviceRemoved );
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
        Q_EMIT updated();
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
        Q_EMIT updated();
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
            return true;
        default:
            return Collection::hasCapabilityInterface( type );
    }
}

Capabilities::Capability*
DatabaseCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::CollectionImport:
            return new DatabaseCollectionImportCapability( this );
        case Capabilities::Capability::CollectionScan:
            return new DatabaseCollectionScanCapability( this );
        default:
            return Collection::createCapabilityInterface( type );
    }
}

// --------- DatabaseCollectionScanCapability -------------

DatabaseCollectionScanCapability::DatabaseCollectionScanCapability( DatabaseCollection* collection )
    : m_collection( collection )
{
    Q_ASSERT( m_collection );
}

DatabaseCollectionScanCapability::~DatabaseCollectionScanCapability()
{ }

void
DatabaseCollectionScanCapability::startFullScan()
{
    QList<QUrl> urls;
    for( const QString& path : m_collection->mountPointManager()->collectionFolders() )
        urls.append( QUrl::fromLocalFile( path ) );

    m_collection->scanManager()->requestScan( urls, GenericScanManager::FullScan );
}

void
DatabaseCollectionScanCapability::startIncrementalScan( const QString &directory )
{
    if( directory.isEmpty() )
    {
        QList<QUrl> urls;
        for( const QString& path : m_collection->mountPointManager()->collectionFolders() )
            urls.append( QUrl::fromLocalFile( path ) );

        m_collection->scanManager()->requestScan( urls, GenericScanManager::UpdateScan );
    }
    else
    {
        QList<QUrl> urls;
        urls.append( QUrl::fromLocalFile( directory ) );

        m_collection->scanManager()->requestScan( urls,
                                                  GenericScanManager::PartialUpdateScan );
    }
}

void
DatabaseCollectionScanCapability::stopScan()
{
    m_collection->scanManager()->abort();
}

// --------- DatabaseCollectionImportCapability -------------

DatabaseCollectionImportCapability::DatabaseCollectionImportCapability( DatabaseCollection* collection )
    : m_collection( collection )
{
    Q_ASSERT( m_collection );
}

DatabaseCollectionImportCapability::~DatabaseCollectionImportCapability()
{ }

void
DatabaseCollectionImportCapability::import( QIODevice *input, QObject *listener )
{
    DEBUG_BLOCK

    if( listener )
    {
        // TODO: change import capability to collection action
        // TODO: why have listeners here and not for the scan capability
        // TODO: showMessage does not longer work like this, the scan result processor is doing this
        connect( m_collection->scanManager(), SIGNAL(succeeded()),
                 listener, SIGNAL(importSucceeded()) );
        connect( m_collection->scanManager(), SIGNAL(failed(QString)),
                 listener, SIGNAL(showMessage(QString)) );
    }

    m_collection->scanManager()->requestImport( input );
}

