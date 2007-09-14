// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <ruiz@kde.org>  
// (c) 2006 T.R.Shashwath <trshash84@gmail.com>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include "amarok_export.h"
#include "amarok.h"
#include "browserToolBar.h"
#include "meta/meta.h"
#include "pluginmanager.h"
#include "plugin/plugin.h"   //baseclass
#include "scrobbler.h"       //SubmitItem

#include <k3listview.h>       //baseclass
#include <KHBox>
#include <kio/global.h>      //filesize_t
#include <KUrl>            //stack allocated
#include <KVBox>           //baseclass

#include <Q3PtrList>
#include <QDateTime>
#include <QLabel>
#include <QMutex>
#include <QPixmap>
#include <QWidget>


class MediaBrowser;
class MediaDevice;
class MediaItem;
class MediaView;
class SpaceLabel;
class TransferDialog;
class SearchWidget;

class KAction;
class KComboBox;
class KPushButton;
class K3ShellProcess;

class QDropEvent;
class QKeyEvent;
class QPaintEvent;
class Q3DragObject;
class QLabel;
class QProgressBar;

class MediaBrowser : public QWidget
{
    Q_OBJECT
    friend class DeviceConfigureDialog;
    friend class MediaDevice;

    public:
        static bool isAvailable();
        //AMAROK_EXPORT static MediaBrowser *instance() { return s_instance; }

        MediaBrowser( const char *name );
        virtual ~MediaBrowser();
        bool blockQuit() const;
        MediaDevice *currentDevice() const { return m_currentDevice; }
        MediaDevice *deviceFromId( const QString &id ) const;
        QStringList deviceNames() const;
        bool deviceSwitch( const QString &name );

        QString getInternalPluginName ( const QString string ) { return m_pluginName[string]; }
        QString getDisplayPluginName ( const QString string ) { return m_pluginAmarokName[string]; }
        const KService::List &getPlugins() { return m_plugins; }
        void transcodingFinished( const QString &src, const QString &dst );
        bool isTranscoding() const { return m_waitForTranscode; }
        void updateStats();
        void updateButtons();
        void updateDevices();
        // return bundle for url if it is known to MediaBrowser
        //bool getBundle( const KUrl &url, MetaBundle *bundle ) const;
        bool isQuitting() const { return m_quitting; }

        KUrl getProxyUrl( const KUrl& daapUrl ) const;
        KToolBar* getToolBar() const { return m_toolbar; }
        KAction *connectAction() const { return m_connectAction; }
        KAction *disconnectAction() const { return m_disconnectAction; }
        KAction *transferAction() const { return m_transferAction; }
        KAction *configAction() const { return m_configAction; }
        KAction *customAction() const { return m_customAction; }

    signals:
        void availabilityChanged( bool isAvailable );

    protected slots:
        void transferClicked();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();
        void slotSetFilter( const QString &filter );
        void slotEditFilter();
        void deviceAdded( const QString &udi );
        void deviceRemoved( const QString &udi );
        void activateDevice( const MediaDevice *device );
        void activateDevice( int index, bool skipDummy = true );
        void cancelClicked();
        void connectClicked();
        void disconnectClicked();
        void customClicked();
        bool config(); // false if canceled by user
        KUrl transcode( const KUrl &src, const QString &filetype );
        //void tagsChanged( const MetaBundle &bundle );
        void prepareToQuit();

    private:
        MediaDevice *loadDevicePlugin( const QString &udi );
        void         unloadDevicePlugin( MediaDevice *device );

        QTimer *m_timer;
        AMAROK_EXPORT static MediaBrowser *s_instance;

        QList<MediaDevice *> m_devices;
        MediaDevice * m_currentDevice;

        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
        void addDevice( MediaDevice *device );
        void removeDevice( MediaDevice *device );

        bool m_waitForTranscode;
        KUrl m_transcodedUrl;
        QString m_transcodeSrc;

        SpaceLabel*      m_stats;
        KHBox*           m_progressBox;
        QProgressBar*       m_progress;
        QWidget*           m_views;
        KPushButton*     m_cancelButton;
        //KPushButton*     m_playlistButton;
        KVBox*           m_configBox;
        KComboBox*       m_configPluginCombo;
        KComboBox*       m_deviceCombo;
        Browser::ToolBar*m_toolbar;
        //typedef QMap<QString, MediaItem*> ItemMap;
        mutable QMutex   m_itemMapMutex;
        //ItemMap          m_itemMap;
        KService::List m_plugins;
        bool             m_haveDevices;
        bool             m_quitting;
        KAction *m_connectAction;
        KAction *m_disconnectAction;
        KAction *m_customAction;
        KAction *m_configAction;
        KAction *m_transferAction;
        SearchWidget *m_searchWidget;
};

/* at least the pure virtual functions have to be implemented by a media device,
   all items are stored in a hierarchy of MediaItems,
   when items are manipulated the MediaItems have to be updated accordingly */

class AMAROK_EXPORT MediaDevice : public QObject, public Amarok::Plugin
{
    Q_OBJECT
    friend class DeviceConfigureDialog;
    friend class TransferDialog;
    friend class MediaBrowser;

    public:
        enum Flags
        {
            None = 0,
            OnlyPlayed = 1,
            DeleteTrack = 2,
            Recursing = 4
        };

        MediaDevice();
        virtual void init( MediaBrowser* parent );
        virtual ~MediaDevice();

