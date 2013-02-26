/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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

using namespace Collections;

DatabaseCollection::DatabaseCollection( const QString &id, const QString &prettyName )
    : Collection()
    , m_mpm( 0 )
    , m_scanManager( 0 )
    , m_blockUpdatedSignalCount( 0 )
    , m_updatedSignalRequested( false )
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}

DatabaseCollection::~DatabaseCollection()
{
    delete m_mpm;
}

QString
DatabaseCollection::collectionId() const
{
    return m_collectionId;
}

QString
DatabaseCollection::prettyName() const
{
    return m_prettyName;
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


