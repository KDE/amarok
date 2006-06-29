// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include "amarok.h"
#include "amarok_export.h"
#include "medium.h"
#include "plugin/plugin.h"   //baseclass
#include "pluginmanager.h"

#include <qvbox.h>           //baseclass
#include <qdatetime.h>

#include <klistview.h>       //baseclass
#include <kurl.h>            //stack allocated
#include <kio/global.h>      //filesize_t
#include "scrobbler.h"       //SubmitItem

class MediaBrowser;
class MediaDevice;
class MediaView;
class MetaBundle;
class SpaceLabel;
class TransferDialog;

class KComboBox;
class KDialogBase;
class KProgress;
class KPushButton;
class KShellProcess;

class QLabel;
class QPalette;
class MediaItemTip;

namespace Browser
{
    class ToolBar;
}

class LIBAMAROK_EXPORT MediaItem : public KListViewItem
{
    public:
        MediaItem( QListView* parent );
        MediaItem( QListViewItem* parent );
        MediaItem( QListView* parent, QListViewItem* after );
        MediaItem( QListViewItem* parent, QListViewItem* after );
        void init();
        virtual ~MediaItem();

        MediaItem *lastChild() const;

        KURL url() const;
        const MetaBundle *bundle() const;
        void setBundle( MetaBundle *bundle );

        enum Type { UNKNOWN, ARTIST, ALBUM, TRACK, PODCASTSROOT, PODCASTCHANNEL,
                    PODCASTITEM, PLAYLISTSROOT, PLAYLIST, PLAYLISTITEM, INVISIBLEROOT,
                    INVISIBLE, STALEROOT, STALE, ORPHANEDROOT, ORPHANED, DIRECTORY };

        void setType( Type type );
        Type type() const { return m_type; }
        MediaItem *findItem(const QString &key, const MediaItem *after=0) const;

        virtual bool isLeafItem() const;        // A leaf node of the tree
        virtual bool isFileBacked() const;      // Should the file be deleted of the device when removed
        virtual QDateTime playTime() const { return QDateTime(); }
        virtual int  played() const { return 0; }
        virtual int  recentlyPlayed() const { return 0; } // no of times played on device since last sync
        virtual int  rating() const { return 0; } // rating on device, normalized to 100
        virtual void setRating( int rating ) { (void)rating; }
        virtual bool ratingChanged() const { return false; }
        virtual long size() const;
        virtual MediaDevice *device() const { return m_device; }

        int compare(QListViewItem *i, int col, bool ascending) const;

        void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );

        //attributes:
        int          m_order;
        Type         m_type;
        QString      m_playlistName;
        MediaDevice *m_device;

        static QPixmap *s_pixUnknown;
        static QPixmap *s_pixRootItem;
        static QPixmap *s_pixFile;
        static QPixmap *s_pixArtist;
        static QPixmap *s_pixAlbum;
        static QPixmap *s_pixPlaylist;
        static QPixmap *s_pixPodcast;
        static QPixmap *s_pixTrack;
        static QPixmap *s_pixInvisible;
        static QPixmap *s_pixStale;
        static QPixmap *s_pixOrphaned;
        static QPixmap *s_pixDirectory;

    private:
        mutable MetaBundle *m_bundle;
};

class MediaQueue : public KListView
{
    Q_OBJECT

    public:
        MediaQueue(MediaBrowser *parent);
        MediaItem *findPath( QString path );

        KIO::filesize_t totalSize() const; // total size of items to transfer in KB
        void removeSelected();
        void clearItems();

        void load( const QString &path );
        void save( const QString &path );
        void addURL( const KURL& url, MetaBundle *bundle=NULL, const QString &playlistName=QString::null );
        void addURLs( const KURL::List urls, const QString &playlistName=QString::null );
        void URLsAdded(); // call after finishing adding single urls

        // Reimplemented from KListView
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent *e );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void startDrag();

    public slots:
        void itemCountChanged();

    private slots:
        void selectAll() {QListView::selectAll(true); }
        void slotShowContextMenu( QListViewItem* item, const QPoint& point, int );

    private:
        void keyPressEvent( QKeyEvent *e );

        MediaBrowser *m_parent;
};


class MediaBrowser : public QVBox
{
    Q_OBJECT
    friend class DeviceConfigureDialog;
    friend class MediaDevice;
    friend class MediaView;
    friend class MediaQueue;
    friend class MediumPluginChooser;
    friend class MediaItem;

