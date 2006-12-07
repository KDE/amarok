/***************************************************************************
 * copyright            : (c) 2004 Pierpaolo Di Panfilo                    *
 *                        (c) 2004 Mark Kretschmann <markey@web.de>        *
 *                        (c) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 *                        (c) 2005 Gábor Lehel <illissius@gmail.com>       *
 *                        (c) 2006 Adam Pigg <adam@piggz.co.uk>           *
 * See COPYING file for licensing information                              *
 ***************************************************************************/

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include "amarokconfig.h"
#include "playlistbrowseritem.h"
#include "podcastsettings.h"

#include <kaction.h>
#include <klistview.h>
#include <kpushbutton.h>
#include <kurl.h>
#include <qdom.h>
#include <qptrlist.h>
#include <qvbox.h>

class KTextBrowser;
class KToolBar;

class QCustomEvent;
class QColorGroup;
class QDragObject;
class QPainter;
class QPixmap;
class QPoint;
class QSplitter;
class QTimer;

class HTMLView;
class InfoPane;
class PlaylistBrowserView;
class PlaylistTrackItem;


class PlaylistBrowser : public QVBox
{
        Q_OBJECT
    friend class DynamicMode;
    friend class PlaylistBrowserView;

    friend class PlaylistBrowserEntry;
    friend class PlaylistCategory;
    friend class PlaylistEntry;
    friend class PlaylistTrackItem;
    friend class PodcastChannel;  //for changing podcast timer list
    friend class PodcastEpisode;
    friend class DynamicEntry;
    friend class StreamEntry;
    friend class SmartPlaylist;


    public:
        enum AddMode  { PLAYLIST, PLAYLIST_IMPORT, STREAM, SMARTPLAYLIST, PODCAST, ADDDYNAMIC };

        ~PlaylistBrowser();

        void setInfo( const QString &title, const QString &info );

        void addStream( QListViewItem *parent = 0 );
        void addSmartPlaylist( QListViewItem *parent = 0 );
        void addDynamic( QListViewItem *parent = 0 );
        void addPlaylist( const QString &path, QListViewItem *parent = 0, bool force=false, bool imported=false );
        PlaylistEntry *findPlaylistEntry( const QString &url, QListViewItem *parent=0 ) const;
        int  loadPlaylist( const QString &playlist, bool force=false );

        void addPodcast( QListViewItem *parent = 0 );
        void addPodcast( const KURL &url, QListViewItem *parent = 0 );
        void loadPodcastsFromDatabase( PlaylistCategory *p = 0 );
        void registerPodcastSettings( const QString &title, const PodcastSettings *settings );

        static bool savePlaylist( const QString &path, const QValueList<KURL> &urls,
                                  const QValueList<QString> &titles = QValueList<QString>(),
                                  const QValueList<int> &lengths = QValueList<int>(),
                                  bool relative = AmarokConfig::relativePlaylist() );

        QString dynamicBrowserCache() const;
        QString playlistBrowserCache() const;
        QString podcastBrowserCache() const;
        QString streamBrowserCache() const;
        QString smartplaylistBrowserCache() const;

        PlaylistBrowserEntry *findItem( QString &t, int c ) const;
        QListViewItem *findItemInTree( const QString &searchstring, int c ) const;
        PodcastEpisode *findPodcastEpisode( const KURL &episode, const KURL &feed ) const;

        QPtrList<PlaylistBrowserEntry> dynamicEntries() const { return m_dynamicEntries; }
        DynamicMode *findDynamicModeByTitle( const QString &title );
        QListViewItem *podcastCategory() const { return m_podcastCategory; }

        static PlaylistBrowser *instance() {
            if(!s_instance)  s_instance = new PlaylistBrowser("PlaylistBrowser");
            return s_instance;
        }

        //following used by PlaylistSelection.cpp
        PlaylistBrowserView* getListView() const { return m_listview; }
        PlaylistCategory* getDynamicCategory() const { return m_dynamicCategory; }
        void saveDynamics();

    protected:
        virtual void resizeEvent( QResizeEvent * );

    signals:
        void selectionChanged();

    public slots:
        void openPlaylist( QListViewItem *parent = 0 );
        void scanPodcasts();

    private slots:
        void abortPodcastQueue();
        void addSelectedToPlaylist( int options = -1 );
        void collectionScanDone();
        void currentItemChanged( QListViewItem * );
        void downloadPodcastQueue();
        void editStreamURL( StreamEntry *item, const bool readOnly=false );
        void removeSelectedItems();
        void renamePlaylist( QListViewItem*, const QString&, int );
        void renameSelectedItem();
        void invokeItem( QListViewItem*, const QPoint &, int column );
        void slotDoubleClicked( QListViewItem *item );

        void slotAddMenu( int id );
        void slotAddPlaylistMenu( int id );
        void showContextMenu( QListViewItem*, const QPoint&, int );

        void loadDynamicItems();

    private:
        PlaylistBrowser( const char* name=0 );
        void polish();

        bool m_polished;

        PlaylistCategory* loadStreams();
        void loadCoolStreams();
        void saveStreams();

        void loadLastfmStreams( const bool subscriber = false );
        void addLastFmRadio( QListViewItem *parent );
        void addLastFmCustomRadio( QListViewItem *parent );
        void saveLastFm();

