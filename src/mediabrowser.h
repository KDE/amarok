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
        MediaItem( QListView* parent )
            : KListViewItem( parent ) { m_bundle=NULL; m_track=0; m_podcast=false; }
        MediaItem( QListViewItem* parent )
            : KListViewItem( parent ) { m_bundle=NULL;  m_track=0; m_podcast=false; }
        MediaItem( QListView* parent, QListViewItem* after )
            : KListViewItem( parent, after ) { m_bundle=NULL;  m_track=0; m_podcast=false; }
        MediaItem( QListViewItem* parent, QListViewItem* after )
            : KListViewItem( parent, after ) { m_bundle=NULL;  m_track=0; m_podcast=false; }

        void setUrl( const QString& url ) { m_url.setPath( url ); }
        const KURL& url() const { return m_url; }
        int m_track;
        bool m_podcast;
        const MetaBundle *bundle() const { if(m_bundle == NULL) m_bundle = new MetaBundle( url() ); return m_bundle; }
        MetaBundle *bundle() { if(m_bundle == NULL) m_bundle = new MetaBundle( url() ); return m_bundle; }

        //attributes:
        KURL m_url;
        mutable MetaBundle *m_bundle;

        int compare(QListViewItem *i, int col, bool ascending) const
        {
            MediaItem *item = (MediaItem *)i;
            if(col==0 && item->m_track != m_track)
                return ascending ? m_track-item->m_track : item->m_track-m_track;

            return KListViewItem::compare(i, col, ascending);
        }
};

class MediaDeviceTransferList : public KListView
{
    Q_OBJECT

    public:
        MediaDeviceTransferList(MediaDeviceView *parent);

        // Reimplemented from KListView
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent *e );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );

        MediaItem *findPath( QString path );

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

    private:
        MediaDeviceView* m_view;

        KLineEdit* m_searchEdit;
};


class MediaDeviceList : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;
    friend class GpodMediaDevice;
    friend class Item;

    public:
        MediaDeviceList( MediaDeviceView* parent );
        ~MediaDeviceList();

    private slots:
        void slotCollapse( QListViewItem* );
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void renderView( QListViewItem* parent );
        void renameItem( QListViewItem* item );

    private:
        void startDrag();
        KURL::List nodeBuildDragList( MediaItem* item );

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
    friend class GpodMediaDevice;
    friend class MediaDeviceTransferList;

    public:
        MediaDeviceView( MediaBrowser* parent );
        ~MediaDeviceView();

    private slots:
        void slotShowContextMenu( QListViewItem* item, const QPoint& point, int );

    private:
        QLabel*          m_stats;
        KProgress*       m_progress;
        MediaDevice*     m_device;
        MediaDeviceList* m_deviceList;
        MediaDeviceTransferList*
                         m_transferList;
        KPushButton*     m_transferButton;
        KPushButton*     m_connectButton;
        KPushButton*     m_configButton;

        MediaBrowser* m_parent;
};


class MediaDevice : public QObject
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDeviceView;

    public:
        MediaDevice( MediaDeviceView* parent );
        virtual ~MediaDevice();

        void        addURL( const KURL& url, MetaBundle *bundle=NULL, bool isPodcast=false );
        void        addURLs( const KURL::List urls, MetaBundle *bundle=NULL );
        virtual bool        isConnected() = 0;
        virtual QStringList items( QListViewItem* item ) = 0;
        virtual KURL::List  songsByArtist( const QString& artist ) = 0;
        virtual KURL::List  songsByArtistAlbum( const QString& artist, const QString& album ) = 0;
        virtual bool renameArtist( const QString& oldArtist, const QString& newArtist ) = 0;
        virtual bool renameAlbum( const QString& artist,
                const QString& oldAlbum, const QString& newAlbum ) = 0;
        virtual bool renameTrack( const QString& artist, const QString& album,
                const QString& oldTrack, const QString& newTrack ) = 0;

        QString     m_mntpnt;
        QString     m_mntcmd;
        QString     m_umntcmd;

        static MediaDevice *instance() { return s_instance; }

    public slots:
        void clearItems();
        void config();
        virtual void deleteFiles( const KURL::List& urls ) = 0;
        virtual void connectDevice() = 0;
        int  mount();
        void removeSelected();
        void setMountPoint(const QString & mntpnt);
        void setMountCommand(const QString & mnt);
        void setUmountCommand(const QString & umnt);
        int  umount();
        virtual void transferFiles() = 0;

    private slots:
        void fileTransferred();
    protected slots:
        void fileTransferFinished();

    private:
        int              sysCall(const QString & command);
        virtual bool     fileExists( const MetaBundle& bundle ) = 0;

    protected:
        KShellProcess   *sysProc;
        MediaDeviceView* m_parent;
        bool             m_wait;

        void loadTransferList( const QString &path );
        void saveTransferList( const QString &path );

        static MediaDevice *s_instance;
};


#endif /* AMAROK_MEDIABROWSER_H */
