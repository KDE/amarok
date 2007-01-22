/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 *                        (C) 2006 Seb Ruiz <me@sebruiz.net>               *
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
#include "threadmanager.h"

#include <kdeversion.h>
#include <kdialogbase.h>
#include <ktempfile.h>

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

class AddHostBase;
class MediaItem;
class ServerItem;
class DaapServer;

class KURL;

class QCheckBox;
class QString;
class QTimer;

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

        virtual bool needsManualConfig() { return true; }
        virtual void addConfigElements   ( QWidget * /*parent*/ );
        virtual void removeConfigElements( QWidget * /*parent*/ );
        virtual void applyConfig();
        virtual void loadConfig();


        int incRevision( const QString& host );
        int getSession( const QString& host );
        KURL getProxyUrl( const KURL& url );
        void customClicked();
        bool autoConnect() { return true; }

    public slots:
         void passwordPrompt();

    protected:
         bool getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
         void rmbPressed( QListViewItem* qitem, const QPoint& point, int );
         bool lockDevice( bool tryOnly = false );
         void unlockDevice();
         bool openDevice( bool silent=false );
         bool closeDevice();
         void synchronizeDevice();
         MediaItem* copyTrackToDevice(const MetaBundle& bundle);
         MediaItem* trackExists( const MetaBundle& );
         virtual int deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack );

   private slots:
        void serverOffline( DNSSD::RemoteService::Ptr );
        void foundDaap( DNSSD::RemoteService::Ptr );
        void resolvedDaap( bool );
        void createTree( const QString& host, Daap::SongList bundles );
        void broadcastButtonToggled();

   private:
        ServerItem* newHost( const QString& serviceName, const QString& host, const QString& ip, const Q_INT16 port );
        void downloadSongs( KURL::List urls );
        QString resolve( const QString& hostname );
#if DNSSD_SUPPORT
        QString serverKey( const DNSSD::RemoteService* service ) const;
        DNSSD::ServiceBrowser* m_browser;
#endif
        /// @return true if track is already in the local collection
        const bool trackExistsInCollection( MetaBundle *bundle );

        bool    m_connected;
        QMap<QString, ServerInfo*> m_servers;
        QMap<QString, ServerItem*> m_serverItemMap;

        DaapServer *m_sharingServer;
        QCheckBox  *m_broadcastServerCheckBox;
        QCheckBox  *m_removeDuplicatesCheckBox;
        bool        m_broadcastServer;
        bool        m_removeDuplicates;
        KToolBarButton *m_broadcastButton;

        // if set to true, we don't display items that the user already has in the collection
        bool        m_hideMusicOwned;
};

class ServerItem : public QObject, public MediaItem
{
    Q_OBJECT

    public:
        ServerItem( QListView* parent, DaapClient* client, const QString& ip, Q_UINT16 port, const QString& title, const QString& host );
        ~ServerItem();
        void setOpen( bool o );
        void resetTitle()                     { setText( 0, m_title ); }
        void unLoaded()                       { m_loaded = false; }
        void setReader( Daap::Reader* reader) { m_reader = reader; }
        Daap::Reader* getReader() const       { return m_reader; }

        void startAnimation();
        void stopAnimation();

        QString key() const { return key( m_host, m_port ); }
        void reset();
        static QString key( const QString& host, Q_UINT16 port ) { return host + ':' + QString::number( port ); }
    public slots:
        void httpError( const QString& );
    private slots:
        void slotAnimation();

    private:
        DaapClient     *m_daapClient;
        Daap::Reader   *m_reader;
        const QString   m_ip;
        const Q_UINT16  m_port;
        const QString   m_title;
        const QString   m_host;
        bool            m_loaded;

        QPixmap        *m_loading1, *m_loading2;    //icons for loading animation
        QTimer          m_animationTimer;
        uint            m_iconCounter;

        static const int ANIMATION_INTERVAL = 250;
};

class DaapDownloader : public ThreadManager::Job
{
Q_OBJECT
public:
    DaapDownloader( KURL::List urls );

    virtual bool doJob();

    virtual void completeJob();

private slots:
    void downloadFinished( int id, bool error );
    void dataReadProgress( int done, int total );
    void downloadFailed( const QString &error );

private:
    KURL::List m_urls;
    QValueList<KTempFile*> m_tempFileList;
    bool m_ready;
    bool m_successful;
    bool m_errorOccured;
};

#endif /*AMAROK_DAAPCLIENT_H*/
