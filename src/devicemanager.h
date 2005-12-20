#ifndef AMAROK_DEVICE_MANAGER_H
#define AMAROK_DEVICE_MANAGER_H

#include <dcopobject.h>
#include "medium.h"

typedef Medium::List MediaList;

class DeviceManager : public QObject
{

    static const uint GENERIC = 0;
    static const uint APPLE = 1;
    static const uint IFP = 2;

    Q_OBJECT
    public:
        DeviceManager();
        ~DeviceManager();
        static DeviceManager *instance();

        void mediumAdded( QString name );
        void mediumChanged( QString name);
        void mediumRemoved( QString name);

    signals:
        void mediumAdded( Medium*, uint );
        void mediumChanged( Medium*, uint );
        void mediumRemoved( Medium*, uint );

    private:
        Medium* getDevice(QString name);
        uint deviceKind(Medium *);

        DCOPClient *m_dc;
        bool m_valid;
        MediaList m_currMediaList;

};

#endif

