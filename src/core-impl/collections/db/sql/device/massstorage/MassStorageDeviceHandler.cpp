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

#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>

#include <QUrl>
#include <QDir>
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

MassStorageDeviceHandler::MassStorageDeviceHandler(): DeviceHandler()
{
}

MassStorageDeviceHandler::MassStorageDeviceHandler( int deviceId, const QString &mountPoint, const QString &udi )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_udi( udi )
{
    DEBUG_BLOCK
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
    return QStringLiteral("uuid");
}

int MassStorageDeviceHandler::getDeviceID()
{
    return m_deviceID;
}

const QString &MassStorageDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void MassStorageDeviceHandler::getURL( QUrl &absolutePath, const QUrl &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath = absolutePath.adjusted(QUrl::StripTrailingSlash);
    absolutePath.setPath(absolutePath.path() + QLatin1Char('/') + ( relativePath.path() ));
    absolutePath.setPath( QDir::cleanPath(absolutePath.path()) );
}

void MassStorageDeviceHandler::getPlayableURL( QUrl &absolutePath, const QUrl &relativePath )
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
    return QStringLiteral("uuid");
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
    DEBUG_BLOCK
    const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
    if( !volume )
    {
        debug() << "found no volume";
        return false;
    }
    if( volume->uuid().isEmpty() )
        debug() << "has empty uuid";
    if( volume->isIgnored() )
        debug() << "volume is ignored";
    if( excludedFilesystem( volume->fsType() ) )
        debug() << "excluded filesystem of type " << volume->fsType();
    return volume && !volume->uuid().isEmpty()
           && !volume->isIgnored() && !excludedFilesystem( volume->fsType() );
}

MassStorageDeviceHandlerFactory::~MassStorageDeviceHandlerFactory( )
{
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const KSharedConfigPtr&, QSharedPointer<SqlStorage> ) const
{
    return nullptr;
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const Solid::Device &device, const QString &udi, QSharedPointer<SqlStorage> s ) const
{
    DEBUG_BLOCK
    if( !s )
    {
        debug() << "!s, returning 0";
        return nullptr;
    }
    const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
    const Solid::StorageAccess *volumeAccess = device.as<Solid::StorageAccess>();
    if( !volume || !volumeAccess )
    {
        debug() << "Volume isn't valid, can't create a handler";
        return nullptr;
    }
    if( volumeAccess->filePath().isEmpty() )
    {
        debug() << "not mounted, can't do anything";
        return nullptr; // It's not mounted, we can't do anything.
    }
    QStringList ids = s->query( QStringLiteral( "SELECT id, label, lastmountpoint "
                                         "FROM devices WHERE type = 'uuid' "
                                         "AND uuid = '%1';" ).arg( volume->uuid() ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing UUID config for ID " << ids[0] << " , uuid " << volume->uuid();
        s->query( QStringLiteral( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                           "id = %1;" )
                           .arg( ids[0],
                                 s->escape( volumeAccess->filePath() ) ) );
        return new MassStorageDeviceHandler( ids[0].toInt(), volumeAccess->filePath(), udi );
    }
    else
    {
        const int id = s->insert( QStringLiteral( "INSERT INTO devices( type, uuid, lastmountpoint ) "
                                           "VALUES ( 'uuid', '%1', '%2' );" )
                                           .arg( volume->uuid(),
                                                 s->escape( volumeAccess->filePath() ) ),
                                           QStringLiteral("devices") );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=uuid, uuid=" << volume->uuid();
            return nullptr;
        }
        debug() << "Created new UUID device with ID " << id << " , uuid " << volume->uuid();
        return new MassStorageDeviceHandler( id, volumeAccess->filePath(), udi );
    }
}

bool
MassStorageDeviceHandlerFactory::excludedFilesystem( const QString &fstype ) const
{
    return fstype.isEmpty() ||
           fstype.indexOf( QStringLiteral("smb") ) != -1 ||
           fstype.indexOf( QStringLiteral("cifs") ) != -1 ||
           fstype.indexOf( QStringLiteral("nfs") ) != -1 ||
           fstype == QStringLiteral("udf")  ||
           fstype == QStringLiteral("iso9660") ;
}
