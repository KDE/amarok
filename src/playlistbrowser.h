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
#include <kactionmenu.h>
#include <k3listview.h>
#include <kpushbutton.h>
#include <kurl.h>
#include <qdom.h>
#include <q3ptrlist.h>
#include <kvbox.h>
//Added by qt3to4:
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <Q3ValueList>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QPaintEvent>

class KToolBar;

class QPixmap;
class QPoint;
class QSplitter;
class QTimer;

class InfoPane;
class PlaylistBrowserView;
class PlaylistTrackItem;
class KHTMLPart;

class PlaylistBrowser : public KVBox
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

        void addStream( Q3ListViewItem *parent = 0 );
        void addSmartPlaylist( Q3ListViewItem *parent = 0 );
        void addDynamic( Q3ListViewItem *parent = 0 );
        void addPlaylist( const QString &path, Q3ListViewItem *parent = 0, bool force=false, bool imported=false );
        PlaylistEntry *findPlaylistEntry( const QString &url, Q3ListViewItem *parent=0 ) const;
        int  loadPlaylist( const QString &playlist, bool force=false );

        void addPodcast( Q3ListViewItem *parent = 0 );
        void addPodcast( const KUrl &url, Q3ListViewItem *parent = 0 );
        void loadPodcastsFromDatabase( PlaylistCategory *p = 0 );
        void registerPodcastSettings( const QString &title, const PodcastSettings *settings );

        static bool savePlaylist( const QString &path, const Q3ValueList<KUrl> &urls,
                                  const Q3ValueList<QString> &titles = Q3ValueList<QString>(),
                                  const Q3ValueList<int> &lengths = Q3ValueList<int>(),
                                  bool relative = AmarokConfig::relativePlaylist() );

        QString dynamicBrowserCache() const;
        QString playlistBrowserCache() const;
        QString podcastBrowserCache() const;
        QString streamBrowserCache() const;
        QString smartplaylistBrowserCache() const;

        PlaylistBrowserEntry *findItem( QString &t, int c ) const;
        Q3ListViewItem *findItemInTree( const QString &searchstring, int c ) const;
        PodcastEpisode *findPodcastEpisode( const KUrl &episode, const KUrl &feed ) const;

        Q3PtrList<PlaylistBrowserEntry> dynamicEntries() const { return m_dynamicEntries; }
        DynamicMode *findDynamicModeByTitle( const QString &title );
        Q3ListViewItem *podcastCategory() const { return m_podcastCategory; }

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
        void openPlaylist( Q3ListViewItem *parent = 0 );
        void scanPodcasts();

    private slots:
        void abortPodcastQueue();
        void addSelectedToPlaylist( int options = -1 );
        void collectionScanDone();
        void currentItemChanged( Q3ListViewItem * );
        void downloadPodcastQueue();
        void editStreamURL( StreamEntry *item, const bool readOnly=false );
        void removeSelectedItems();
        void renamePlaylist( Q3ListViewItem*, const QString&, int );
        void renameSelectedItem();
        void invokeItem( Q3ListViewItem*, const QPoint &, int column );
        void slotDoubleClicked( Q3ListViewItem *item );

        void slotAddMenu( int id );
        void slotAddPlaylistMenu( int id );
        void showContextMenu( Q3ListViewItem*, const QPoint&, int );

        void loadDynamicItems();

    private:
        PlaylistBrowser( const char* name=0 );
        void polish();

        bool m_polished;

        PlaylistCategory* loadStreams();
        void loadCoolStreams();
        void saveStreams();

        void loadLastfmStreams( const bool subscriber = false );
        void addLastFmRadio( Q3ListViewItem *parent );
        void addLastFmCustomRadio( Q3ListViewItem *parent );
        void saveLastFm();

        PlaylistCategory* loadSmartPlaylists();
        void loadDefaultSmartPlaylists();
        void editSmartPlaylist( SmartPlaylist* );
        void saveSmartPlaylists( PlaylistCategory *smartCategory = NULL );
        void updateSmartPlaylists( Q3ListViewItem *root );
        void updateSmartPlaylistElement( QDomElement& query );

        PlaylistCategory* loadDynamics();
        void fixDynamicPlaylistPath( Q3ListViewItem *item );
        QString guessPathFromPlaylistName( QString name );

        PlaylistCategory* loadPodcasts();
        QMap<int,PlaylistCategory*> loadPodcastFolders( PlaylistCategory *p );
        void changePodcastInterval();
        void configurePodcasts( Q3ListViewItem *parent );
        void configurePodcasts( Q3PtrList<PodcastChannel> &podcastChannelList, const QString &caption );
        void configureSelectedPodcasts();
        bool deleteSelectedPodcastItems( const bool removeItem=false, const bool silent=false );
        bool deletePodcasts( Q3PtrList<PodcastChannel> items );
        void downloadSelectedPodcasts();
        void refreshPodcasts( Q3ListViewItem *category );
        void removePodcastFolder( PlaylistCategory *item );
        void savePodcastFolderStates( PlaylistCategory *folder );
        PodcastChannel *findPodcastChannel( const KUrl &feed, Q3ListViewItem *parent=0 ) const;

        void markDynamicEntries();
        PlaylistBrowserEntry* findByName( QString name );

        PlaylistCategory* loadPlaylists();
        void savePlaylists();
        void savePlaylist( PlaylistEntry * );
        bool createPlaylist( Q3ListViewItem *parent = 0, bool current = true, QString title = 0 );
        bool deletePlaylists( Q3PtrList<PlaylistEntry> items );
        bool deletePlaylists( KUrl::List items );

        void customEvent( QEvent* e );
        void saveM3U( PlaylistEntry *, bool append );
        void savePLS( PlaylistEntry *, bool append );
        void saveXSPF( PlaylistEntry *, bool append );

        static KUrl::List recurse( const KUrl &url );

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
        Q3ValueList<int>      m_dynamicSizeSave;

        Q3Dict<PodcastSettings>   m_podcastSettings;
        Q3PtrList<PlaylistBrowserEntry>  m_dynamicEntries;

        QTimer                  *m_podcastTimer;
        int                      m_podcastTimerInterval;        //in ms
        Q3PtrList<PodcastChannel> m_podcastItemsToScan;
        Q3PtrList<PodcastEpisode> m_podcastDownloadQueue;

        InfoPane *m_infoPane;

        bool                 m_removeDirt;

        QSplitter *m_splitter;
};



