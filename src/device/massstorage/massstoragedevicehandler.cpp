//
// C++ Implementation: massstoragedevicehandler
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#define DEBUG_PREFIX "MassStorageDeviceHandler"

#include "massstoragedevicehandler.h"

AMAROK_EXPORT_PLUGIN( MassStorageDeviceHandlerFactory )

#include "collectiondb.h"
#include "debug.h"

#include <kconfig.h>
#include <kurl.h>

#include <qvaluelist.h>

MassStorageDeviceHandler::MassStorageDeviceHandler(): DeviceHandler()
{
}

MassStorageDeviceHandler::MassStorageDeviceHandler( int deviceId, QString mountPoint, QString uuid )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_uuid( uuid )
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

void MassStorageDeviceHandler::getURL( KURL &absolutePath, const KURL &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void MassStorageDeviceHandler::getPlayableURL( KURL &absolutePath, const KURL &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool MassStorageDeviceHandler::deviceIsMedium( const Medium * m ) const
{
    return m_uuid == m->id();
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

bool MassStorageDeviceHandlerFactory::canHandle( const Medium * m ) const
{
    return m && !m->id().isEmpty() && !excludedFilesystem( m->fsType() );
}

MassStorageDeviceHandlerFactory::MassStorageDeviceHandlerFactory( )
{
}

MassStorageDeviceHandlerFactory::~MassStorageDeviceHandlerFactory( )
{
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const KConfig* ) const
{
    return 0;
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const Medium * m ) const
{
    QStringList ids = CollectionDB::instance()->query( QString( "SELECT id, label, lastmountpoint "
                                                               "FROM devices WHERE type = 'uuid' "
                                                               "AND uuid = '%1';" ).arg( m->id() ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing UUID config for ID " << ids[0] << " , uuid " << m->id() << endl;
        CollectionDB::instance()->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                                                  "id = %1;" ).arg( ids[0] ).arg( m->mountPoint() ) );
        return new MassStorageDeviceHandler( ids[0].toInt(), m->mountPoint(), m->id() );
    }
    else
    {
        int id = CollectionDB::instance()->insert( QString( "INSERT INTO devices( type, uuid, lastmountpoint ) "
                                                            "VALUES ( 'uuid', '%1', '%2' );" )
                                                            .arg( m->id() )
                                                            .arg( m->mountPoint() ), "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=uuid, uuid=" << m->id() << endl;
            return 0;
        }
        debug() << "Created new UUID device with ID " << id << " , uuid " << m->id() << endl;
        return new MassStorageDeviceHandler( id, m->mountPoint(), m->id() );
    }
}

bool
MassStorageDeviceHandlerFactory::excludedFilesystem( QString fstype ) const
{
    return fstype.isEmpty() ||
           fstype.find( "smb" ) != -1 ||
           fstype.find( "cifs" ) != -1 ||
           fstype.find( "nfs" ) != -1 ||
           fstype == "udf"  ||
           fstype == "iso9660" ;
}
