// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIABROWSER_H
#define AMAROK_MEDIABROWSER_H

#include "ipod/ipod.h"

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
    friend class Item;
    
    public:
        MediaDeviceList( MediaDeviceView* parent );
        ~MediaDeviceList();

    private slots:
        void slotCollapse( QListViewItem* );
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void renderView( QListViewItem* parent );
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
        MediaDevice*     m_device;
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
        MediaDevice( MediaDeviceView* parent );
        ~MediaDevice();

        void addURL( const KURL& url );
        QStringList items( QListViewItem* item );
        KURL::List songsByArtist( const QString& artist );
        KURL::List songsByArtistAlbum( const QString& artist, const QString& album );

        static MediaDevice *instance() { return s_instance; }

    public slots:
        void transferFiles();
        void deleteFiles( const KURL::List& urls );
        
    private slots:
        void fileTransferred( KIO::Job *job, const KURL &from, const KURL &to, bool dir, bool renamed );
        void fileTransferFinished( KIO::Job *job );
        void syncIPod();

    private:
        bool fileExists( const MetaBundle& bundle );
        KURL::List m_transferURLs;

        MediaDeviceView* m_parent;
        IPod::IPod* m_ipod;
        static MediaDevice *s_instance;
};


#endif /* AMAROK_MEDIABROWSER_H */
