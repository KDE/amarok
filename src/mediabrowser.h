// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include <qhbox.h>
#include <qvbox.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klistview.h>       //baseclass
#include <kurl.h>            //stack allocated

class Color;
class MetaBundle;

class KProgress;
class QLabel;
class QPalette;


class MediaItem : public KListViewItem
{
    public:
        MediaItem( QListView* parent )
            : KListViewItem( parent ) {};
        MediaItem( QListViewItem* parent )
            : KListViewItem( parent ) {};

        void setUrl( const QString& url ) { m_url.setPath( url ); }
        const KURL& url() const { return m_url; }

        //attributes:
        KURL m_url;
};


class MediaBrowser : public QVBox
{
    Q_OBJECT
    friend class MediaDeviceView;
    friend class MediaDevice;

    public:
        static bool isAvailable();

        MediaBrowser( const char *name );
        ~MediaBrowser();

    private:
        MediaDeviceView* m_view;
        MediaDevice* m_device;

        KLineEdit* m_searchEdit;
};


class MediaDeviceList : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;
    friend class Item;
    
    public:
        MediaDeviceList( MediaDeviceView* parent );
        ~MediaDeviceList();

        void renderView();

    private slots:
        void slotExpand( QListViewItem* );
        void slotCollapse( QListViewItem* );
        void renderNode( QListViewItem* parent, const KURL& url );

    private:
        void startDrag();
        void nodeBuildDragList( MediaItem* item );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDropEvent( QDropEvent *e );
        void contentsDragMoveEvent( QDragMoveEvent* e );

        MediaDeviceView* m_parent;
        KURL::List       m_dragList;
};


class MediaDeviceView : public QVBox
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;
    friend class MediaDeviceList;

    public:
        MediaDeviceView( MediaBrowser* parent );
        ~MediaDeviceView();
        
    private:
        QLabel*          m_stats;
        KProgress*       m_progress;
        MediaDeviceList* m_deviceList;
        KListView*       m_transferList;

        MediaBrowser* m_parent;
};


class MediaDevice : public QObject
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDeviceView;

    public:
        MediaDevice( MediaBrowser* parent );
        void addURL( const KURL& url );

        static MediaDevice *instance() { return s_instance; }

    public slots:
        void transferFiles();
        
    private slots:
        void fileTransferred( KIO::Job *job, const KURL &from, const KURL &to, bool dir, bool renamed );
        void fileTransferFinished( KIO::Job *job );

    private:
        bool fileExists( const MetaBundle& bundle );
        KURL::List m_transferURLs;

        MediaBrowser* m_parent;
        static MediaDevice *s_instance;
};


#endif /* AMAROK_MEDIABROWSER_H */
