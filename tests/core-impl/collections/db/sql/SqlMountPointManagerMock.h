/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef SQLMOUNTPOINTMANAGERMOCK_H
#define SQLMOUNTPOINTMANAGERMOCK_H

#include "core-impl/collections/db/MountPointManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"

#include <QMap>
#include <QString>

//Note: this class will probably break horribly on win32

class SqlMountPointManagerMock : public MountPointManager
{
public:

    SqlMountPointManagerMock( QObject *parent, QSharedPointer<SqlStorage> storage )
        : MountPointManager( parent, storage )
    {
    }

    int getIdForUrl( const QUrl &url ) override
    {
        QString path = url.path();
        foreach( int id, m_mountPoints.keys() )
        {
            if( path.startsWith( m_mountPoints.value( id ) ) )
            {
                return id;
            }
        }

        return -1;
    }

    QString getAbsolutePath ( const int deviceId, const QString& relativePath ) const override
    {
        if( deviceId == -1 )
            return relativePath.right( relativePath.length() -1 );
        else
        {
            return m_mountPoints.value( deviceId ) + relativePath.right( relativePath.length() -1 );
        }
    }

    QString getRelativePath( const int deviceId, const QString& absolutePath ) const override
    {
        if( deviceId == -1 )
            return '.' + absolutePath;
        else
        {
            QString mp = m_mountPoints.value( deviceId );
            return '.' + absolutePath.right( mp.length() );
        }
    }

    IdList getMountedDeviceIds() const override
    {
        IdList result;
        result << -1;
        result << m_mountPoints.keys();
        return result;
    }

    QStringList collectionFolders() const override
    {
        return m_folders;
    }

    void setCollectionFolders( const QStringList &folders ) override
    {
        m_folders = folders;
    }

    void emitDeviceAdded( int id )
    {
        Q_EMIT deviceAdded( id );
    }

    void emitDeviceRemoved( int id )
    {
        Q_EMIT deviceRemoved( id );
    }

    QMap<int,QString> m_mountPoints;
    QStringList m_folders;
};

#endif // SQLMOUNTPOINTMANAGERMOCK_H
