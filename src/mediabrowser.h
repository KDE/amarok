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
#include "medium.h"
#include "metabundle.h"
#include "pluginmanager.h"
#include "plugin/plugin.h"   //baseclass
#include "scrobbler.h"       //SubmitItem

#include <k3listview.h>       //baseclass
#include <KHBox>
#include <kio/global.h>      //filesize_t
#include <KUrl>            //stack allocated
#include <KVBox>           //baseclass

#include <QDateTime>
#include <QLabel>
#include <QMutex>
#include <QPainter>
#include <QPaintEvent>
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

class SpaceLabel : public QLabel {
    public:
    SpaceLabel(QWidget *parent)
        : QLabel(parent)
    {
        m_total = m_used = m_scheduled = 0;
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), palette().brush(QColorGroup::Background));

        if(m_total > 0)
        {
            int used = int(float(m_used)/float(m_total)*width());
            int scheduled = int(float(m_used + m_scheduled)/float(m_total)*width());

            if(m_used > 0)
            {
                QColor blueish(70,120,255);
                if(e->rect().left() < used)
                {
                    int right = used;
                    if(e->rect().right() < right)
                        right = e->rect().right();
                    p.fillRect(e->rect().left(), e->rect().top(),
                            used, e->rect().bottom()+1, QBrush(blueish, Qt::SolidPattern));
                }
            }

            if(m_scheduled > 0)
            {
                QColor sched(70, 230, 120);
                if(m_used + m_scheduled > m_total - m_total/200)
                {
                    sched.setRgb( 255, 120, 120 );
                }
                int left = e->rect().left();
                if(used > left)
                    left = used;
                int right = e->rect().right();
                if(scheduled < right)
                    right = scheduled;
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, QBrush(sched, Qt::SolidPattern));
            }

            if(m_used + m_scheduled < m_total)
            {
                QColor grey(180, 180, 180);
                int left = e->rect().left();
                if(scheduled > left)
                    left = scheduled;
                int right = e->rect().right();
                p.fillRect(left, e->rect().top(), right, e->rect().bottom()+1, palette().brush(QColorGroup::Background));
            }
        }
        QLabel::paintEvent(e);
    }

    KIO::filesize_t m_total;
    KIO::filesize_t m_used;
    KIO::filesize_t m_scheduled;
};

class MediaQueue : public K3ListView
{
    Q_OBJECT

    public:
        MediaQueue(MediaBrowser *parent);
        MediaItem *findPath( QString path );

        KIO::filesize_t totalSize() const; // total size of items to transfer in KB
        void computeSize() const; // compute total size of items to transfer in KB
        void addItemToSize( const MediaItem *item ) const;
        void subtractItemFromSize( const MediaItem *item, bool unconditonally=false ) const;

        void removeSelected();
        void clearItems();

        void load( const QString &path );
        void save( const QString &path );
        void syncPlaylist( const QString &playlistName, const QString &sql, bool loading=false );
        void syncPlaylist( const QString &playlistName, const KUrl &url, bool loading=false );
        void addUrl( const KUrl& url, MetaBundle *bundle=NULL, const QString &playlistName=QString() );
        void addUrl( const KUrl& url, MediaItem *item );
        void addUrls( const KUrl::List urls, const QString &playlistName=QString() );

        void URLsAdded(); // call after finishing adding single urls

        void dropProxyEvent( QDropEvent *e );
        // Reimplemented from K3ListView
        bool acceptDrag( QDropEvent *e ) const;
        Q3DragObject *dragObject();

    public slots:
        void itemCountChanged();

    private slots:
        void selectAll() {Q3ListView::selectAll(true); }
        void slotShowContextMenu( Q3ListViewItem* item, const QPoint& point, int );
        void slotDropped (QDropEvent* e, Q3ListViewItem* parent, Q3ListViewItem* after);

    private:
        void keyPressEvent( QKeyEvent *e );
        MediaBrowser *m_parent;
        mutable KIO::filesize_t m_totalSize;
};


class MediaBrowser : public KVBox
{
    Q_OBJECT
    friend class MediaDevice;
    friend class MediaView;
    friend class MediaQueue;
    friend class MediaItem;

