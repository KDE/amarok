// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include <qhbox.h>
#include <qvbox.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klistview.h>       //baseclass
#include <kprocess.h>
#include <kurl.h>            //stack allocated
#include "debug.h"
#include "metabundle.h"

class MediaDevice;
class MediaDeviceView;
class MetaBundle;
class SpaceLabel;

class KProgress;
class KPushButton;
class QLabel;
class QPalette;

class MediaItem : public KListViewItem
{
    public:
        MediaItem( QListView* parent );
        MediaItem( QListViewItem* parent );
        MediaItem( QListView* parent, QListViewItem* after );
        MediaItem( QListViewItem* parent, QListViewItem* after );
        void init();

        MediaItem *lastChild() const;

        void setUrl( const QString& url );
        const KURL& url() const { return m_url; }
        const MetaBundle *bundle() const;
        MetaBundle *bundle();

        enum Type { UNKNOWN, ARTIST, ALBUM, TRACK, PODCASTSROOT, PODCASTCHANNEL,
                    PODCASTITEM, PLAYLISTSROOT, PLAYLIST, PLAYLISTITEM, INVISIBLEROOT,
                    INVISIBLE, STALEROOT, STALE, ORPHANEDROOT, ORPHANED, DIRECTORY };

        void setType( Type type );
        Type type() const { return m_type; }
        MediaItem *findItem(const QString &key) const;

        virtual bool isLeafItem() const;        // A leaf node of the tree
        virtual bool isFileBacked() const;      // Should the file be deleted of the device when removed
        virtual int  played() const { return 0; }
        virtual long size() const;

        //attributes:
        KURL m_url;
        mutable MetaBundle *m_bundle;

        int m_order;
        mutable long m_size;
        Type m_type;
        QString m_playlistName;
        int compare(QListViewItem *i, int col, bool ascending) const;

        static QPixmap *s_pixUnknown;
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

class MediaDeviceTransferList : public KListView
{
    Q_OBJECT

    public:
        MediaDeviceTransferList(MediaDeviceView *parent);
        MediaItem *findPath( QString path );

        // Reimplemented from KListView
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent *e );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        unsigned totalSize() const; // total size of items to transfer in KB

    private:
        MediaDeviceView *m_parent;
};


class MediaBrowser : public QVBox
{
    Q_OBJECT
    friend class MediaDevice;
    friend class MediaDeviceList;
    friend class MediaDeviceView;

    public:
        static bool isAvailable();

        MediaBrowser( const char *name );
        ~MediaBrowser();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();

    private:
        MediaDeviceView* m_view;

        KLineEdit* m_searchEdit;
        QTimer *m_timer;
};


class MediaDeviceList : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;

    public:
        MediaDeviceList( MediaDeviceView* parent );
        ~MediaDeviceList();

    private slots:
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void renameItem( QListViewItem *item );
        void slotExpand( QListViewItem* );

    private:
        void startDrag();
        KURL::List nodeBuildDragList( MediaItem* item, bool onlySelected=true );
        // leaves of selected items, returns no. of files within leaves
        int getSelectedLeaves(MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected=true, bool onlyPlayed=false );

        // Reimplemented from KListView
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void viewportPaintEvent( QPaintEvent* );

        MediaDeviceView* m_parent;
        QString m_renameFrom;
};


class MediaDeviceView : public QVBox
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;
    friend class MediaDeviceList;
    friend class MediaDeviceTransferList;

    public:
        MediaDeviceView( MediaBrowser* parent );
        ~MediaDeviceView();
        bool setFilter( const QString &filter, MediaItem *parent=NULL );
        void updateStats();

    private slots:
        void slotShowContextMenu( QListViewItem* item, const QPoint& point, int );

    private:
        QString          prettySize( unsigned long size ); // KB to QString
        bool             match( const MediaItem *item, const QString &filter );
        SpaceLabel*      m_stats;
        KProgress*       m_progress;
        MediaDevice*     m_device;
        MediaDeviceList* m_deviceList;
        KPushButton*     m_transferButton;
        KPushButton*     m_connectButton;
        KPushButton*     m_playlistButton;
        KPushButton*     m_configButton;

        MediaBrowser* m_parent;
};

