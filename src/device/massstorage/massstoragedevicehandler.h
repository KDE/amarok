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
#ifndef MASSSTORAGEDEVICEHANDLER_H
#define MASSSTORAGEDEVICEHANDLER_H

#include <mountpointmanager.h>

class MassStorageDeviceHandlerFactory : public DeviceHandlerFactory
{
public:
    MassStorageDeviceHandlerFactory();
    virtual ~MassStorageDeviceHandlerFactory();

    virtual bool canHandle( const Medium* m ) const;

    virtual bool canCreateFromMedium() const;

    virtual DeviceHandler* createHandler( const Medium* m ) const;

    virtual bool canCreateFromConfig() const;

    virtual DeviceHandler* createHandler( const KConfig* c ) const;

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
    virtual void getURL( KURL &absolutePath, const KURL &relativePath );
    virtual void getPlayableURL( KURL &absolutePath, const KURL &relativePath );
    virtual bool deviceIsMedium( const Medium *m ) const;

private:

    int m_deviceID;
    const QString m_mountPoint;
    QString m_uuid;

};

#endif
