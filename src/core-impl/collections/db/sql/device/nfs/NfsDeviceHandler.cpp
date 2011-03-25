/****************************************************************************************
 * Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2011 Peter C. Ndikuwera <pndiku@gmail.com>                             *
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

#define DEBUG_PREFIX "NfsDeviceHandler"
 
#include "NfsDeviceHandler.h"

AMAROK_EXPORT_DEVICE_PLUGIN( nfs, NfsDeviceHandlerFactory )

#include "core/support/Debug.h"
#include "core/collections/support/SqlStorage.h"

#include <kconfig.h>
#include <kurl.h>
#include <kmountpoint.h>
#include <solid/storagevolume.h>
#include <solid/storageaccess.h>

NfsDeviceHandler::NfsDeviceHandler( int deviceId, const QString &server, const QString &share, const QString &mountPoint, const QString &udi )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_server( server )
    , m_share( share )
    , m_mountPoint( mountPoint )
    , m_udi( udi )
{
  DEBUG_BLOCK
}

NfsDeviceHandler::NfsDeviceHandler( int deviceId, const QString &mountPoint, const QString &udi )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_udi( udi )
{
  DEBUG_BLOCK
}

NfsDeviceHandler::~NfsDeviceHandler()
{
}

bool
NfsDeviceHandler::isAvailable() const
{
    return true;
}


QString
NfsDeviceHandler::type() const
{
    return "nfs";
}

int
NfsDeviceHandler::getDeviceID()
{
    return m_deviceID;
}

const QString &NfsDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void NfsDeviceHandler::getURL( KUrl &absolutePath, const KUrl &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void NfsDeviceHandler::getPlayableURL( KUrl &absolutePath, const KUrl &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool NfsDeviceHandler::deviceMatchesUdi( const QString &udi ) const
{
  return m_udi == udi;
}

///////////////////////////////////////////////////////////////////////////////
// class NfsDeviceHandlerFactory
///////////////////////////////////////////////////////////////////////////////

QString NfsDeviceHandlerFactory::type( ) const
{
    return "nfs";
}

void NfsDeviceHandlerFactory::init()
{
    m_initialized = true;
}

bool NfsDeviceHandlerFactory::canCreateFromMedium( ) const
{
    return true;
}

bool NfsDeviceHandlerFactory::canCreateFromConfig( ) const
{
    return false;
}

bool NfsDeviceHandlerFactory::canHandle( const Solid::Device &device ) const
{
    DEBUG_BLOCK

    const Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
 
    if( !access || access->filePath().isEmpty() )
    {
        debug() << "Device not accessible";
        return false;
    }

    // find mount point
    KMountPoint::Ptr m = KMountPoint::currentMountPoints().findByPath( access->filePath() );

    if ( m && ( m->mountType() == "nfs" || m->mountType() == "nfs4" ))
      return true;

    return false;
}

NfsDeviceHandlerFactory::NfsDeviceHandlerFactory( QObject *parent, const QVariantList &args )
    : DeviceHandlerFactory( parent, args )
{
    KPluginInfo pluginInfo( "amarok_device_nfs.desktop", "services" );
    pluginInfo.setConfig( Amarok::config("Device_MassiveStorage") );
    m_info = pluginInfo;
}

NfsDeviceHandlerFactory::~NfsDeviceHandlerFactory( )
{
}

DeviceHandler *
NfsDeviceHandlerFactory::createHandler( KSharedConfigPtr, SqlStorage* ) const
{
    return 0;
}

DeviceHandler *
NfsDeviceHandlerFactory::createHandler( const Solid::Device &device, const QString &udi, SqlStorage *s ) const
{
    DEBUG_BLOCK
    if( !s )
    {
        debug() << "!s, returning 0";
        return 0;
    }

    const Solid::StorageAccess *access = device.as<Solid::StorageAccess>();
    if( !access )
    {
        debug() << "Device isn't valid, can't create a handler";
        return 0;
    }
    if( access->filePath().isEmpty() )
    {
        debug() << "not mounted, can't do anything";
        return 0; // It's not mounted, we can't do anything.
    }

    // find mount point
    KMountPoint::Ptr m = KMountPoint::currentMountPoints().findByPath( access->filePath() );

    QString server = m->mountedFrom().section( ':', 0, 0 );
    QString share = m->mountedFrom().section( ':', 1, 1 );
    QStringList ids = s->query( QString( "SELECT id, label, lastmountpoint "
                                         "FROM devices WHERE type = 'nfs' "
                                         "AND servername = '%1' AND sharename = '%2';" )
                                         .arg( s->escape( server ) )
                                         .arg( s->escape( share ) ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing NFS config for ID " << ids[0] << " , server " << server << " ,share " << share;
        s->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                           "id = %1;" )
                           .arg( ids[0] )
                           .arg( s->escape( m->mountPoint() ) ) );
        return new NfsDeviceHandler( ids[0].toInt(), server, share, m->mountPoint(), udi );
    }
    else
    {
        int id = s->insert( QString( "INSERT INTO devices"
                                     "( type, servername, sharename, lastmountpoint ) "
                                     "VALUES ( 'nfs', '%1', '%2', '%3' );" )
                                     .arg( s->escape( server ) )
                                     .arg( s->escape( share ) )
                                     .arg( s->escape( m->mountPoint() ) ),
                                     "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=nfs, server=" << server << ", share=" << share;
            return 0;
        }
        debug() << "Created new NFS device with ID " << id << " , server " << server << " ,share " << share;
        return new NfsDeviceHandler( id, server, share, m->mountPoint(), udi );
    }
}

