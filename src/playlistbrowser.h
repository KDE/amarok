// (c) Pierpaolo Di Panfilo 2004
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include "playlistbrowseritem.h"

#include <klistview.h>
#include <kurl.h>
#include <qptrlist.h>
#include <qvbox.h>

class KAction;
class KActionMenu;
class KActionCollection;
class KToolBar;
class QCustomEvent;
class QColorGroup;
class QDragObject;
class QPainter;
class QPixmap;
class QPoint;
class QSplitter;
class QTimer;

class PlaylistBrowser : public QVBox
{
        Q_OBJECT

    friend class PlaylistBrowserView;
    friend class PlaylistEntry;
    friend class SmartPlaylistView;

    public:
        enum ViewMode { DETAILEDVIEW, LISTVIEW, UNSORTED, ASCENDING, DESCENDING };
        enum AddMode  { PLAYLIST, STREAM, SMARTPLAYLIST };

        PlaylistBrowser( const char* );
        ~PlaylistBrowser();

        void loadStreams();
        void addStream();

        void loadSmartPlaylists();
        void addSmartPlaylist();
        void editSmartPlaylist();

        void loadPartyConfigs();
        void addPartyConfig();
        void editPartyConfig();

        void loadPlaylists();
        void addPlaylist( QString path, bool force=false );
        void savePlaylist( PlaylistEntry * );

        QString partyCacheFile();
        QString playlistCacheFile();
        QString smartCacheFile();
        QString streamCacheFile();

        ViewMode viewMode() { return m_viewMode; }

        static PlaylistBrowser *instance() { return s_instance; }

    public slots:
        void openPlaylist();

    private slots:
        void currentItemChanged( QListViewItem * );
        void deleteSelectedPlaylists();
        void editStreamURL( StreamEntry *item );
        void removeSelectedItems();
        void renamePlaylist( QListViewItem*, const QString&, int );
        void renameSelectedPlaylist();
        void saveCurrentPlaylist();
        void slotDoubleClicked( QListViewItem *item );

        void slotAddMenu( int id );
        void slotViewMenu( int id );
        void showContextMenu( QListViewItem*, const QPoint&, int );

    private:
        void customEvent( QCustomEvent* e );
        void saveM3U( PlaylistEntry *, bool append );
        void savePLS( PlaylistEntry *, bool append );

        static PlaylistBrowser *s_instance;

        KListViewItem       *m_lastPlaylist;
        PlaylistCategory    *m_playlistCategory;
        PlaylistCategory    *m_streamsCategory;
        PlaylistCategory    *m_smartCategory;
        PlaylistCategory    *m_partyCategory;


        QSplitter *m_splitter;
        PlaylistBrowserView *m_listview;
        SmartPlaylistView   *m_smartlistview;
        KActionCollection   *m_ac;
        KAction             *removeButton, *renameButton, *deleteButton;
        KAction             *saveCurrentButton;
        KActionMenu         *viewMenuButton;
        KActionMenu         *addMenuButton;
        KToolBar            *m_toolbar;
        ViewMode             m_viewMode;
        int                  m_sortMode;
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
        void slotAnimation();

    private:
        void startDrag();
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void eraseMarker();

        QListViewItem *m_marker;       //track that has the drag/drop marker under it
        QTimer *m_animationTimer;
        QPtrList<QListViewItem> m_loadingItems;
        QPixmap *m_loading1, *m_loading2;    //icons for loading animation
};


inline bool
isCategory( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistCategory::RTTI ? true : false;
}

inline bool
isPlaylist( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistEntry::RTTI ? true : false;
}

inline bool
isPlaylistTrackItem( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistTrackItem::RTTI ? true : false;
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
fileExtension( const QString &fileName )
{
    return fileName.right( 4 );
}

inline QString
fileDirPath( const QString &filePath )
{
    return filePath.left( filePath.findRev( '/' )+1 );
}

#endif
