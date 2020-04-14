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
 
#ifndef NFSDEVICEHANDLER_H
#define NFSDEVICEHANDLER_H

#include "core-impl/collections/db/MountPointManager.h"

class NfsDeviceHandlerFactory : public DeviceHandlerFactory
{
public:
    explicit NfsDeviceHandlerFactory( QObject *parent ) : DeviceHandlerFactory( parent ) {}
    ~NfsDeviceHandlerFactory() override;

    bool canHandle( const Solid::Device &device ) const override;

    bool canCreateFromMedium() const override;

    DeviceHandler* createHandler( const Solid::Device &device, const QString &uuid, QSharedPointer<SqlStorage> s ) const override;

    bool canCreateFromConfig() const override;

    DeviceHandler* createHandler( const KSharedConfigPtr &c, QSharedPointer<SqlStorage> s ) const override;

    QString type() const override;
};

/**
    @author Maximilian Kossick <maximilian.kossick@googlemail.com>
*/
class NfsDeviceHandler : public DeviceHandler
{
public:
    NfsDeviceHandler();
    NfsDeviceHandler(int deviceId, const QString &mountPoint, const QString &udi );
    NfsDeviceHandler(int deviceId, const QString &server, const QString &share, const QString &mountPoint, const QString &udi );

    ~NfsDeviceHandler() override;

    bool isAvailable() const override;
    QString type() const override;
    int getDeviceID( ) override;
    const QString &getDevicePath() const override;
    void getURL( QUrl &absolutePath, const QUrl &relativePath ) override;
    void getPlayableURL( QUrl &absoluteUrl, const QUrl &relativeUrl ) override;
    bool deviceMatchesUdi( const QString &udi ) const override;

private:

    int m_deviceID;
    QString m_server;
    QString m_share;
    const QString m_mountPoint;
    QString m_udi;

};

#endif
