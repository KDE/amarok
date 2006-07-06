#ifndef AMAROK_DAAPCLIENT_H
#define AMAROK_DAAPCLIENT_H

#include "mediabrowser.h"

#include <dnssd/remoteservice.h> //for DNSSD::RemoteService::Ptr

namespace DNSSD {
    class ServiceBrowser;
}

class DaapClient : public MediaDevice
{

    Q_OBJECT

    public:
         DaapClient();
         virtual ~DaapClient();
         bool isConnected();
    protected:
         bool getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
         bool lockDevice( bool tryOnly = false );
         void unlockDevice();
         bool openDevice( bool silent=false );
         bool closeDevice();
         void synchronizeDevice();
         MediaItem* copyTrackToDevice(const MetaBundle& bundle);
         MediaItem* trackExists( const MetaBundle& );
         int    deleteItemFromDevice( MediaItem *item, bool onlyPlayed = false );
   private slots:
         void foundDaap( DNSSD::RemoteService::Ptr );
         void resolvedDaap( bool );
   private:
        DNSSD::ServiceBrowser* m_browser;
        bool    m_connected;

};

#endif /*AMAROK_DAAPCLIENT_H*/
