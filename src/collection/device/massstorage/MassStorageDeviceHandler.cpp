/****************************************************************************************
 * Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "MassStorageDeviceHandler"

#include "MassStorageDeviceHandler.h"

AMAROK_EXPORT_PLUGIN( MassStorageDeviceHandlerFactory )

#include "Debug.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"

#include <kconfig.h>
#include <kurl.h>
#include <solid/storagevolume.h>
#include <solid/storageaccess.h>


MassStorageDeviceHandler::MassStorageDeviceHandler(): DeviceHandler()
{
}

MassStorageDeviceHandler::MassStorageDeviceHandler( int deviceId, const QString &mountPoint, const QString &udi )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_udi( udi )
{
}

MassStorageDeviceHandler::~MassStorageDeviceHandler()
{
}

bool MassStorageDeviceHandler::isAvailable() const
{
    return true;
}


QString MassStorageDeviceHandler::type() const
{
    return "uuid";
}

int MassStorageDeviceHandler::getDeviceID()
{
    return m_deviceID;
}

const QString &MassStorageDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void MassStorageDeviceHandler::getURL( KUrl &absolutePath, const KUrl &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void MassStorageDeviceHandler::getPlayableURL( KUrl &absolutePath, const KUrl &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool MassStorageDeviceHandler::deviceMatchesUdi( const QString &udi ) const
{
    return m_udi == udi;
}

///////////////////////////////////////////////////////////////////////////////
// class MassStorageDeviceHandlerFactory
///////////////////////////////////////////////////////////////////////////////

QString MassStorageDeviceHandlerFactory::type( ) const
{
    return "uuid";
}

bool MassStorageDeviceHandlerFactory::canCreateFromMedium( ) const
{
    return true;
}

bool MassStorageDeviceHandlerFactory::canCreateFromConfig( ) const
{
    return false;
}

bool MassStorageDeviceHandlerFactory::canHandle( const Solid::Device &device ) const
{
    const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
    return volume && !volume->uuid().isEmpty()
           && !volume->isIgnored() && !excludedFilesystem( volume->fsType() );
}

MassStorageDeviceHandlerFactory::MassStorageDeviceHandlerFactory( )
{
}

MassStorageDeviceHandlerFactory::~MassStorageDeviceHandlerFactory( )
{
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( KSharedConfigPtr ) const
{
    return 0;
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const Solid::Device &device, const QString &udi ) const
{
    DEBUG_BLOCK
    SqlStorage *s = CollectionManager::instance()->sqlStorage();
    Q_ASSERT( s );
    const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
    const Solid::StorageAccess *volumeAccess = device.as<Solid::StorageAccess>();
    if( !volume || !volumeAccess )
    {
        debug() << "Volume isn't valid, can't create a handler";
        return 0;
    }
    if( volumeAccess->filePath().isEmpty() )
        return 0; // It's not mounted, we can't do anything.
    QStringList ids = s->query( QString( "SELECT id, label, lastmountpoint "
                                         "FROM devices WHERE type = 'uuid' "
                                         "AND uuid = '%1';" ).arg( volume->uuid() ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing UUID config for ID " << ids[0] << " , uuid " << volume->uuid();
        s->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                           "id = %1;" )
                           .arg( ids[0] )
                           .arg( s->escape( volumeAccess->filePath() ) ) );
        return new MassStorageDeviceHandler( ids[0].toInt(), volumeAccess->filePath(), volume->uuid() );
    }
    else
    {
        const int id = s->insert( QString( "INSERT INTO devices( type, uuid, lastmountpoint ) "
                                           "VALUES ( 'uuid', '%1', '%2' );" )
                                           .arg( volume->uuid() )
                                           .arg( s->escape( volumeAccess->filePath() ) ),
                                           "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=uuid, uuid=" << volume->uuid();
            return 0;
        }
        debug() << "Created new UUID device with ID " << id << " , uuid " << volume->uuid();
        return new MassStorageDeviceHandler( id, volumeAccess->filePath(), udi );
    }
}

bool
MassStorageDeviceHandlerFactory::excludedFilesystem( const QString &fstype ) const
{
    return fstype.isEmpty() ||
           fstype.indexOf( "smb" ) != -1 ||
           fstype.indexOf( "cifs" ) != -1 ||
           fstype.indexOf( "nfs" ) != -1 ||
           fstype == "udf"  ||
           fstype == "iso9660" ;
}