    public:
        static bool isAvailable();
        LIBAMAROK_EXPORT static MediaBrowser *instance() { return s_instance; }
        LIBAMAROK_EXPORT static MediaQueue *queue() { return s_instance ? s_instance->m_queue : 0; }

        MediaBrowser( const char *name );
        virtual ~MediaBrowser();
        bool blockQuit() const;
        MediaDevice *currentDevice();
        MediaDevice *deviceFromId( const QString &id );
        QStringList deviceNames();
        bool deviceSwitch( const QString &name );

        QString getInternalPluginName ( const QString string ) { return m_pluginName[string]; }
        QString getDisplayPluginName ( const QString string ) { return m_pluginAmarokName[string]; }
        const KTrader::OfferList &getPlugins() { return m_plugins; }
        void transcodingFinished( const QString &src, const QString &dst );
        void updateStats();
        void updateButtons();
        void updateDevices();
        // return bundle for url if it is known to MediaBrowser
        const MetaBundle *getBundle( const KURL &url ) const;
        bool isQuitting() const { return m_quitting; }

    signals:
        void availabilityChanged( bool isAvailable );

    protected slots:
        void transferClicked();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();
        void mediumAdded( const Medium *, QString , bool constructing = false);
        void mediumChanged( const Medium *, QString );
        void mediumRemoved( const Medium *, QString );
        void activateDevice( const MediaDevice *device );
        void activateDevice( int index, bool skipDummy = true );
        void pluginSelected( const Medium *, const QString );
        void showPluginManager();
        void cancelClicked();
        void connectClicked();
        void disconnectClicked();
        void configSelectPlugin( int index );
        bool config(); // false if cancelled by user
        KURL transcode( const KURL &src, const QString &filetype );
        void tagsChanged( const MetaBundle &bundle );
        void prepareToQuit();

    private:
        MediaDevice *loadDevicePlugin( const QString &deviceName );
        void         unloadDevicePlugin( MediaDevice *device );

        KLineEdit* m_searchEdit;
        QTimer *m_timer;
        LIBAMAROK_EXPORT static MediaBrowser *s_instance;

        QValueList<MediaDevice *> m_devices;
        QValueList<MediaDevice *>::iterator m_currentDevice;

        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
        void addDevice( MediaDevice *device );
        void removeDevice( MediaDevice *device );

        MediaQueue* m_queue;
        bool m_waitForTranscode;
        KURL m_transcodedUrl;
        QString m_transcodeSrc;

        SpaceLabel*      m_stats;
        QHBox*           m_progressBox;
        KProgress*       m_progress;
        QVBox*           m_views;
        KPushButton*     m_cancelButton;
        //KPushButton*     m_playlistButton;
        QVBox*           m_configBox;
        KComboBox*       m_configPluginCombo;
        KComboBox*       m_deviceCombo;
        enum { CONNECT, DISCONNECT, TRANSFER, CONFIGURE };
        Browser::ToolBar*m_toolbar;
        typedef QMap<QString, MediaItem*> ItemMap;
        ItemMap          m_itemMap;
        KTrader::OfferList m_plugins;
        bool             m_haveDevices;
        bool             m_quitting;
};

class MediaView : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;

    public:
        MediaView( QWidget *parent, MediaDevice *device );
        virtual ~MediaView();
        LIBAMAROK_EXPORT KURL::List nodeBuildDragList( MediaItem* item, bool onlySelected=true );
        int getSelectedLeaves(MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected=true, bool onlyPlayed=false );
        LIBAMAROK_EXPORT MediaItem *newDirectory( MediaItem* parent );
        bool setFilter( const QString &filter, MediaItem *parent=NULL );

    private slots:
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void renameItem( QListViewItem *item );
        void slotExpand( QListViewItem* );
        void selectAll() { QListView::selectAll(true); }
        void invokeItem( QListViewItem* );

    private:
        void startDrag();
        // leaves of selected items, returns no. of files within leaves

        // Reimplemented from KListView
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void viewportPaintEvent( QPaintEvent* );

        QWidget *m_parent;
        MediaDevice *m_device;
        MediaItemTip *m_toolTip;
};


/* at least the pure virtual functions have to be implemented by a media device,
   all items are stored in a hierarchy of MediaItems,
   when items are manipulated the MediaItems have to be updated accordingly */