    public:
        static bool isAvailable();
        AMAROK_EXPORT static MediaBrowser *instance() { return s_instance; }
        AMAROK_EXPORT static MediaQueue *queue() { return s_instance ? s_instance->m_queue : 0; }

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
        bool getBundle( const KUrl &url, MetaBundle *bundle ) const;
        bool isQuitting() const { return m_quitting; }

        KUrl getProxyUrl( const KUrl& daapUrl ) const;
        KToolBar* getToolBar() const { return m_toolbar; }
        KAction *connectAction() const { return m_connectAction; }
        KAction *disconnectAction() const { return m_disconnectAction; }
        KAction *transferAction() const { return m_transferAction; }
        KAction *configAction() const { return m_configAction; }
        KAction *customAction() const { return m_customAction; }

    protected slots:
        void transferClicked();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();
        void slotSetFilter( const QString &filter );
        void slotEditFilter();
        void deviceAdded( const QString &uid );
        void deviceRemoved( const QString &uid );
        void activateDevice( const MediaDevice *device );
        void activateDevice( int index, bool skipDummy = true );
        //TODO Put these back!
        void pluginSelected( const QString &, const QString & );
        void showPluginManager();
        //void configSelectPlugin( int index );
        void cancelClicked();
        void connectClicked();
        void disconnectClicked();
        void customClicked();
        bool config(); // false if canceled by user
        KUrl transcode( const KUrl &src, const QString &filetype );
        void tagsChanged( const MetaBundle &bundle );
        void prepareToQuit();

    private:
        MediaDevice *loadDevicePlugin( const QString &uid );
        void         unloadDevicePlugin( MediaDevice *device );

        QTimer *m_timer;
        AMAROK_EXPORT static MediaBrowser *s_instance;

        QList<MediaDevice *> m_devices;
        MediaDevice * m_currentDevice;

        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
        void addDevice( MediaDevice *device );
        void removeDevice( MediaDevice *device );

        MediaQueue* m_queue;
        bool m_waitForTranscode;
        KUrl m_transcodedUrl;
        QString m_transcodeSrc;

        SpaceLabel*      m_stats;
        KHBox*           m_progressBox;
        QProgressBar*       m_progress;
        KVBox*           m_views;
        KPushButton*     m_cancelButton;
        //KPushButton*     m_playlistButton;
        KVBox*           m_configBox;
        KComboBox*       m_configPluginCombo;
        KComboBox*       m_deviceCombo;
        Browser::ToolBar*m_toolbar;
        typedef QMap<QString, MediaItem*> ItemMap;
        mutable QMutex   m_itemMapMutex;
        ItemMap          m_itemMap;
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

class MediaView : public K3ListView
{
    Q_OBJECT
    friend class MediaBrowser;
    friend class MediaDevice;

    public:
        enum Flags
        {
            None = 0,
            OnlySelected = 1,
            OnlyPlayed = 2
        };

        MediaView( QWidget *parent, MediaDevice *device );
        virtual ~MediaView();
        AMAROK_EXPORT KUrl::List nodeBuildDragList( MediaItem* item, int flags=OnlySelected );
        AMAROK_EXPORT int getSelectedLeaves(MediaItem *parent, QList<MediaItem*> *list, int flags=OnlySelected );
        AMAROK_EXPORT MediaItem *newDirectory( MediaItem* parent );
        bool setFilter( const QString &filter, MediaItem *parent=NULL );

    private slots:
        void rmbPressed( Q3ListViewItem*, const QPoint&, int );
        void renameItem( Q3ListViewItem *item );
        void slotExpand( Q3ListViewItem* );
        void selectAll() { Q3ListView::selectAll(true); }
        void invokeItem( Q3ListViewItem*, const QPoint &, int column );
        void invokeItem( Q3ListViewItem* );

    private:
        void keyPressEvent( QKeyEvent *e );
        // Reimplemented from K3ListView
        void contentsDropEvent( QDropEvent *e );
        void viewportPaintEvent( QPaintEvent* );
        bool acceptDrag( QDropEvent *e ) const;
        Q3DragObject *dragObject();

        QWidget *m_parent;
        MediaDevice *m_device;
};

#endif /* AMAROK_MEDIABROWSER_H */
