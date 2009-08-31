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

#ifndef MASSSTORAGEDEVICEHANDLER_H
#define MASSSTORAGEDEVICEHANDLER_H

#include <MountPointManager.h>

#include <solid/device.h>

class MassStorageDeviceHandlerFactory : public DeviceHandlerFactory
{
public:
    MassStorageDeviceHandlerFactory();
    virtual ~MassStorageDeviceHandlerFactory();

    virtual bool canHandle( const Solid::Device &device ) const;

    virtual bool canCreateFromMedium() const;

    virtual DeviceHandler* createHandler( const Solid::Device &device, const QString &uuid ) const;

    virtual bool canCreateFromConfig() const;

    virtual DeviceHandler* createHandler( KSharedConfigPtr c ) const;

    virtual QString type() const;

private:
    bool excludedFilesystem( const QString &fstype ) const;
};

/**
	@author Maximilian Kossick <maximilian.kossick@googlemail.com>
*/
class MassStorageDeviceHandler : public DeviceHandler
{
public:
    MassStorageDeviceHandler();
    MassStorageDeviceHandler(int deviceId, const QString &mountPoint, const QString &uuid );

    virtual ~MassStorageDeviceHandler();

    virtual bool isAvailable() const;
    virtual QString type() const;
    virtual int getDeviceID( );
    virtual const QString &getDevicePath() const;
    virtual void getURL( KUrl &absolutePath, const KUrl &relativePath );
    virtual void getPlayableURL( KUrl &absolutePath, const KUrl &relativePath );
    virtual bool deviceMatchesUdi( const QString &udi ) const;

private:

    int m_deviceID;
    const QString m_mountPoint;
    QString m_udi;

};

#endif
