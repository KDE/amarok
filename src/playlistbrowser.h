// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include <klistview.h>
#include <kurl.h>
#include <qvbox.h>

class KAction;
class KActionCollection;
class KToolBar;
class QCustomEvent;
class QColorGroup;
class QDragObject;
class QPainter;
class QPixmap;
class QPoint;
class QSplitter;


class PlaylistBrowser : public QVBox
{
Q_OBJECT
   friend class PlaylistBrowserView;
   friend class PlaylistBrowserItem;
   friend class SmartPlaylistView;
   
   public:
       PlaylistBrowser( const char* );
       ~PlaylistBrowser();
       void loadPlaylists( QStringList files );
       void addPlaylist( QString path );
       
       static PlaylistBrowser *instance() { return s_instance; }
       
   public slots:
       void openPlaylist();

   private slots:
       void showContextMenu( QListViewItem*, const QPoint&, int );
       void removePlaylist();
       void renamePlaylist();
       void renamePlaylist( QListViewItem*, const QString&, int );
       void loadPlaylist( QListViewItem *item );
       void currentItemChanged( QListViewItem * );
           
   private:
       void customEvent( QCustomEvent* e );
       
       static PlaylistBrowser *s_instance;
       
       QSplitter *m_splitter;
       PlaylistBrowserView *m_listview;
       PlaylistBrowserItem *lastPlaylist;
       SmartPlaylistView *m_smartlistview;
       KActionCollection *m_ac;
       KAction *removeButton, *renameButton;
       KToolBar *m_toolbar;
};



class PlaylistBrowserView : public KListView
{
Q_OBJECT
    public:
        PlaylistBrowserView( QWidget *parent, const char *name=0 );
        ~PlaylistBrowserView();
        
    protected:
        virtual void keyPressEvent( QKeyEvent * );
        virtual class QDragObject *dragObject();
    
    private slots:
        void mousePressed( QListViewItem *, const QPoint &, int );
};

    

class PlaylistBrowserItem :  public QObject, public KListViewItem
{
Q_OBJECT
   public:
       PlaylistBrowserItem( KListView *, KListViewItem *, const KURL &  );
       PlaylistBrowserItem( KListViewItem *, KListViewItem *, const KURL&, const QString& );
       
       bool isPlaylist() { return m_isPlaylist; }
       
       void setUrl( const QString &u ) { m_url = KURL(u); }
       const KURL &url() { return m_url; }
       KURL::List tracks();    
       //QStringList &titleList() { return m_titleList; }
       int length() { return m_length; }
    
       void setOpen( bool );
       void setup();
       void paintCell( QPainter*, const QColorGroup&, int, int, int);
       
   private:
       void customEvent( QCustomEvent* e );
            
       KURL m_url;  //path
       KURL::List m_list; //tracks' url in playlist
       QString m_title; //track title
       QStringList m_titleList;    //tracks' title in playlist
       int m_trackn;  //number of tracks
       int m_length;    //total length in seconds
       KListViewItem *lastChild;
       bool m_isPlaylist;
       bool m_done;
};



class SmartPlaylistView : KListView
{
Q_OBJECT
    
    enum QueryType { AllCollection=0, MostPlayed, NewestTracks, RecentlyPlayed, NeverPlayed };
    
   public:
       SmartPlaylistView( QWidget *parent, const char *name = 0 );
       
       KURL::List loadSmartPlaylist( QueryType );
   
   protected:
       virtual class QDragObject *dragObject();

   private slots:
       void loadPlaylistSlot( QListViewItem * );
       
};


inline QString
fileBaseName( const QString &filePath )
{
    QString fileName = filePath.right( filePath.length() - filePath.findRev( '/' ) - 1 );
    return fileName.mid( 0, fileName.findRev( '.' ) );
}

inline QString
fileExtension( const QString &fileName )
{
    return fileName.mid( fileName.findRev( '.' ) + 1 );
}

inline QString
fileDirPath( const QString &filePath )
{
    return filePath.left( filePath.findRev( '/' )+1 );
}

#endif