        PlaylistCategory* loadSmartPlaylists();
        void loadDefaultSmartPlaylists();
        void editSmartPlaylist( SmartPlaylist* );
        void saveSmartPlaylists( PlaylistCategory *smartCategory = NULL );
        void updateSmartPlaylists( QListViewItem *root );
        void updateSmartPlaylistElement( QDomElement& query );

        PlaylistCategory* loadDynamics();
        void fixDynamicPlaylistPath( QListViewItem *item );
        QString guessPathFromPlaylistName( QString name );

        PlaylistCategory* loadPodcasts();
        QMap<int,PlaylistCategory*> loadPodcastFolders( PlaylistCategory *p );
        void changePodcastInterval();
        void configurePodcasts( QListViewItem *parent );
        void configurePodcasts( QPtrList<PodcastChannel> &podcastChannelList, const QString &caption );
        void configureSelectedPodcasts();
        bool deleteSelectedPodcastItems( const bool removeItem=false, const bool silent=false );
        bool deletePodcasts( QPtrList<PodcastChannel> items );
        void downloadSelectedPodcasts();
        void refreshPodcasts( QListViewItem *category );
        void removePodcastFolder( PlaylistCategory *item );
        void savePodcastFolderStates( PlaylistCategory *folder );
        PodcastChannel *findPodcastChannel( const KURL &feed, QListViewItem *parent=0 ) const;

        void markDynamicEntries();
        PlaylistBrowserEntry* findByName( QString name );

        PlaylistCategory* loadPlaylists();
        void savePlaylists();
        void savePlaylist( PlaylistEntry * );
        bool createPlaylist( QListViewItem *parent = 0, bool current = true, QString title = 0 );
        bool deletePlaylists( QPtrList<PlaylistEntry> items );
        bool deletePlaylists( KURL::List items );

        void customEvent( QCustomEvent* e );
        void saveM3U( PlaylistEntry *, bool append );
        void savePLS( PlaylistEntry *, bool append );
        void saveXSPF( PlaylistEntry *, bool append );

        static KURL::List recurse( const KURL &url );

        static PlaylistBrowser *s_instance;

        PlaylistCategory    *m_playlistCategory;
        PlaylistCategory    *m_streamsCategory;
        PlaylistCategory    *m_smartCategory;
        PlaylistCategory    *m_dynamicCategory;
        PlaylistCategory    *m_podcastCategory;
        PlaylistCategory    *m_coolStreams;
        PlaylistCategory    *m_smartDefaults;
        PlaylistCategory    *m_lastfmCategory;
        ShoutcastBrowser    *m_shoutcastCategory;
        PlaylistEntry       *m_lastPlaylist;

        DynamicEntry        *m_randomDynamic;
        DynamicEntry        *m_suggestedDynamic;

        bool                 m_coolStreamsOpen;
        bool                 m_smartDefaultsOpen;
        bool                 m_lastfmOpen;

        PlaylistBrowserView *m_listview;
        KActionCollection   *m_ac;
        KAction             *removeButton;
        KAction             *renameButton;
        KActionMenu         *viewMenuButton;
        KActionMenu         *addMenuButton;
        KToolBar            *m_toolbar;
        QValueList<int>      m_dynamicSizeSave;

        QDict<PodcastSettings>   m_podcastSettings;
        QPtrList<PlaylistBrowserEntry>  m_dynamicEntries;

        QTimer                  *m_podcastTimer;
        int                      m_podcastTimerInterval;        //in ms
        QPtrList<PodcastChannel> m_podcastItemsToScan;
        QPtrList<PodcastEpisode> m_podcastDownloadQueue;

        InfoPane *m_infoPane;

        bool                 m_removeDirt;

        QSplitter *m_splitter;
};



class PlaylistBrowserView : public KListView
{
        Q_OBJECT

    friend class PlaylistEntry;

    public:
        PlaylistBrowserView( QWidget *parent, const char *name=0 );
        ~PlaylistBrowserView();

        void rename( QListViewItem *item, int c );

    protected:
        virtual void keyPressEvent( QKeyEvent * );

    private slots:
        void mousePressed( int, QListViewItem *, const QPoint &, int );
        void moveSelectedItems( QListViewItem* newParent );

    private:
        void startDrag();
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void eraseMarker();

        QListViewItem   *m_marker;       //track that has the drag/drop marker under it
};

class PlaylistDialog: public KDialogBase
{
    Q_OBJECT
    public:
        static QString getSaveFileName( const QString &suggestion = QString::null, bool proposeOverwriting = false );

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

// Returns true if item is Playlist, Stream, Smart Playlist or DynamicMode.
inline bool
isElement( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == ( PlaylistEntry::RTTI || StreamEntry::RTTI ||
                             SmartPlaylist::RTTI /*|| DynamicEntry::RTTI */) ? true : false;
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
    return item->rtti() == DynamicEntry::RTTI ? true : false;
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
isPodcastEpisode( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PodcastEpisode::RTTI ? true : false;
}

inline bool
isStream( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == StreamEntry::RTTI ? true : false;
}

inline bool
isLastFm( QListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == LastFmEntry::RTTI ? true : false;
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



class InfoPane : public QVBox
{
    Q_OBJECT

public:
    InfoPane( QWidget *parent );
    ~InfoPane();
    int getHeight();
    void setStoredHeight( const int newHeight );

public slots:
    void setInfo( const QString &title, const QString &info );

private slots:
    void toggle( bool );

private:
    HTMLView *m_infoBrowser;
    KPushButton *m_pushButton;
    bool m_enable;
    int m_storedHeight;
};


#endif