class LIBAMAROK_EXPORT MediaDevice : public QObject, public amaroK::Plugin
{
    Q_OBJECT
    friend class DeviceConfigureDialog;
    friend class TransferDialog;
    friend class MediaBrowser;
    friend class MediaView;
    friend class MediaQueue;

    public:
        MediaDevice();
        virtual void init( MediaBrowser* parent );
        virtual ~MediaDevice();

        MediaView *view();

        virtual void rmbPressed( QListViewItem *item, const QPoint &point, int ) { (void)item; (void) point; }

        /**
         * @return list of filetypes playable on this device
         *  (empty list is interpreted as all types are good)
         */
        virtual QStringList supportedFiletypes() { return QStringList(); }

        /**
         * @param bundle describes track that should be checked
         * @return true if the device is capable of playing the track referred to by bundle
         */
        virtual bool isPlayable( const MetaBundle &bundle );

        /**
         * @param bundle describes track that should be checked
         * @return true if the track is in the preferred (first in list) format of the device
         */
        virtual bool isPreferredFormat( const MetaBundle &bundle );

        /**
         * @return true if the device is connected
         */
        virtual bool       isConnected() = 0;

        /**
         * Adds particular tracks to a playlist
         * @param playlist parent playlist for tracks to be added to
         * @param after insert following this item
         * @param items tracks to add to playlist
         */
        virtual void       addToPlaylist(MediaItem *playlist, MediaItem *after, QPtrList<MediaItem> items) { Q_UNUSED(playlist); Q_UNUSED(after); Q_UNUSED(items); }

