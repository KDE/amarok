#ifndef AMAROK_DAAPCLIENT_H
#define AMAROK_DAAPCLIENT_H

#include "mediabrowser.h"

class DaapClient : public MediaDevice
{
    public:
         DaapClient() { }
         virtual ~DaapClient() { }
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

};

#endif /*AMAROK_DAAPCLIENT_H*/
