// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include "amarok.h"
#include "plugin/plugin.h"   //baseclass

#include <qvbox.h>           //baseclass
#include <qdatetime.h>

#include <klistview.h>       //baseclass
#include <kurl.h>            //stack allocated
#include "scrobbler.h"       //SubmitItem

class MediaBrowser;
class MediaDevice;
class MediaView;
class MetaBundle;
class SpaceLabel;
class Medium;

class KComboBox;
class KProgress;
class KPushButton;
class KShellProcess;

class QLabel;
class QPalette;
class MediaItemTip;

struct PodcastInfo
{
    // per show
    QString url;
    QString description;
    QDateTime date;
    QString author;

    // per channel
    QString rss;
    QString webpage;
};

class MediaItem : public KListViewItem
{
    public:
        MediaItem( QListView* parent );
        MediaItem( QListViewItem* parent );
        MediaItem( QListView* parent, QListViewItem* after );
        MediaItem( QListViewItem* parent, QListViewItem* after );
        void init();
        virtual ~MediaItem();

        MediaItem *lastChild() const;

        void  setUrl( const QString& url );
        const KURL& url() const { return m_url; }
        const MetaBundle *bundle() const;
              MetaBundle *bundle();
        PodcastInfo *podcastInfo() const { return m_podcastInfo; }

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

        int compare(QListViewItem *i, int col, bool ascending) const;
        bool match( const QString &filter ) const;

        void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );

        //attributes:
        mutable MetaBundle *m_bundle;

        KURL         m_url;
        int          m_order;
        mutable long m_size;
        Type         m_type;
        QString      m_playlistName;
        PodcastInfo *m_podcastInfo;

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
};

class MediaQueue : public KListView
{
    Q_OBJECT

    public:
        MediaQueue(MediaBrowser *parent);
        MediaItem *findPath( QString path );

        unsigned totalSize() const; // total size of items to transfer in KB
        void removeSelected();
        void clearItems();

        // Reimplemented from KListView
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent *e );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );

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
    friend class MediaDevice;
    friend class MediaView;
    friend class MediaQueue;

    public:
        static bool isAvailable();
        static MediaBrowser *instance() { return s_instance; }

        MediaBrowser( const char *name );
        virtual ~MediaBrowser();
        MediaDevice *currentDevice();
        void        addURL( const KURL& url, MetaBundle *bundle=NULL, PodcastInfo *info=NULL, const QString &playlistName=QString::null );
        void        addURLs( const KURL::List urls, const QString &playlistName=QString::null );
        void        URLsAdded();
        MediaQueue *queue() { return m_queue; }

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();
        void mediumAdded( const Medium *, QString );
        void mediumChanged( const Medium *, QString );
        void mediumRemoved( const Medium *, QString );
        void activateDevice( int index );

    private:
        MediaDevice *loadDevicePlugin( const QString &deviceName );
        void         unloadDevicePlugin( MediaDevice *device );

        KLineEdit* m_searchEdit;
        QTimer *m_timer;
        static MediaBrowser *s_instance;

        QValueList<MediaDevice *> m_devices;
        QValueList<MediaDevice *>::iterator m_currentDevice;

        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
        QMap<QString, QString> m_pluginSupports;
        void addDevice( MediaDevice *device );
        void removeDevice( MediaDevice *device );

        MediaQueue* m_queue;
        void loadTransferQueue( const QString &path );
        void saveTransferQueue( const QString &path );


    public:
    void updateStats();
    void updateButtons();
    void updateDevices();

    private slots:
    void config();

    private:
    QString          prettySize( unsigned long size ); // KB to QString
    SpaceLabel*      m_stats;
    QHBox*           m_progressBox;
    KProgress*       m_progress;
    QVBox*           m_views;
    KPushButton*     m_cancelButton;
    KPushButton*     m_transferButton;
    KPushButton*     m_connectButton;
    KPushButton*     m_disconnectButton;
    KPushButton*     m_playlistButton;
    KPushButton*     m_configButton;
    KComboBox*       m_deviceCombo;
};

class MediaView : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;

    public:
        MediaView( QWidget *parent, MediaDevice *device );
        virtual ~MediaView();
        KURL::List nodeBuildDragList( MediaItem* item, bool onlySelected=true );
        int getSelectedLeaves(MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected=true, bool onlyPlayed=false );
        MediaItem *newDirectory( MediaItem* parent );
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

