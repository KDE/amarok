/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef AMAROK_DAAPCLIENT_H
#define AMAROK_DAAPCLIENT_H

#include "daapreader/reader.h"
#include "mediabrowser.h"
#include <kdeversion.h>

#define DNSSD_SUPPORT KDE_IS_VERSION(3,4,0)

#if DNSSD_SUPPORT
    #include <dnssd/remoteservice.h> //for DNSSD::RemoteService::Ptr
#else
namespace DNSSD {
    namespace RemoteService {
        class Ptr {}; //HACK Dummy class, so that daapclient.moc compiles
    }
}
#endif

namespace DNSSD {
    class ServiceBrowser;
}

class QString;
class MediaItem;

class DaapClient : public MediaDevice
{

    Q_OBJECT
   public:
        struct ServerInfo
        {
            ServerInfo() : sessionId( -1 ), revisionID( 10 ) { }
            int sessionId;
            int revisionID;
        };

        DaapClient();
        virtual ~DaapClient();
        bool isConnected();

        int incRevision( const QString& host );
        int getSession( const QString& host );
        KURL getProxyUrl( const KURL& url );

    protected:
         bool getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
         bool lockDevice( bool tryOnly = false );
         void unlockDevice();
         bool openDevice( bool silent=false );
         bool closeDevice();
         void synchronizeDevice();
         MediaItem* copyTrackToDevice(const MetaBundle& bundle);
         MediaItem* trackExists( const MetaBundle& );
         virtual int deleteItemFromDevice( MediaItem *item, bool onlyPlayed = false, bool deleteTrack = true );

   private slots:
        void foundDaap( DNSSD::RemoteService::Ptr );
        void resolvedDaap( bool );
        void createTree( const QString& host, Daap::SongList bundles );

   private:
#if DNSSD_SUPPORT
        DNSSD::ServiceBrowser* m_browser;
#endif
        bool    m_connected;
        QMap<QString, ServerInfo*> m_servers;
};

#endif /*AMAROK_DAAPCLIENT_H*/
