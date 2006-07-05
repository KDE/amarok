#ifndef AMAROK_DAAPCLIENT_CPP
#define AMAROK_DAAPCLIENT_CPP

#include "daapclient.h"
#include "mediabrowser.h"

AMAROK_EXPORT_PLUGIN( DaapClient )

bool
DaapClient::isConnected()
{
    return true;
}

bool
DaapClient::getCapacity(  KIO::filesize_t* /* total */, KIO::filesize_t* /* available */ )
{
    return false;
}

bool
DaapClient::lockDevice(bool /*tryOnly = false*/ )
{
    return false;
}

void
DaapClient::unlockDevice()
{
    return;
}

bool
DaapClient::openDevice(bool /* silent=false */)
{
    return false;
}

bool
DaapClient::closeDevice()
{
    return false;
}

void
DaapClient::synchronizeDevice()
{
    return;
}

MediaItem*
DaapClient::copyTrackToDevice(const MetaBundle& /* bundle */)
{
    return 0;
}

MediaItem*
DaapClient::trackExists( const MetaBundle& )
{
    return 0;
}

int
DaapClient::deleteItemFromDevice( MediaItem *item, bool onlyPlayed )
{
    return 0;
}



#endif /* AMAROK_DAAPCLIENT_CPP */
