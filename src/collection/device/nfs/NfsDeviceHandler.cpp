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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "NfsDeviceHandler.h"

AMAROK_EXPORT_PLUGIN( NfsDeviceHandlerFactory )

#include "Debug.h"

#include <KConfig>
#include <KUrl>

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
NfsDeviceHandler::getURL( KUrl &absolutePath, const KUrl &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void
NfsDeviceHandler::getPlayableURL( KUrl &absolutePath, const KUrl &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool
NfsDeviceHandler::deviceIsMedium( const Medium * m ) const
{
    return m->deviceNode() == m_server + ':' + m_dir;
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
NfsDeviceHandlerFactory::createHandler( KSharedConfigPtr ) const
{
    return 0;
}

DeviceHandler *
NfsDeviceHandlerFactory::createHandler( const Medium * m ) const
{
    SqlStorage *s = CollectionManager::instance()->sqlStorage();
    QString server = m->deviceNode().section( ':', 0, 0 );
    QString share = m->deviceNode().section( ':', 1, 1 );
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
        return new NfsDeviceHandler( ids[0].toInt(), server, share, m->mountPoint() );
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
        return new NfsDeviceHandler( id, server, share, m->mountPoint() );
    }
}

