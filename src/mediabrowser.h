// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include <qvbox.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klistview.h>       //baseclass
#include <kurl.h>            //stack allocated

class Color;
class MetaBundle;

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


class MediaDevice : public QVBox
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDeviceView;
    friend class Item;

    public:
        MediaDevice( MediaBrowser* m_parent );
        void addURL( const KURL& url );

        static MediaDevice *instance() { return s_instance; }

    private slots:
        void transferFiles();
        void fileTransferred( KIO::Job *job, const KURL &from, const KURL &to, bool dir, bool renamed );
        void fileTransferFinished( KIO::Job *job );

    private:
        bool fileExists( const MetaBundle& bundle );
        KListView* m_transferList;
        KURL::List m_transferURLs;

        MediaBrowser* m_parent;
        static MediaDevice *s_instance;
};


class MediaDeviceView : public KListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class Item;

    public:
        MediaDeviceView( MediaBrowser* parent );
        ~MediaDeviceView();
        
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

        MediaBrowser* m_parent;
        KURL::List    m_dragList;
};

#endif /* AMAROK_MEDIABROWSER_H */
