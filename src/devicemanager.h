#ifndef AMAROK_DEVICE_MANAGER_H
#define AMAROK_DEVICE_MANAGER_H

#include <dcopobject.h>
#include <qmap.h>

#include "medium.h"

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

        MediumMap getMediumMap( ) { return m_mediumMap; }

        bool isValid( ) { return m_valid; }

    signals:
        void mediumAdded( const Medium*, QString );
        void mediumChanged( const Medium*, QString );
        void mediumRemoved( const Medium*, QString );

    private:
        Medium* getDevice(QString name);

        //don't make getDeviceList public.  Use getMediumMap()...it pre-filters and keeps things in sync
        Medium::List getDeviceList();

        DCOPClient *m_dc;
        bool m_valid;
        MediumMap m_mediumMap;
};

#endif

