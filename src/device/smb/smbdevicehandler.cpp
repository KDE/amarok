/*
 *  Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "smbdevicehandler.h"

AMAROK_EXPORT_PLUGIN( SmbDeviceHandlerFactory )

#include "debug.h"

#include <kconfig.h>
#include <kurl.h>

#include <qvaluelist.h>


SmbDeviceHandler::SmbDeviceHandler( int deviceId, QString server, QString dir, QString mountPoint )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_server( server )
    , m_dir( dir )
{
}

SmbDeviceHandler::~SmbDeviceHandler()
{
}

bool
SmbDeviceHandler::isAvailable() const
{
    return true;
}


QString
SmbDeviceHandler::type() const
{
    return "smb";
}

int
SmbDeviceHandler::getDeviceID()
{
    return m_deviceID;
}

const QString &
SmbDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void
SmbDeviceHandler::getURL( KURL &absolutePath, const KURL &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void
SmbDeviceHandler::getPlayableURL( KURL &absolutePath, const KURL &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool
SmbDeviceHandler::deviceIsMedium( const Medium * m ) const
{
    return m->deviceNode() == m_server + ':' + m_dir;
}

///////////////////////////////////////////////////////////////////////////////
// class SmbDeviceHandlerFactory 
///////////////////////////////////////////////////////////////////////////////

QString
SmbDeviceHandlerFactory::type( ) const
{
    return "smb";
}

bool
SmbDeviceHandlerFactory::canCreateFromMedium( ) const
{
    return true;
}

bool
SmbDeviceHandlerFactory::canCreateFromConfig( ) const
{
    return false;
}

bool
SmbDeviceHandlerFactory::canHandle( const Medium * m ) const
{
    return m && ( m->fsType().find( "smb" ) != -1 ||
                  m->fsType().find( "cifs" ) != -1 ) 
		&& m->isMounted();
}

SmbDeviceHandlerFactory::SmbDeviceHandlerFactory( )
{
}

SmbDeviceHandlerFactory::~SmbDeviceHandlerFactory( )
{
}

DeviceHandler *
SmbDeviceHandlerFactory::createHandler( const KConfig* ) const
{
    return 0;
}

DeviceHandler *
SmbDeviceHandlerFactory::createHandler( const Medium * m ) const
{
    QString server = m->deviceNode().section( "/", 2, 2 );
    QString share = m->deviceNode().section( "/", 3, 3 );
    QStringList ids = CollectionDB::instance()->query( QString( "SELECT id, label, lastmountpoint "
                                                                "FROM devices WHERE type = 'smb' "
                                                                "AND servername = '%1' AND sharename = '%2';" )
                                                                .arg( server )
                                                                .arg( share ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing SMB config for ID " << ids[0] << " , server " << server << " ,share " << share << endl;
        CollectionDB::instance()->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                                                  "id = %1;" ).arg( ids[0] ).arg( m->mountPoint() ) );
        return new SmbDeviceHandler( ids[0].toInt(), server, share, m->mountPoint() );
    }
    else
    {
        int id = CollectionDB::instance()->insert( QString( "INSERT INTO devices"
                                                            "( type, servername, sharename, lastmountpoint ) "
                                                            "VALUES ( 'smb', '%1', '%2', '%3' );" )
                                                            .arg( server )
                                                            .arg( share )
                                                            .arg( m->mountPoint() ), "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=smb, server=" << server << ", share=" << share << endl;
            return 0;
        }
        debug() << "Created new SMB device with ID " << id << " , server " << server << " ,share " << share << endl;
        return new SmbDeviceHandler( id, server, share, m->mountPoint() );
    }
}

