/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef CONNECTIONASSISTANT_H
#define CONNECTIONASSISTANT_H

#include "mediadevicecollection_export.h"

#include <QObject>

class MediaDeviceInfo;

class QString;

/**
@class ConnectionAssistant

The ConnectionAssistant (CA) serves as a way for MediaDeviceCollectionFactory to register its
device type with the MediaDeviceMonitor (MDM). Once registered, the MDM can use the CA to
attempt to identify a newly plugged-in device, and retrieve the MediaDeviceInfo object necessary
for the Factory to connect to it.

*/

class MEDIADEVICECOLLECTION_EXPORT ConnectionAssistant : public QObject
{
    Q_OBJECT
    
public:

    virtual ~ConnectionAssistant();

    /**

    identify checks if a device identified by @param uid matches the type
    of device described by this ConnectionAssistant
    
    */
    virtual bool identify( const QString& udi );

    /**

    deviceInfo returns a pointer to a new MediaDeviceInfo of the type of device
    described by this ConnectionAssistant

    */

    virtual MediaDeviceInfo* deviceInfo( const QString& udi );

    bool wait();

    // Simply emit identified( info )
    virtual void tellIdentified( const QString &udi );
    virtual void tellDisconnected( const QString &udi );

protected:

    /*
     * Constructor
     * @param wait whether or not to wait to identify this device,
     * to give other types of devices a chance to claim this device
     * type during autodetection
     */
    ConnectionAssistant( bool wait = false );

signals:

    /**

    identified is emitted when identify returns true, and the device type's Factory
    has a slot that then attempts to connect to the device

    */
     void identified( MediaDeviceInfo* info );

     /**

     disconnected is emitted when a device with a given @param udi
     has been disconnected.  The device type's factory should then
     destroy the Collection appropriately

     */

     void disconnected( const QString &udi );

private:
     bool m_wait;

};

#endif // CONNECTIONASSISTANT_H
