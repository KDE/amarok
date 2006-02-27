// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// License: GPL V2. See COPYING file for information.

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include "amarokconfig.h"
#include "playlistbrowseritem.h"

#include <kaction.h>
#include <klistview.h>
#include <kurl.h>
#include <qdom.h>
#include <qptrlist.h>
#include <qvbox.h>

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
class KToolBar;
class QCustomEvent;
class QColorGroup;
class QDragObject;
class QPainter;
class QPixmap;
class QPoint;
class QSplitter;
class QTimer;

class PlaylistBrowserView;
class PlaylistTrackItem;

class PlaylistBrowser : public QVBox
{
        Q_OBJECT
    friend class Party;
    friend class PlaylistBrowserView;
    friend class PlaylistBrowserEntry;
    friend class PlaylistCategory;
    friend class PlaylistEntry;
    friend class PodcastChannel;  //for changing podcast timer list

    public:
        enum ViewMode { DETAILEDVIEW, LISTVIEW, UNSORTED, ASCENDING, DESCENDING };
        enum AddMode  { PLAYLIST, STREAM, SMARTPLAYLIST, PODCAST };
        enum SaveMode { CURRENT, DYNAMIC };

        ~PlaylistBrowser();

        void addStream( QListViewItem *parent = 0 );
        void addSmartPlaylist( QListViewItem *parent = 0 );
        void addDynamic( QListViewItem *parent = 0 );
        void addPlaylist( const QString &path, QListViewItem *parent = 0, bool force=false );
        void addPodcast( QListViewItem *parent = 0 );
        void addPodcast( const QString &url, QListViewItem *parent = 0 );

        static bool savePlaylist( const QString &path, const QValueList<KURL> &urls,
                                  const QValueList<QString> &titles = QValueList<QString>(),
                                  const QValueList<QString> &seconds = QValueList<QString>(),
                                  bool relative = AmarokConfig::relativePlaylist() );

        QString partyBrowserCache() const;
        QString playlistBrowserCache() const;
        QString podcastBrowserCache() const;
        QString streamBrowserCache() const;
        QString smartplaylistBrowserCache() const;

        PlaylistBrowserEntry *findItem( QString &t, int c ) const;

        QPtrList<QListViewItem> dynamicEntries() const { return m_dynamicEntries; }

        ViewMode viewMode() const { return m_viewMode; }

        static PlaylistBrowser *instance() {
        if(!s_instance)  s_instance = new PlaylistBrowser("PlaylistBrowser");
        return s_instance; }

    signals:
        void selectionChanged();

    public slots:
        void openPlaylist( QListViewItem *parent = 0 );
        void scanPodcasts();

    private slots:
        void addToDynamic();
        void collectionScanDone();
        void currentItemChanged( QListViewItem * );
        void downloadPodcastQueue();
        void editStreamURL( StreamEntry *item );
        void removeSelectedItems();
        void renamePlaylist( QListViewItem*, const QString&, int );
        void renameSelectedItem();
        void slotDoubleClicked( QListViewItem *item );
        void subFromDynamic();

        void slotAddMenu( int id );
        void slotSaveMenu( int id );
        void slotViewMenu( int id );
        void showContextMenu( QListViewItem*, const QPoint&, int );

    private:
        PlaylistBrowser( const char* name=0 );
        void polish();

        bool m_polished;

        PlaylistCategory* loadStreams();
        void loadCoolStreams();
        void saveStreams();

        PlaylistCategory* loadSmartPlaylists();
        void loadDefaultSmartPlaylists();
        void editSmartPlaylist( SmartPlaylist* );
        void saveSmartPlaylists();

        PlaylistCategory* loadDynamics();
        void loadDynamicItems();
        void saveDynamics();

        PlaylistCategory* loadPodcasts();
        void savePodcasts();
        void changePodcastInterval();
        bool deletePodcasts( QPtrList<PodcastChannel> items );
        void downloadSelectedPodcasts();
        void refreshPodcasts( QListViewItem *category );
        void setGlobalPodcastSettings( PodcastChannel *item );

        PlaylistCategory* loadPlaylists();
        void loadOldPlaylists();
        void savePlaylists();
        void savePlaylist( PlaylistEntry * );
        bool createPlaylist( QListViewItem *parent = 0, bool current = true );
        bool deletePlaylists( QPtrList<PlaylistEntry> items );
        bool deletePlaylists( KURL::List items );

