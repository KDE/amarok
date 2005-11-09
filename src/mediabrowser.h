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

        void setUrl( const QString& url );
        const KURL& url() const { return m_url; }
        const MetaBundle *bundle() const;
        MetaBundle *bundle();
        enum Type { UNKNOWN, ARTIST, ALBUM, TRACK, PODCASTSROOT, PODCASTCHANNEL, PODCASTITEM, PLAYLISTSROOT, PLAYLIST, PLAYLISTITEM, INVISIBLEROOT, INVISIBLE, STALEROOT, STALE, ORPHANEDROOT, ORPHANED };
        void setType( Type type );
        Type type() const { return m_type; }
        MediaItem *findItem(const QString &key) const;
        bool isLeaveItem() const;
        virtual int played() const { return 0; }

        //attributes:
        KURL m_url;
        mutable MetaBundle *m_bundle;

        int m_order;
        Type m_type;
        int compare(QListViewItem *i, int col, bool ascending) const;

        static QPixmap *s_pixFile;
        static QPixmap *s_pixArtist;
        static QPixmap *s_pixAlbum;
        static QPixmap *s_pixPlaylist;
        static QPixmap *s_pixPodcast;
        static QPixmap *s_pixTrack;
        static QPixmap *s_pixInvisible;
        static QPixmap *s_pixStale;
        static QPixmap *s_pixOrphaned;
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

    private:
        void startDrag();
        KURL::List nodeBuildDragList( MediaItem* item, bool onlySelected=true );
        int getSelectedLeaves(MediaItem *parent, QPtrList<MediaItem> *list, bool onlySelected=true, bool onlyPlayed=false ); // leaves of selected items, returns no. of files within leaves

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
    friend class GpodMediaDevice;

    public:
        MediaDeviceView( MediaBrowser* parent );
        ~MediaDeviceView();
        bool setFilter( const QString &filter, MediaItem *parent=NULL );

    private slots:
        void slotShowContextMenu( QListViewItem* item, const QPoint& point, int );

    private:
        bool             match( const MediaItem *item, const QString &filter );
        QLabel*          m_stats;
        KProgress*       m_progress;
        MediaDevice*     m_device;
        MediaDeviceList* m_deviceList;
        KPushButton*     m_transferButton;
        KPushButton*     m_connectButton;
        KPushButton*     m_playlistButton;
        KPushButton*     m_configButton;

        MediaBrowser* m_parent;
};


class MediaDevice : public QObject
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDeviceView;
    friend class MediaDeviceList;

    public:
        MediaDevice( MediaDeviceView* parent, MediaDeviceList* listview );
        virtual ~MediaDevice();

        void        addURL( const KURL& url, MetaBundle *bundle=NULL, bool isPodcast=false );
        void        addURLs( const KURL::List urls );
        virtual bool        isConnected() = 0;
        virtual void        addToPlaylist(MediaItem *playlist, MediaItem *after, QPtrList<MediaItem> items) = 0;
        virtual MediaItem * newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items) = 0;

        static MediaDevice *instance() { return s_instance; }

    public slots:
        void clearItems();
        void config();
        void connectDevice();
        int  mount();
        void removeSelected();
        void setMountPoint(const QString & mntpnt);
        void setMountCommand(const QString & mnt);
        void setUmountCommand(const QString & umnt);
        void setAutoDeletePodcasts(bool value);
        int  umount();
        void transferFiles();
        virtual void renameItem( QListViewItem *item ) {(void)item; }

    private slots:
        void fileTransferred();
    protected slots:
        void fileTransferFinished();

    private:
        int              sysCall(const QString & command);
        virtual bool     trackExists( const MetaBundle& bundle ) = 0;

    protected:

        QString     m_mntpnt;
        QString     m_mntcmd;
        QString     m_umntcmd;
        bool        m_autoDeletePodcasts;

        KShellProcess   *sysProc;
        MediaDeviceView* m_parent;
        MediaDeviceList* m_listview;
        bool             m_wait;

        MediaDeviceTransferList* m_transferList;
        void loadTransferList( const QString &path );
        void saveTransferList( const QString &path );

        virtual void lockDevice( bool ) = 0;
        virtual void unlockDevice() = 0;
        virtual bool openDevice( bool useDialogs=true ) = 0;
        virtual bool closeDevice() = 0;
        virtual void synchronizeDevice() = 0;
        virtual MediaItem *addTrackToDevice(const QString& pathname, const MetaBundle& bundle, bool isPodcast) = 0;
        virtual void updateRootItems();

        void deleteFromDevice( MediaItem *item, bool onlyPlayed=false, bool recursing=false );
        void deleteFile( const KURL &url);
        virtual bool deleteItemFromDevice( MediaItem *item, bool onlyPlayed=false ) = 0;

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
