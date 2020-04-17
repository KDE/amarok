/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

/*

Description:

The MediaDeviceMonitor connects to the MediaDeviceCache, monitoring the connection and disconnection of devices.  It tests
for devices known to Amarok, and if it finds them, sends a signal that the appropriate CollectionFactory is connected to,
which triggers the creation of the associated Collection.  Similar behaviour for when a device is disconnected.

All new MediaDeviceCollection-type classes must register their ConnectionAssistant of their device with this class, and have
it connect to the right signals to properly build/delete the associated Collection.  An example of this is seen in the
IpodCollectionFactory.

*/

#ifndef AMAROK_MEDIADEVICEMONITOR_H
#define AMAROK_MEDIADEVICEMONITOR_H

#include "amarok_export.h"

#include <QHash>
#include <QList>
#include <QObject>

class ConnectionAssistant;
class MediaDeviceInfo;

class QStringList;

class AMAROK_EXPORT MediaDeviceMonitor : public QObject
{
    Q_OBJECT

    public:

    static MediaDeviceMonitor* instance() { return s_instance ? s_instance : new MediaDeviceMonitor(); }

    MediaDeviceMonitor();
    ~MediaDeviceMonitor() override;

    void init(); // connect to MediaDeviceCache

    QStringList getDevices(); // get list of devices

    /// Get assistant for a given udi
    ConnectionAssistant *getUdiAssistant( const QString &udi )
    {
        return m_udiAssistants[ udi ];
    }

    /**

    registerDeviceType adds the device type described by @param assistant to the list
    of known device types by the MDM, and then checks the list of known devices
    for a match with this type

    */
    void registerDeviceType( ConnectionAssistant *assistant );

    public Q_SLOTS:

    /**

    checkDevice checks if @p udi is a known device
    and if so attempts to connect it

    checkOneDevice runs an identify check using the given
    assistant and udi

    checkDevicesFor checks if the device type described
    by @p assistant matches any of the udi's in the
    MediaDeviceCache, and if so, attempts to connect to
    it

    */

    void checkDevice( const QString &udi );
    void checkOneDevice( ConnectionAssistant* assistant, const QString& udi );
    void checkDevicesFor( ConnectionAssistant* assistant );


    Q_SIGNALS:
        void deviceDetected( const MediaDeviceInfo &deviceinfo );
        void deviceRemoved( const QString &udi );


    private Q_SLOTS:

        void deviceAdded( const QString &udi );
        void slotDeviceRemoved( const QString &udi );
        void slotAccessibilityChanged( bool accessible, const QString & udi );

        void slotDequeueWaitingAssistant();


    private:
        static MediaDeviceMonitor *s_instance;

        // keeps track of which CA to contact for which udi
        QHash<QString,ConnectionAssistant*> m_udiAssistants;
        // holds all registered assistants
        QList<ConnectionAssistant*> m_assistants;
        // holds all waiting assistants
        QList<ConnectionAssistant*> m_waitingassistants;
        // holds index of next waiting assistant to check
        // devices with, during initialization of device
        // factories
        int m_nextassistant;
};

#endif /* AMAROK_MEDIADEVICEMONITOR_H */