        void customEvent( QCustomEvent* e );
        void saveM3U( PlaylistEntry *, bool append );
        void savePLS( PlaylistEntry *, bool append );

        static KURL::List recurse( const KURL &url );

        static PlaylistBrowser *s_instance;

        PlaylistCategory    *m_playlistCategory;
        PlaylistCategory    *m_streamsCategory;
        PlaylistCategory    *m_smartCategory;
        PlaylistCategory    *m_dynamicCategory;
        PlaylistCategory    *m_podcastCategory;
        PlaylistCategory    *m_coolStreams;
        PlaylistCategory    *m_smartDefaults;
        PlaylistEntry       *m_lastPlaylist;

        PlaylistBrowserView *m_listview;
        KActionCollection   *m_ac;
        KAction             *removeButton, *renameButton;
        KActionMenu         *viewMenuButton;
        KActionMenu         *saveMenuButton;
        KActionMenu         *addMenuButton;
        KToolBar            *m_toolbar;
        ViewMode             m_viewMode;
        int                  m_sortMode;
        QValueList<int>      m_partySizeSave;
        QPtrList<QListViewItem> m_dynamicEntries;

        QTimer              *m_podcastTimer;
        int                  m_podcastTimerInterval;        //in ms
        QPtrList<PodcastChannel> m_podcastItemsToScan;
        QPtrList<PodcastItem> m_podcastDownloadQueue;

        bool                 m_removeDirt;
};



class PlaylistBrowserView : public KListView
{
        Q_OBJECT

    friend class PlaylistEntry;

    public:
        PlaylistBrowserView( QWidget *parent, const char *name=0 );
        ~PlaylistBrowserView();
        void startAnimation( PlaylistEntry * );
        void stopAnimation( PlaylistEntry * );

        void rename( QListViewItem *item, int c );

    protected:
        virtual void keyPressEvent( QKeyEvent * );

    private slots:
        void mousePressed( int, QListViewItem *, const QPoint &, int );
        void moveSelectedItems( QListViewItem* newParent );
        void slotAnimation();

    private:
        void startDrag();
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void eraseMarker();

        QListViewItem   *m_marker;       //track that has the drag/drop marker under it
        QTimer          *m_animationTimer;
        QPtrList<QListViewItem> m_loadingItems;
        QPixmap         *m_loading1, *m_loading2;    //icons for loading animation
};

class PlaylistDialog: public KDialogBase
{
    Q_OBJECT
    public:
        static QString getSaveFileName( const QString &suggestion = QString::null );

    private:
        KLineEdit *edit;
        bool customChosen;
        QString result;
        PlaylistDialog();

    private slots:
       void slotOk();

       void slotTextChanged( const QString &s );

       void slotCustomPath();
};

// Returns true if item is Playlist, Stream, Smart Playlist or Party.
inline bool
isElement( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == ( PlaylistEntry::RTTI || StreamEntry::RTTI ||
                             SmartPlaylist::RTTI /*|| PartyEntry::RTTI */) ? true : false;
}

inline bool
isCategory( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistCategory::RTTI ? true : false;
}

inline bool
isDynamic( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PartyEntry::RTTI ? true : false;
}

inline bool
isPlaylist( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistEntry::RTTI ? true : false;
}

inline bool
isSmartPlaylist( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == SmartPlaylist::RTTI ? true : false;
}

inline bool
isPlaylistTrackItem( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistTrackItem::RTTI ? true : false;
}

inline bool
isPodcastChannel( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PodcastChannel::RTTI ? true : false;
}

inline bool
isPodcastItem( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PodcastItem::RTTI ? true : false;
}

inline bool
isStream( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == StreamEntry::RTTI ? true : false;
}

inline QString
fileBaseName( const QString &filePath )
{
    // this function returns the file name without extension
    // (e.g. if the file path is "/home/user/playlist.m3u", "playlist" is returned
    QString fileName = filePath.right( filePath.length() - filePath.findRev( '/' ) - 1 );
    return fileName.mid( 0, fileName.findRev( '.' ) );
}

inline QString
fileDirPath( const QString &filePath )
{
    return filePath.left( filePath.findRev( '/' )+1 );
}


#endif