class MediaDevice : public QObject, public amaroK::Plugin
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaView;
    friend class MediaQueue;

    public:
        MediaDevice();
        virtual void init( MediaBrowser* parent );
        virtual ~MediaDevice();

        void giveItemsToView();
        void takeItemsFromView();
        MediaView *view();

        virtual void rmbPressed( MediaView *deviceList, QListViewItem *item, const QPoint &point, int ) { (void)deviceList; (void)item; (void) point; }

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
         * @return true if the device is connected
         */
        virtual bool       isConnected() = 0;

        /**
         * Adds particular tracks to a playlist
         * @param playlist parent playlist for tracks to be added to
         * @param after insert following this item
         * @param items tracks to add to playlist
         */
        virtual void       addToPlaylist(MediaItem *playlist, MediaItem *after, QPtrList<MediaItem> items) = 0;

        /**
         * Create a new playlist
         * @param name playlist title
         * @param parent parent MediaItem of the new playlist
         * @param items tracks to add to the new playlist
         * @return the newly created playlist
         */
        virtual MediaItem *newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items) = 0;

        /**
         * Move items to a directory
         * @param directory new parent of dropped items
         * @param items tracks to add to the directory
         */
        virtual void      addToDirectory( MediaItem *directory, QPtrList<MediaItem> items ) = 0;

        /**
         * Create a new directory
         * @param name directory title
         * @param parent parent MediaItem of the new directory
         * @param items tracks to add to the new directory
         * @return the newly created directory
         */
        virtual MediaItem *newDirectory( const QString &name, MediaItem *parent ) = 0;

        void         setRequireMount( const bool b ) { m_requireMount = b; }
        void         setDeviceType( const QString &type ) { m_type = type; }
        QString      deviceType() { return m_type; }
        virtual bool autoConnect() { return false; }
        virtual bool asynchronousTransfer() { return false; }
        bool         isTransferring() { return m_transferring; }
        MediaItem   *transferredItem() { return m_transferredItem; }
        bool         isCancelled() { return m_cancelled; }
        void         setCancelled( const bool b ) { m_cancelled = b; }

        int          progress() const;
        void         setProgress( const int progress, const int total = -1 /* leave total unchanged by default */ );
        void         hideProgress();


        /**
         * @return a unique identifier that is constant across sessions
         */
        QString uniqueId() const { return m_uniqueId; }

        /**
         * @return the name for the device that should be presented to the user
         */
        QString name() const { return m_name; }

    public slots:
        void abortTransfer();
        void connectClicked( bool silent=false );
        void disconnectClicked( bool silent=false );
        int  mount();
        void setMountPoint(const QString & mntpnt);
        void setMountCommand(const QString & mnt);
        void setUmountCommand(const QString & umnt);
        void setAutoDeletePodcasts(bool value);
        void setSyncStats(bool value);
        int  umount();
        void transferFiles();
        virtual void renameItem( QListViewItem *item ) {(void)item; }
        virtual void expandItem( QListViewItem *item ) {(void)item; }

    protected slots:
        void fileTransferFinished();

    private:
        int              sysCall(const QString & command);

        /**
         * Find a particular track
         * @param bundle The metabundle of the requested media item
         * @return The MediaItem of the item if found, otherwise NULL
         * @note This may not be worth implementing for non database driven devices, as it could be slow
         */
        virtual MediaItem *trackExists( const MetaBundle& bundle ) = 0;

    protected:
        /**
         * Get the capacity and freespace available on the device, in KB
         * @return true if successful
         */
        virtual bool getCapacity( unsigned long *total, unsigned long *available ) = 0;

        /**
         * Lock device for exclusive access if possible
         */
        virtual void lockDevice( bool ) = 0;

        /**
         * Unlock device
         */
        virtual void unlockDevice() = 0;

        /**
         * Connect to device, and populate m_listview with MediaItems
         * @param silent if true, suppress error dialogs
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
         * @param isPodcast true if item is a podcast
         * @return If successful, the created MediaItem in the media device view, else 0
         */
        virtual MediaItem *copyTrackToDevice(const MetaBundle& bundle, const PodcastInfo *info) = 0;

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
        virtual void cancelTransfer() = 0;

        virtual void updateRootItems();

        int deleteFromDevice( MediaItem *item=0, bool onlyPlayed=false, bool recursing=false );

        void purgeEmptyItems( MediaItem *root=0 );
        void syncStatsFromDevice( MediaItem *root=0 );
        void syncStatsToDevice( MediaItem *root=0 );
        bool connectDevice( bool silent=false );
        bool disconnectDevice();

        QString     m_name;
        QString     m_uniqueId;

        QString     m_mntpnt;
        QString     m_mntcmd;
        QString     m_umntcmd;
        bool        m_autoDeletePodcasts;
        bool        m_syncStats;

        KShellProcess   *sysProc;
        MediaBrowser    *m_parent;
        MediaView       *m_listview;
        bool             m_wait;
        bool             m_waitForDeletion;
        bool             m_copyFailed;
        bool             m_requireMount;
        bool             m_hasPodcast;
        bool             m_hasStats;
        bool             m_hasPlaylists;
        bool             m_cancelled;
        bool             m_transferring;
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