        /**
         * @return a KAction that will be plugged into the media device browser toolbar
         */
        virtual KAction *customAction() { return 0; }

        /**
         * @return list of filetypes playable on this device
         *  (empty list is interpreted as all types are good)
         */
        virtual QStringList supportedFiletypes() { return QStringList(); }

        /**
         * @param bundle describes track that should be checked
         * @return true if the device is capable of playing the track referred to by bundle
         */
        virtual bool isPlayable( const Meta::TrackPtr track );

        /**
         * @param bundle describes track that should be checked
         * @return true if the track is in the preferred (first in list) format of the device
         */
        virtual bool isPreferredFormat( const Meta::TrackPtr track );

        /**
         * @return true if the device is connected
         */
        virtual bool       isConnected() = 0;

        virtual void addConfigElements( QWidget * /*parent*/ ) {}
        virtual void removeConfigElements( QWidget * /*parent*/ ) {}
        virtual void applyConfig() {}
        virtual void loadConfig();

        QString configString( const QString &name, const QString &defValue = QString() );
        void setConfigString( const QString &name, const QString &value );
        bool configBool( const QString &name, bool defValue=false );
        void setConfigBool( const QString &name, bool value );

        void         setRequireMount( const bool b ) { m_requireMount = b; }
        bool         hasMountPoint() { return m_hasMountPoint; }
        void         setDeviceType( const QString &type ) { m_type = type; }
        QString      deviceType() { return m_type; }
        virtual bool autoConnect() { return false; }
        virtual bool asynchronousTransfer() { return false; }
        bool         isTransferring() { return m_transferring; }
        bool         isDeleting() { return m_deleting; }
        bool         isCanceled() { return m_canceled; }
        void         setCanceled( const bool b ) { m_canceled = b; }

        int          progress() const;
        void         setProgress( const int progress, const int total = -1 /* leave total unchanged by default */ );
        void         hideProgress();


        QString           getTransferDir() { return m_transferDir; }

    public slots:
        void abortTransfer();
        bool connectDevice( bool silent=false );
        bool disconnectDevice( bool postdisconnecthook=true );
        void scheduleDisconnect() { m_scheduledDisconnect = true; }

    protected slots:
        void fileTransferred( KIO::Job *job );
        void fileTransferFinished();

    private:
        int  sysCall(const QString & command);
        int  runPreConnectCommand();
        int  runPostDisconnectCommand();
        QString replaceVariables( const QString &cmd ); // replace %m with mount point and %d with device node

        QString uid() { return m_uid; }
        void setUid( const QString &uid ) { m_uid = uid; }

        /**
         * Find a particular track
         * @param bundle The metabundle of the requested media item
         * @return The MediaItem of the item if found, otherwise NULL
         * @note This may not be worth implementing for non database driven devices, as it could be slow
         */
        //virtual MediaItem *trackExists( const MetaBundle& bundle ) = 0;

    protected:
        /**
         * Get the capacity and freespace available on the device, in bytes
         * @return true if successful
         */
        virtual bool getCapacity( KIO::filesize_t *total, KIO::filesize_t *available ) { Q_UNUSED(total); Q_UNUSED(available); return false; }

        /**
         * Lock device for exclusive access if possible
         */
        virtual bool lockDevice( bool tryOnly = false ) = 0;

        /**
         * Unlock device
         */
        virtual void unlockDevice() = 0;

        /**
         * Connect to device, and populate m_view with MediaItems
         * @return true if successful
         */
        virtual bool openDevice( bool silent=false ) = 0;

        /**
         * Wrap up any loose ends and close the device
         * @return true if successful
         */
        virtual bool closeDevice() = 0;

        /**
         * Write any pending changes to the device, such as database changes
         */
        virtual void synchronizeDevice() = 0;

        /**
         * Recursively remove MediaItem from the tracklist and the device
         * @param item MediaItem to remove
         * @param onlyPlayed True if item should be deleted only if it has been played
         * @return -1 on failure, number of files deleted otherwise
         */
        //virtual int deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack ) = 0;

        /**
         * Abort the currently active track transfer
         */
        virtual void cancelTransfer() { /* often checking m_cancel is enough */ }

        virtual void updateRootItems();

        //virtual bool isSpecialItem( MediaItem *item );

        //int deleteFromDevice( MediaItem *item=0, int flags=DeleteTrack );

        bool kioCopyTrack( const KUrl &src, const KUrl &dst );

        QString     m_name;

        bool        m_hasMountPoint;

        QString     m_preconnectcmd;
        QString     m_postdisconnectcmd;
        bool        m_autoDeletePodcasts;
        bool        m_syncStats;

        bool        m_transcode;
        bool        m_transcodeAlways;
        bool        m_transcodeRemove;

        K3ShellProcess   *sysProc;
        MediaBrowser    *m_parent;
        QString          m_transferDir;
        QString          m_firstSort;
        QString          m_secondSort;
        QString          m_thirdSort;
        QString          m_uid;
        bool             m_wait;
        bool             m_waitForDeletion;
        bool             m_copyFailed;
        bool             m_requireMount;
        bool             m_canceled;
        bool             m_transferring;
        bool             m_deleting;
        bool             m_deferredDisconnect;
        bool             m_scheduledDisconnect;
        bool             m_runDisconnectHook;
        bool             m_spacesToUnderscores;
        bool             m_transfer;
        bool             m_configure;
        bool             m_customButton;

        QString          m_type;

};


#endif /* AMAROK_MEDIABROWSER_H */
