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
class DeviceManager : public QObject
{

    //static const uint GENERIC = 0;
    //static const uint APPLE = 1;
    //static const uint IFP = 2;

    Q_OBJECT
    public:
        DeviceManager();
        ~DeviceManager();
        static DeviceManager *instance();

        void mediumAdded( QString name );
        void mediumChanged( QString name);
        void mediumRemoved( QString name);

        MediumMap getMediumMap() { return m_mediumMap; }
        Medium* getDevice( QString name );

        bool isValid() { return m_valid; }

        //only use getDeviceList to initialise clients
        Medium::List getDeviceList();

        //public so can be called from DCOP...but don't use this, see the
        //warning about getDeviceList()
        QStringList getDeviceStringList();

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

