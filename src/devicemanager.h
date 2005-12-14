#ifndef AMAROK_DEVICE_MANAGER_H
#define AMAROK_DEVICE_MANAGER_H

#include <dcopobject.h>

class DeviceManager : public QObject
{

    Q_OBJECT
    public:
        DeviceManager();
        ~DeviceManager();
        static DeviceManager *instance();

        void displayDevices(QString name);

    private:
        DCOPClient *m_dc;
        bool m_valid;

};

#endif