class PlaylistBrowserView : public K3ListView
{
        Q_OBJECT

    friend class PlaylistEntry;

    public:
        explicit PlaylistBrowserView( QWidget *parent, const char *name=0 );
        ~PlaylistBrowserView();

        void rename( Q3ListViewItem *item, int c );

    protected:
        virtual void keyPressEvent( QKeyEvent * );

    private slots:
        void mousePressed( int, Q3ListViewItem *, const QPoint &, int );
        void moveSelectedItems( Q3ListViewItem* newParent );

    private:
        void startDrag();
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void eraseMarker();

        Q3ListViewItem   *m_marker;       //track that has the drag/drop marker under it
};

class PlaylistDialog: public KDialog
{
    Q_OBJECT
    public:
        static QString getSaveFileName( const QString &suggestion = QString(), bool proposeOverwriting = false );

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
isElement( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == ( PlaylistEntry::RTTI || StreamEntry::RTTI ||
                             SmartPlaylist::RTTI /*|| DynamicEntry::RTTI */) ? true : false;
}

inline bool
isCategory( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistCategory::RTTI ? true : false;
}

inline bool
isDynamic( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == DynamicEntry::RTTI ? true : false;
}

inline bool
isPlaylist( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistEntry::RTTI ? true : false;
}

inline bool
isSmartPlaylist( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == SmartPlaylist::RTTI ? true : false;
}

inline bool
isPlaylistTrackItem( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PlaylistTrackItem::RTTI ? true : false;
}

inline bool
isPodcastChannel( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PodcastChannel::RTTI ? true : false;
}

inline bool
isPodcastEpisode( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == PodcastEpisode::RTTI ? true : false;
}

inline bool
isStream( Q3ListViewItem *item )
{
    if( !item )
        return false;
    return item->rtti() == StreamEntry::RTTI ? true : false;
}

inline bool
isLastFm( Q3ListViewItem *item )
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
    QString fileName = filePath.right( filePath.length() - filePath.lastIndexOf( '/' ) - 1 );
    return fileName.mid( 0, fileName.lastIndexOf( '.' ) );
}

inline QString
fileDirPath( const QString &filePath )
{
    return filePath.left( filePath.lastIndexOf( '/' )+1 );
}



class InfoPane : public KVBox
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
    KHTMLPart *m_infoBrowser;
    KPushButton *m_pushButton;
    bool m_enable;
    int m_storedHeight;
};


#endif
