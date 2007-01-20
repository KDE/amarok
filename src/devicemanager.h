//
// C++ Interface: devicemanager
//
// Description: Controls device/medium object handling, providing
//              helper functions for other objects
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//         Maximilian Kossick <maximilian.kossick@googlemail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef AMAROK_DEVICE_MANAGER_H
#define AMAROK_DEVICE_MANAGER_H

#include "medium.h"

#include <qmap.h>

#include <dcopobject.h>

typedef QMap<QString, Medium*> MediumMap;


//this class provides support for MountPointManager and MediaDeviceManager
//the latter is responsible for handling mediadevices (e.g. ipod)
//unless you have special requirements you should use either MountPointManager or
//MediaDeviceManager instead of this class.
class DeviceManager : public QObject
{

    Q_OBJECT
    public:
        DeviceManager();
        ~DeviceManager();
        static DeviceManager *instance();

        void mediumAdded( const QString name );
        void mediumChanged( const QString name);
        void mediumRemoved( const QString name);

        MediumMap getMediumMap() { return m_mediumMap; }
        Medium* getDevice( const QString name );
        // reconciles m_mediumMap to whatever kded has in it.
        void reconcileMediumMap();

        bool isValid() { return m_valid; }

        //only use getDeviceList to initialise clients
        Medium::List getDeviceList();

        //public so can be called from DCOP...but don't use this, see the
        //warning about getDeviceList()
        QStringList getDeviceStringList();

        // Converts a media://media/hdc URL as provided by the KDE media
        // manager on CD insert to /dev/hdc so amarok can play it.
        // This method is safe to call with a device path, it returns it
        // unchanged.
        QString convertMediaUrlToDevice( QString url );

    signals:
        void mediumAdded( const Medium*, QString );
        void mediumChanged( const Medium*, QString );
        void mediumRemoved( const Medium*, QString );

    private:

        DCOPClient *m_dc;
        bool m_valid;
        MediumMap m_mediumMap;

};

#endif