        /**
         * Create a new playlist
         * @param name playlist title
         * @param parent parent MediaItem of the new playlist
         * @param items tracks to add to the new playlist
         * @return the newly created playlist
         */
        virtual MediaItem *newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items) { Q_UNUSED(name); Q_UNUSED(parent); Q_UNUSED(items); return 0; }

        /**
         * Move items to a directory
         * @param directory new parent of dropped items
         * @param items tracks to add to the directory
         */
        virtual void      addToDirectory( MediaItem *directory, QPtrList<MediaItem> items ) { Q_UNUSED(directory); Q_UNUSED(items); }

        /**
         * Create a new directory
         * @param name directory title
         * @param parent parent MediaItem of the new directory
         * @param items tracks to add to the new directory
         * @return the newly created directory
         */
        virtual MediaItem *newDirectory( const QString &name, MediaItem *parent ) { Q_UNUSED(name); Q_UNUSED(parent); return 0; }

        /**
         * Notify device of changed tags
         * @param item item to be updated
         * @param changed bundle containing new tags
         * @return the changed MediaItem
         */
        virtual MediaItem *tagsChanged( MediaItem *item, const MetaBundle &changed ) { Q_UNUSED(item); Q_UNUSED(changed); return 0; }

        /**
         * Indicate whether the device has a custom transfer dialog
         * @return whether there is a custom dialog
         */
        virtual bool hasTransferDialog() { return false; }

        /**
         * Run the transfer dialog to be used when Transfer is clicked
         */
        virtual void runTransferDialog() {}

        /**
         * Get the transfer dialog, if any
         * @return the transfer dialog, if any, else NULL;
         */
        virtual TransferDialog *getTransferDialog() { return NULL; }

        /**
         * Can be used to explicitly indicate whether a device needs manual configuration
         * @return whether manual configuration is needed
         */
        virtual bool needsManualConfig() { return true; }

        virtual void addConfigElements( QWidget * /*parent*/ ) {}
        virtual void removeConfigElements( QWidget * /*parent*/ ) {}
        virtual void applyConfig() {}
        virtual void loadConfig();

        QString configString( const QString &name, const QString &defValue = QString::null );
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
        MediaItem   *transferredItem() { return m_transferredItem; }
        bool         isCancelled() { return m_cancelled; }
        void         setCancelled( const bool b ) { m_cancelled = b; }

        int          progress() const;
        void         setProgress( const int progress, const int total = -1 /* leave total unchanged by default */ );
        void         hideProgress();


        /**
         * @return a unique identifier that is constant across sessions
         */
        QString uniqueId() const { return m_medium.id(); }

        /**
         * @return the name for the device that should be presented to the user
         */
        QString name() const { return m_name; }

        /**
         * @return the device node
         */
        QString deviceNode() const { return m_medium.deviceNode(); }

        /*
         * @return the device mount point (or empty if non-applicable or unknown)
         */
        QString mountPoint() const { return m_medium.mountPoint(); }

        QString           getTransferDir() { return m_transferDir; }
        Medium           &getMedium() { return m_medium; }

        void              setSpacesToUnderscores( bool yesno ) { m_spacesToUnderscores = yesno;
            setConfigBool( "spacesToUnderscores", yesno); }
        bool              getSpacesToUnderscores() { return m_spacesToUnderscores; }

        void              setFirstSort( QString text ) { m_firstSort = text;
            setConfigString( "firstGrouping", text ); }
        void              setSecondSort( QString text ) { m_secondSort = text;
            setConfigString( "secondGrouping", text ); }
        void              setThirdSort( QString text ) { m_thirdSort = text;
            setConfigString( "thirdGrouping", text ); }



    public slots:
        void abortTransfer();
        void transferFiles();
        virtual void renameItem( QListViewItem *item ) {(void)item; }
        virtual void expandItem( QListViewItem *item ) {(void)item; }
        bool connectDevice( bool silent=false );
        bool disconnectDevice( bool postdisconnecthook=true );

    protected slots:
        void fileTransferred( KIO::Job *job );
        void fileTransferFinished();

    private:
        int              sysCall(const QString & command);
        int  runPreConnectCommand();
        int  runPostDisconnectCommand();
        QString replaceVariables( const QString &cmd ); // replace %m with mount point and %d with device node

        /**
         * Find a particular track
         * @param bundle The metabundle of the requested media item
         * @return The MediaItem of the item if found, otherwise NULL
         * @note This may not be worth implementing for non database driven devices, as it could be slow
         */
        virtual MediaItem *trackExists( const MetaBundle& bundle ) = 0;

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
         * Copy a track to the device
         * @param bundle The MetaBundle of the item to transfer. Will move the item specified by bundle().url().path()
         * @return If successful, the created MediaItem in the media device view, else 0
         */
        virtual MediaItem *copyTrackToDevice(const MetaBundle& bundle) = 0;

        /**
         * Recursively remove MediaItem from the tracklist and the device
         * @param item MediaItem to remove
         * @param onlyPlayed True if item should be deleted only if it has been played
         * @return -1 on failure, number of files deleted otherwise
         */
        virtual int deleteItemFromDevice( MediaItem *item, bool onlyPlayed=false ) = 0;

        /**
         * Abort the currently active track transfer
         */
        virtual void cancelTransfer() { /* often checking m_cancel is enough */ }

        virtual void updateRootItems();

        virtual bool isSpecialItem( MediaItem *item );

        int deleteFromDevice( MediaItem *item=0, bool onlyPlayed=false, bool recursing=false );

        void purgeEmptyItems( MediaItem *root=0 );
        void syncStatsFromDevice( MediaItem *root=0 );
        void syncStatsToDevice( MediaItem *root=0 );

        bool kioCopyTrack( const KURL &src, const KURL &dst );

        QString     m_name;

        bool        m_hasMountPoint;

        QString     m_preconnectcmd;
        QString     m_postdisconnectcmd;
        bool        m_autoDeletePodcasts;
        bool        m_syncStats;

        bool        m_transcode;
        bool        m_transcodeAlways;
        bool        m_transcodeRemove;

        KShellProcess   *sysProc;
        MediaBrowser    *m_parent;
        MediaView       *m_view;
        Medium           m_medium;
        QString          m_transferDir;
        QString          m_firstSort;
        QString          m_secondSort;
        QString          m_thirdSort;
        bool             m_wait;
        bool             m_waitForDeletion;
        bool             m_copyFailed;
        bool             m_requireMount;
        bool             m_cancelled;
        bool             m_transferring;
        bool             m_deleting;
        bool             m_deferredDisconnect;
        bool             m_runDisconnectHook;
        bool             m_spacesToUnderscores;

        MediaItem       *m_transferredItem;
        QString          m_type;

        // root listview items
        MediaItem *m_playlistItem;
        MediaItem *m_podcastItem;
        // items not on the master playlist and not on the podcast playlist are not visible on the ipod
        MediaItem *m_invisibleItem;
        // items in the database for which the file is missing
        MediaItem *m_staleItem;
        // files without database entry
        MediaItem *m_orphanedItem;

        // stow away all items below m_rootItems when device is not current
        QPtrList<QListViewItem> m_rootItems;
};


#endif /* AMAROK_MEDIABROWSER_H */
