// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include <qvbox.h>
#include <klistview.h>
#include <kurl.h>
#include <qptrlist.h>

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
   friend class PlaylistBrowserItem;
   friend class SmartPlaylistView;

   public:
       enum ViewMode { DetailedView, ListView, Unsorted, SortAscending, SortDescending };

       PlaylistBrowser( const char* );
       ~PlaylistBrowser();
       void loadPlaylists();
       void addPlaylist( QString path, bool force=false );
       void savePlaylist( PlaylistBrowserItem * );
       QString playlistCacheFile();    //return the playlists stats cache file
       ViewMode viewMode() { return m_viewMode; }

       static PlaylistBrowser *instance() { return s_instance; }

   public slots:
       void openPlaylist();

   private slots:
       void slotViewMenu( int id );
       void showContextMenu( QListViewItem*, const QPoint&, int );
       void removeSelectedItems();
       void renameSelectedPlaylist();
       void deleteSelectedPlaylists();
       void renamePlaylist( QListViewItem*, const QString&, int );
       void slotDoubleClicked( QListViewItem *item );
       void currentItemChanged( QListViewItem * );

   private:
       void customEvent( QCustomEvent* e );
       void saveM3U( PlaylistBrowserItem *, bool append );
       void savePLS( PlaylistBrowserItem *, bool append );

       static PlaylistBrowser *s_instance;

       QSplitter *m_splitter;
       PlaylistBrowserView *m_listview;
       PlaylistBrowserItem *lastPlaylist;
       SmartPlaylistView *m_smartlistview;
       KActionCollection *m_ac;
       KAction *removeButton, *renameButton, *deleteButton;
       KActionMenu *viewMenuButton;
       KToolBar *m_toolbar;
       ViewMode m_viewMode;
       int m_sortMode;
};



class PlaylistBrowserView : public KListView
{
Q_OBJECT
    friend class PlaylistBrowserItem;

    public:
        PlaylistBrowserView( QWidget *parent, const char *name=0 );
        ~PlaylistBrowserView();
        void startAnimation( PlaylistBrowserItem * );
        void stopAnimation( PlaylistBrowserItem * );

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


class PlaylistBrowserItem :  public QObject, public KListViewItem
{
Q_OBJECT
    friend class PlaylistTrackItem;
    friend class TrackItemInfo;

    public:
        PlaylistBrowserItem( KListView *parent, QListViewItem *after, const KURL &, int tracks=0, int length=0 );
        ~PlaylistBrowserItem();
        void load();
        void restore();

        const KURL &url() const { return m_url; }
        void setUrl( const QString &u ) { m_url = KURL(u); }
        int trackCount() { return m_trackCount; }
        int length() { return m_length; }
        bool isModified() { return m_modified; }
        void setModified( bool );
        void setLoadingPix( QPixmap *pix ) { m_loadingPix = pix; repaint();}

        int compare( QListViewItem* i, int col, bool ascending ) const; //reimpl.
        KURL::List tracksURL();    //returns the list of tracks url
        QPtrList<TrackItemInfo> trackList() { return m_trackList; }    //returns the list of tracks information
        QPtrList<TrackItemInfo> droppedTracks() { return tmp_droppedTracks; }
        void insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map );
        void removeTrack( QListViewItem *item );

        void setOpen( bool );
        void setup();
        void paintCell( QPainter*, const QColorGroup&, int, int, int );

        //rtti is used to distinguish different kinds of list view items
        //in this case playlist items or track items
        int rtti() const { return RTTI; }
        static const int RTTI = 1001;    //playlist item

    signals:
        void startingLoading();
        void loaded();

    private:
        void customEvent( QCustomEvent* e );

        KURL m_url;  //playlist url
        int m_length;    //total length in seconds
        int m_trackCount;    //track counter
        QPtrList<TrackItemInfo> m_trackList;    //tracks in playlist
        QPtrList<TrackItemInfo> tmp_droppedTracks;    //tracks dropped to the playlist while it wasn't been loaded
        bool m_loading;
        bool m_loaded;    //playlist loaded
        bool m_modified;    //the playlist has been modified
        QPixmap *m_savePix;
        QPixmap *m_loadingPix;
        PlaylistTrackItem *m_lastTrack;
};


class PlaylistTrackItem : public KListViewItem
{
    friend class TrackItemInfo;
    public:
        PlaylistTrackItem( QListViewItem *parent, QListViewItem *after, TrackItemInfo *info );
        const KURL &url();
        TrackItemInfo *trackInfo() { return m_trackInfo; }

        int rtti() const { return RTTI; }
        static const int RTTI = 1002;    //track item

    private:
        TrackItemInfo *m_trackInfo;
};


//this class is used to store information (url, title and length) of a playlist track
class TrackItemInfo {
public:
    TrackItemInfo( const KURL &u, const QString &t, const int l );
    ~TrackItemInfo() {}
    const KURL &url() { return m_url; }
    const QString &title() { return m_title; }
    const int length() { return m_length; }

private:
    KURL m_url;
    QString m_title;
    int m_length;
};



inline bool
isPlaylist( QListViewItem *item )
{
    //this function, using rtti, checks if the list view item is a playlist
    if( !item )
        return false;
    return item->rtti() == PlaylistBrowserItem::RTTI ? true : false;
}

inline bool
isCurrentPlaylist( QListViewItem *item )
{
    if( isPlaylist( item ) )
        return static_cast<PlaylistBrowserItem*>( item )->url().protocol() == "cur";

    return false;
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
