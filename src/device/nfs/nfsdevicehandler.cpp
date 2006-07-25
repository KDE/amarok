//
// C++ Implementation: nfsdevicehandler
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "nfsdevicehandler.h"

AMAROK_EXPORT_PLUGIN( NfsDeviceHandlerFactory )

#include "debug.h"

#include <kconfig.h>
#include <kurl.h>

#include <qvaluelist.h>


NfsDeviceHandler::NfsDeviceHandler( int deviceId, QString server, QString dir, QString mountPoint )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_server( server )
    , m_dir( dir )
{
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

const QString &
NfsDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void
NfsDeviceHandler::getURL( KURL &absolutePath, const KURL &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void
NfsDeviceHandler::getPlayableURL( KURL &absolutePath, const KURL &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool
NfsDeviceHandler::deviceIsMedium( const Medium * m ) const
{
    return m->deviceNode() == m_server + ":" + m_dir;
}

///////////////////////////////////////////////////////////////////////////////
// class NfsDeviceHandlerFactory 
///////////////////////////////////////////////////////////////////////////////

QString
NfsDeviceHandlerFactory::type( ) const
{
    return "nfs";
}

bool
NfsDeviceHandlerFactory::canCreateFromMedium( ) const
{
    return true;
}

bool
NfsDeviceHandlerFactory::canCreateFromConfig( ) const
{
    return false;
}

bool
NfsDeviceHandlerFactory::canHandle( const Medium * m ) const
{
    return m && m->fsType() == "nfs" && m->isMounted();
}

NfsDeviceHandlerFactory::NfsDeviceHandlerFactory( )
{
}

NfsDeviceHandlerFactory::~NfsDeviceHandlerFactory( )
{
}

DeviceHandler *
NfsDeviceHandlerFactory::createHandler( const KConfig* ) const
{
    return 0;
}

DeviceHandler *
NfsDeviceHandlerFactory::createHandler( const Medium * m ) const
{
    QString server = m->deviceNode().section( ":", 0, 0 );
    QString share = m->deviceNode().section( ":", 1, 1 );
    QStringList ids = CollectionDB::instance()->query( QString( "SELECT id, label, lastmountpoint "
                                                                "FROM devices WHERE type = 'nfs' "
                                                                "AND servername = '%1' AND sharename = '%2';" )
                                                                .arg( server )
                                                                .arg( share ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing NFS config for ID " << ids[0] << " , server " << server << " ,share " << share << endl;
        CollectionDB::instance()->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                                                  "id = %1;" ).arg( ids[0] ).arg( m->mountPoint() ) );
        return new NfsDeviceHandler( ids[0].toInt(), server, share, m->mountPoint() );
    }
    else
    {
        int id = CollectionDB::instance()->insert( QString( "INSERT INTO devices"
                                                            "( type, servername, sharename, lastmountpoint ) "
                                                            "VALUES ( 'nfs', '%1', '%2', '%3' );" )
                                                            .arg( server )
                                                            .arg( share )
                                                            .arg( m->mountPoint() ), "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=nfs, server=" << server << ", share=" << share << endl;
            return 0;
        }
        debug() << "Created new NFS device with ID " << id << " , server " << server << " ,share " << share << endl;
        return new NfsDeviceHandler( id, server, share, m->mountPoint() );
    }
}

