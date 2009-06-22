/* 
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

/*

Description:

The MediaDeviceMonitor connects to the MediaDeviceCache, monitoring the connection and disconnection of devices.  It tests
for devices known to Amarok, and if it finds them, sends a signal that the appropriate CollectionFactory is connected to,
which triggers the creation of the associated Collection.  Similar behaviour for when a device is disconnected.

All new MediaDeviceCollection-type classes must add the detection of their device to this class, and have their CollectionFactory
connect to the right signals to properly build/delete the associated Collection.  An example of this is seen in the
IpodCollectionFactory.

*/

#ifndef AMAROK_MEDIADEVICEMONITOR_H
#define AMAROK_MEDIADEVICEMONITOR_H

#include "collection/mediadevicecollection/support/MediaDeviceInfo.h"

#include "amarok_export.h"

#include <QHash>
#include <QList>
#include <QObject>

class ConnectionAssistant;

class QStringList;

class AMAROK_EXPORT MediaDeviceMonitor : public QObject
{
    Q_OBJECT

    public:

    static MediaDeviceMonitor* instance() { return s_instance ? s_instance : new MediaDeviceMonitor(); }

    MediaDeviceMonitor();
    ~MediaDeviceMonitor();

    void init(); // connect to MediaDeviceCache


    QStringList getDevices(); // get list of devices
    void checkDevices(); // scans for supported devices

    void checkDevicesFor( ConnectionAssistant* assistant );
    void checkDevicesForMtp();
    void checkDevicesForIpod();
    void checkDevicesForCd();

    QString isCdPresent();
    void ejectCd( const QString &udi );

    QString currentCdId();
    void setCurrentCdId( const QString &id );

    /**

    registerDeviceType adds the device type described by @param assistant to the list
    of known device types by the MDM, and then checks the list of known devices
    for a match with this type

    */
    void registerDeviceType( ConnectionAssistant *assistant );

 //   void fetchDevices(); // emits device info for each device present

    signals:
        void deviceRemoved( const QString &udi );
        void ipodDetected( const QString &mountPoint, const QString &udi );
        void mtpDetected( const QString &serial, const QString &udi );
        void audioCdDetected( const QString &udi );

        void ipodReadyToConnect( const QString &mountpoint, const QString &udi );
        void ipodReadyToDisconnect( const QString &udi );
        void mtpReadyToConnect( const QString &serial, const QString &udi );
        void mtpReadyToDisconnect( const QString &udi );

        void deviceDetected( const MediaDeviceInfo &deviceinfo );

    public slots:

        void connectIpod( const QString &mountpoint, const QString &udi );
        void disconnectIpod( const QString &udi );
        void connectMtp( const QString &serial, const QString &udi );
        void disconnectMtp( const QString &udi );


    private slots:


        void deviceAdded( const QString &udi );
        void slotDeviceRemoved( const QString &udi );
        void slotAccessibilityChanged( bool accessible, const QString & udi );


    private:

        bool isIpod( const QString &udi );
        bool isMtp( const QString &udi );
        bool isAudioCd( const QString &udi );

        QString m_currentCdId;

        

        static MediaDeviceMonitor* s_instance;

        // keeps track of which CA to contact for which udi
        QHash<QString,ConnectionAssistant*> m_udiAssistants;
        // holds all registered assistants
        QList<ConnectionAssistant*> m_assistants;


};

#endif /* AMAROK_MEDIADEVICEMONITOR_H */

