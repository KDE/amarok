//
// C++ Interface: smbdevicehandler
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SMBDEVICEHANDLER_H
#define SMBDEVICEHANDLER_H

#include <mountpointmanager.h>

class SmbDeviceHandlerFactory : public DeviceHandlerFactory
{
public:
    SmbDeviceHandlerFactory();
    virtual ~SmbDeviceHandlerFactory();

    virtual bool canHandle( const Medium* m ) const;

    virtual bool canCreateFromMedium() const;

    virtual DeviceHandler* createHandler( const Medium* m ) const;

    virtual bool canCreateFromConfig() const;

    virtual DeviceHandler* createHandler( const KConfig* c ) const;

    virtual QString type() const;
};

/**
	@author Maximilian Kossick <maximilian.kossick@googlemail.com>
*/
class SmbDeviceHandler : public DeviceHandler
{
public:
    SmbDeviceHandler(int deviceId, QString server, QString dir, QString mountPoint );

    virtual ~SmbDeviceHandler();

    virtual bool isAvailable() const;
    virtual QString type() const;
    virtual int getDeviceID( );
    virtual QString getDevicePath() const;
    virtual void getURL( KURL &absolutePath, const KURL &relativePath );
    virtual void getPlayableURL( KURL &absolutePath, const KURL &relativePath );
    virtual bool deviceIsMedium( const Medium *m ) const;

private:

    int m_deviceID;
    QString m_mountPoint;
    QString m_server;
    QString m_dir;

};

#endif
