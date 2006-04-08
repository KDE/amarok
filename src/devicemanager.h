//
// C++ Interface: devicemanager
//
// Description: Controls device/medium object handling, providing
//              helper functions for other objects
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
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

        Medium* getDevice( QString name );
        MediumMap getMediumMap() { return m_mediumMap; }

        void addManualDevice( Medium* added );
        void removeManualDevice( Medium* removed );

        bool isValid() { return m_valid; }

        //public so can be called from DCOP...but don't use this, see the
        //warning about getDeviceList()
        QStringList getDeviceStringList( bool auto_only = false );

    signals:
        void mediumAdded( const Medium*, QString );
        void mediumChanged( const Medium*, QString );
        void mediumRemoved( const Medium*, QString );

    private:
        //don't make getDeviceList public.  Use getMediumMap()...it pre-filters and keeps things in sync
        Medium::List getDeviceList( bool auto_only = false );

        DCOPClient *m_dc;
        bool m_valid;
        MediumMap m_mediumMap;
};

#endif

