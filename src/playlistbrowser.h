// (c) Max Howell 2004
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
   friend class SmartPlaylistView;
   
   class PlaylistListView : public KListView
    {
        public:
            PlaylistListView( QWidget *parent=0, const char *name=0 )
                : KListView( parent, name) {}

        protected:
            virtual class QDragObject *dragObject();
    };
    
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
           
   private:
       void customEvent( QCustomEvent* e );
       
       static PlaylistBrowser *s_instance;
       
       QSplitter *m_splitter;
       PlaylistListView *m_listview;
       SmartPlaylistView *m_smartlistview;
       KActionCollection *m_ac;
       KToolBar *m_toolbar;
};


class PlaylistBrowserItem :  public QObject, public KListViewItem
{
Q_OBJECT
   public:
       PlaylistBrowserItem( KListView *, QListViewItem *, const KURL &  );
       PlaylistBrowserItem( KListViewItem *, KListViewItem *, const KURL&, const QString&, int );
       
       bool isPlaylist() { return m_isPlaylist; }
       
       void setUrl( const QString &u ) { m_url = KURL(u); }
       const KURL &url() { return m_url; }
       int length() { return m_length; } //return length in seconds
    
       void setup();
       void paintCell( QPainter*, const QColorGroup&, int, int, int);
   private:
       void customEvent( QCustomEvent* e );
            
       KURL m_url;  //playlist path
       QString m_title; //track title
       int m_trackn;  //number of tracks
       int m_length;
       KListViewItem *lastChild;
       bool m_isPlaylist;
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