/* at least the pure virtual functions have to be implemented by a media device,
   all items are stored in a hierarchy of MediaItems,
   when items are manipulated the MediaItems have to be updated accordingly */

class MediaDevice : public QObject
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDeviceView;
    friend class MediaDeviceList;
    friend class MediaDeviceTransferList;

    public:
        MediaDevice( MediaDeviceView* parent, MediaDeviceList* listview );
        virtual ~MediaDevice();

        void        addURL( const KURL& url, MetaBundle *bundle=NULL, bool isPodcast=false, const QString &playlistName=QString::null );
        void        addURLs( const KURL::List urls, const QString &playlistName=QString::null );

        // true if the device is connected
        virtual bool        isConnected() = 0;

        // add tracks in 'items' to playlist represented by 'playlist' and insert them after 'after'
        virtual void        addToPlaylist(MediaItem *playlist, MediaItem *after, QPtrList<MediaItem> items) = 0;

        // create a new playlist named 'name' consisting of 'items' and add it to 'parent'
        virtual MediaItem * newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items) = 0;

        void   setRequireMount( const bool b ) { m_requireMount = b; }

        static MediaDevice *instance() { return s_instance; }

    public slots:
        void clearItems();
        void config();
        void connectDevice( bool silent=false );
        int  mount();
        void removeSelected();
        void setMountPoint(const QString & mntpnt);
        void setMountCommand(const QString & mnt);
        void setUmountCommand(const QString & umnt);
        void setAutoDeletePodcasts(bool value);
        int  umount();
        void transferFiles();
        virtual void renameItem( QListViewItem *item ) {(void)item; }
        virtual void expandItem( QListViewItem *item ) {(void)item; }

    private slots:
        void fileTransferred();
    protected slots:
        void fileTransferFinished();

    private:
        int              sysCall(const QString & command);

        // return MediaItem corresponding to track described by 'bundle' or NULL if no such track exists on device
        virtual MediaItem *trackExists( const MetaBundle& bundle ) = 0;

    protected:

        QString     m_mntpnt;
        QString     m_mntcmd;
        QString     m_umntcmd;
        bool        m_autoDeletePodcasts;

        KShellProcess   *sysProc;
        MediaDeviceView* m_parent;
        MediaDeviceList* m_listview;
        bool             m_wait;
        bool             m_requireMount;

        MediaDeviceTransferList* m_transferList;
        void loadTransferList( const QString &path );
        void saveTransferList( const QString &path );

        // fill 'total' with total and 'available' with available capacity on media device, unit is KB
        virtual bool getCapacity( unsigned long *total, unsigned long *available ) = 0;

        // if possible, lock device for exclusive access
        virtual void lockDevice( bool ) = 0;

        // allow access for others again
        virtual void unlockDevice() = 0;

        // connect to device, do not open any dialog boxes if 'silent' is true,
        // return true on success and fill m_listview with MediaItems
        virtual bool openDevice( bool silent=false ) = 0;

        // disconnect device
        virtual bool closeDevice() = 0;

        // write changes to device (especially to database)
        virtual void synchronizeDevice() = 0;

        // add track located on media device at 'pathname' to device database,
        // use metadata from 'bundle', add to podcasts if 'isPodcast' is true
        virtual MediaItem *addTrackToDevice(const QString& pathname, const MetaBundle& bundle, bool isPodcast) = 0;
        virtual void updateRootItems();

        void deleteFromDevice( MediaItem *item, bool onlyPlayed=false, bool recursing=false );
        void deleteFile( const KURL &url);

        // recursively remove 'item', only the already played (sub-) if 'onlyPlayed' is true
        virtual bool deleteItemFromDevice( MediaItem *item, bool onlyPlayed=false ) = 0;

        // return a pathname where a new track with metadata 'bundle' has to be transferred for adding to media device database
        virtual QString createPathname(const MetaBundle& bundle) = 0;

        static MediaDevice *s_instance;

        // root listview items
        MediaItem *m_playlistItem;
        MediaItem *m_podcastItem;
        // items not on the master playlist and not on the podcast playlist are not visible on the ipod
        MediaItem *m_invisibleItem;
        // items in the database for which the file is missing
        MediaItem *m_staleItem;
        // files without database entry
        MediaItem *m_orphanedItem;

};


#endif /* AMAROK_MEDIABROWSER_H */
