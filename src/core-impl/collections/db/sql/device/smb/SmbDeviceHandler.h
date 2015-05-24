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
 
#ifndef SMBDEVICEHANDLER_H
#define SMBDEVICEHANDLER_H

#include "core-impl/collections/db/MountPointManager.h"

class SmbDeviceHandlerFactory : public DeviceHandlerFactory
{
public:
    SmbDeviceHandlerFactory( QObject *parent ) : DeviceHandlerFactory( parent ) {}
    virtual ~SmbDeviceHandlerFactory();

    virtual bool canHandle( const Solid::Device &device ) const;

    virtual bool canCreateFromMedium() const;

    virtual DeviceHandler* createHandler( const Solid::Device &device, const QString &uuid, SqlStorage *s ) const;

    virtual bool canCreateFromConfig() const;

    virtual DeviceHandler* createHandler( KSharedConfigPtr c, SqlStorage *s ) const;

    virtual QString type() const;
};

/**
    @author Maximilian Kossick <maximilian.kossick@googlemail.com>
*/
class SmbDeviceHandler : public DeviceHandler
{
public:
    SmbDeviceHandler();
    SmbDeviceHandler(int deviceId, const QString &mountPoint, const QString &udi );
    SmbDeviceHandler(int deviceId, const QString &server, const QString &share, const QString &mountPoint, const QString &udi );

    virtual ~SmbDeviceHandler();

    virtual bool isAvailable() const;
    virtual QString type() const;
    virtual int getDeviceID( );
    virtual const QString &getDevicePath() const;
    virtual void getURL( QUrl &absolutePath, const QUrl &relativePath );
    virtual void getPlayableURL( QUrl &absolutePath, const QUrl &relativePath );
    virtual bool deviceMatchesUdi( const QString &udi ) const;

private:

    int m_deviceID;
    QString m_server;
    QString m_share;
    const QString m_mountPoint;
    QString m_udi;

};

#endif
