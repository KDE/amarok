//
// C++ Interface: mediadevicemanager
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


#ifndef AMAROK_MEDIA_DEVICE_MANAGER_H
#define AMAROK_MEDIA_DEVICE_MANAGER_H

#include "medium.h"

#include <qmap.h>

#include <dcopobject.h>

typedef QMap<QString, Medium*> MediumMap;

class MediaDeviceManager : public QObject
{

    //static const uint GENERIC = 0;
    //static const uint APPLE = 1;
    //static const uint IFP = 2;

    Q_OBJECT
    public:
        MediaDeviceManager();
        ~MediaDeviceManager();
        static MediaDeviceManager *instance();

        Medium* getDevice( QString name );
        MediumMap getMediumMap() { return m_mediumMap; }

        void addManualDevice( Medium* added );
        void removeManualDevice( Medium* removed );


    signals:
        void mediumAdded( const Medium*, QString );
        void mediumChanged( const Medium*, QString );
        void mediumRemoved( const Medium*, QString );

    public slots:
        void slotMediumAdded( const Medium*, QString );
        void slotMediumChanged( const Medium*, QString );
        void slotMediumRemoved( const Medium*, QString );

    private slots:
        void reinitDevices();

    private:

        MediumMap m_mediumMap;

};

#endif

