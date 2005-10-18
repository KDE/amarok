// (c) 2004 Pierpaolo Di Panfilo
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSERITEM_H
#define PLAYLISTBROWSERITEM_H

#include <kdialogbase.h> // StreamEditor baseclass
#include <klineedit.h>
#include <klistview.h>
#include <kurl.h>

#include <qptrlist.h>
#include <qdom.h>
#include <qtimer.h>     // Podcast loading animation

class PlaylistTrackItem;
class TrackItemInfo;

namespace KIO { class Job; class TransferJob; class CopyJob; } //podcast downloads

/**
 *  RTTI VALUES
 *  1000 - PlaylistCategory
 *  1001 - PlaylistEntry
 *  1002 - PlaylistTrackItem
 *  1003 - StreamEntry
 *  1004 - SmartPlaylist
 *  1005 - PartyEntry
 *  1006 - PodcastChannel
 *  1007 - PodcastItem
 */


/* A base class to be able to use polymorphism and avoid tons of casts */
class PlaylistBrowserEntry : public KListViewItem
{
    public:
        PlaylistBrowserEntry(QListViewItem *parent, QListViewItem *after)
            :KListViewItem(parent, after) { m_notify = false; }
        PlaylistBrowserEntry(QListView *parent, QListViewItem *after)
            :KListViewItem(parent, after) { m_notify = false; }
        PlaylistBrowserEntry(QListViewItem *parent, QListViewItem *after, const QString &name )
            :KListViewItem(parent, after, name) { m_notify = false; }

        virtual QDomElement xml() { return QDomElement(); };
        bool    notify() { return m_notify; }           // use as you like ;-).  eg:
        void    setNotify( bool n ) { m_notify = n; }   // stop podcasts displaying multiple popups

    private:

        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

        bool    m_notify;
};

class PartyEntry : public PlaylistBrowserEntry
{
    public:
        PartyEntry( QListViewItem *parent, QListViewItem *after, const QString &title );
        PartyEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~PartyEntry() { };

        enum  Mode { RANDOM=0, SUGGESTION=1, CUSTOM=2 };

        const QString &title() const { return m_title; }

        QStringList items() { return m_items; }

        void  setItems( QStringList list ) { m_items = list; }
        void  setCycled( bool e )  { m_cycled = e; }
        void  setMarked( bool e )  { m_marked = e; }
        void  setUpcoming( int c ) { m_upcoming = c; }
        void  setPrevious( int c ) { m_previous = c; }
        void  setAppendCount( int c ) { m_appendCount = c; }
        void  setAppendType( int type ) { m_appendType = type; }

        bool  isCycled() { return m_cycled; }
        bool  isMarked() { return m_marked; }
        int   upcoming() { return m_upcoming; }
        int   previous() { return m_previous; }
        int   appendCount() { return m_appendCount; }
        int   appendType() { return m_appendType; }

        QDomElement xml();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1005;

    private:
        QString m_title;
        QStringList m_items;

        bool    m_cycled;
        bool    m_marked;
        int     m_upcoming;
        int     m_previous;
        int     m_appendCount;
        int     m_appendType;
};

class PlaylistCategory : public PlaylistBrowserEntry
{
    public:
        PlaylistCategory( QListView *parent, QListViewItem *after, const QString &, bool isFolder=false );
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &, bool isFolder=true );
        PlaylistCategory( QListView *parent, QListViewItem *after, const QDomElement &xmlDefinition, bool isFolder=false);
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QDomElement &xmlDefinition );

        ~PlaylistCategory() { };

        const QString &title() const { return m_title; }
        bool  isFolder() { return m_folder; }

        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        QDomElement xml();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1000;    //category item

    private:

        void setXml( const QDomElement &xml );

        QString m_title;
        bool    m_folder;
};


class PlaylistEntry :  public QObject, public PlaylistBrowserEntry
{
        Q_OBJECT

    friend class PlaylistTrackItem;
    friend class TrackItemInfo;
    friend class PlaylistCategory;

    public:
        PlaylistEntry( QListViewItem *parent, QListViewItem *after, const KURL &, int tracks=0, int length=0 );
        PlaylistEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~PlaylistEntry();

        void sortChildItems ( int /*column*/, bool /*ascending*/ ) { /* Don't sort its children */ }; //reimplemented

        void load();
        void restore();

        const KURL &url() const                   { return m_url; }
        void        setUrl( const QString &u )    { m_url.setPath( u ); }
        int         trackCount()                  { return m_trackCount; }
        int         length()                      { return m_length; }
        bool        isDynamic()                   { return m_dynamic; }
        bool        isLoaded()                    { return m_loaded; }
        bool        isModified()                  { return m_modified; }

        void        setDynamic( bool );
        void        setLoadingPix( QPixmap *pix ) { m_loadingPix = pix; repaint();}
        void        setModified( bool );

        int         compare( QListViewItem* i, int col ) const; //reimpl.
        KURL::List  tracksURL();    //returns the list of tracks url
        void        insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map );
        // isLast is used to avoid saving the playlist to disk every time a track is removed
        // when removing a list of tracks from the playlist
        void        removeTrack( QListViewItem *item, bool isLast = true );

        QPtrList<TrackItemInfo> trackList()       { return m_trackList; }    //returns the list of tracks information
        QPtrList<TrackItemInfo> droppedTracks()   { return tmp_droppedTracks; }

        void  setOpen( bool );
        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        QDomElement xml();

        //rtti is used to distinguish different kinds of list view items
        int   rtti() const { return RTTI; }
        static const int RTTI = 1001;    //playlist item

    signals:
        void startingLoading();
        void loaded();

    private:
        void customEvent( QCustomEvent* e );

        KURL                 m_url;                 //playlist url
        int                  m_length;              //total length in seconds
        int                  m_trackCount;          //track counter
        QPtrList<TrackItemInfo> m_trackList;        //tracks in playlist
        QPtrList<TrackItemInfo> tmp_droppedTracks;  //tracks dropped to the playlist while it wasn't been loaded
        bool                 m_loading;
        bool                 m_loaded;              //playlist loaded
        bool                 m_modified;            //the playlist has been modified
        bool                 m_dynamic;             //the playlist is scheduled for dynamic mode rotation
        QPixmap             *m_dynamicPix;
        QPixmap             *m_savePix;
        QPixmap             *m_loadingPix;
        PlaylistTrackItem   *m_lastTrack;
};

class PlaylistTrackItem : public PlaylistBrowserEntry
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

class PodcastItem : public QObject, public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastItem( QListViewItem *parent, QListViewItem *after, const QDomElement &xml, const int feedType );

        void  downloadMedia();
        const bool hasDownloaded() { return m_downloaded; }
        const bool hasXml( const QDomNode &xml, const int feedType );
        void  showAbout();
        QListViewItem *itemChannel() { return m_parent; }

        void setNew( bool n = true );
        bool isNew() { return m_new; }

        const KURL    &url() { return m_url; }
        const QString &title() { return m_title; }
        const QString &author() { return m_author; }
        const QString &date() { return m_date; }
        const QString &type() { return m_type; }
        const QString &description() { return m_description; }
        const int     &duration() { return m_duration; }
        const KURL    &localUrl() { return m_localUrl; }
        void  setLocalUrlBase( const QString &s );

        int rtti() const { return RTTI; }
        static const int RTTI = 1007;              //podcastitem

    signals:
        void downloadFinished();

    private slots:
        void abortDownload();
        void downloadResult( KIO::Job* job );
        void slotAnimation();

    private:
        enum FeedType{ RSS=0, ATOM=1 };

        void startAnimation();
        void stopAnimation();

        QListViewItem *m_parent;                   //podcast channel it belongs to
        QString     m_author;
        QString     m_description;
        QString     m_date;
        int         m_duration;
        QString     m_title;
        QString     m_type;
        KURL        m_url;                         //mp3 url
        KURL        m_localUrl;
        QString     m_localUrlString;              //convenience for QFile()

        QPixmap     m_loading1;
        QPixmap     m_loading2;
        bool        m_fetching;
        QTimer      m_animationTimer;
        uint        m_iconCounter;
        bool        m_downloaded;

        KIO::CopyJob* m_podcastItemJob;

        bool        m_new;
};

class PodcastChannel : public QObject, public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url,
                        const QDomNode &channelSettings, const QDomDocument &xml );

        enum MediaFetch{ STREAM=0, AUTOMATIC=1 };

        void sortChildItems( int /*column*/, bool /*ascending*/ ) { /* Don't sort its children */ }; //reimplemented

        void setNew( bool n = true );
        bool hasNew() { return m_new; }

        void  configure();
        void  fetch();
        void  rescan();
        void  setSettings( const QString &save, const bool autoFetch, const int fetchType,
                           const bool purgeItems, const int purgeCount );
        void  showAbout();

        const KURL &url() { return m_url; }
        const KURL xmlUrl();
        const QString &title() { return m_title; }

        const bool autoScan() { return m_autoScan; }
        const int  mediaFetch() { return m_mediaFetch; }
        const bool hasPurge() { return m_purgeItems; }
        const int  purgeCount() { return m_purgeCount; }
        const KURL &saveLocation() { return m_saveLocation; }
        const int  timeout() { return m_interval; }

        void setXml( const QDomNode &xml, const int feedType );
        QDomElement xml();

        int rtti() const { return RTTI; }
        static const int RTTI = 1006;              //podcastchannel

    private slots:
        void abortFetch();
        void downloadChildQueue();
        void fetchResult( KIO::Job* job );
        void slotAnimation();

    private:
        enum FeedType{ RSS=0, ATOM=1 };

        bool containsItem( QDomElement xml );
        void downloadChildren();
        void purge();
        void removeChildren();
        void startAnimation();
        void stopAnimation();

        KURL        m_url;                         //remote xml url
        KURL        m_link;                        //webpage
        KURL        m_saveLocation;
        QString     m_title;
        QString     m_cache;                       //filename for caching
        QString     m_description;
        QString     m_copyright;
        QPixmap     m_loading1;
        QPixmap     m_loading2;
        bool        m_fetching;
        bool        m_updating;
        QTimer      m_animationTimer;
        uint        m_iconCounter;
        bool        m_new;
        bool        m_hasProblem;

        // Configuration
        bool        m_autoScan;
        int         m_interval;                     //how many hours to wait b/w scans
        int         m_mediaFetch;                   //Download or stream?
        bool        m_purgeItems;
        int         m_purgeCount;

        PodcastItem          *m_last;
        KIO::TransferJob     *m_podcastJob;
        QString               m_podcastCurrentUrl;
        QPtrList<PodcastItem> m_podcastDownloadQueue;
};

class StreamEntry : public PlaylistBrowserEntry
{
    public:
        StreamEntry( QListViewItem *parent, QListViewItem *after, const KURL &, const QString &t );
        StreamEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~StreamEntry() { };

        void  setURL  ( KURL u )    { m_url = u; }
        void  setTitle( QString t ) { m_title = t; }

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        const KURL &url()      { return m_url; }
        const QString &title() { return m_title; }

        QDomElement xml();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1003;    //stream item

    private:
        QString m_title;
        KURL    m_url;

};

class StreamEditor : public KDialogBase
{
    public:
        StreamEditor( QWidget *parent, const char *name=0 );
        StreamEditor( QWidget *parent, const QString &title, const QString &url, const char *name=0 );

        KURL    url()  const { return KURL::KURL( m_urlLineEdit->text() ); }
        QString name() const { return m_nameLineEdit->text(); }

    private:
        KLineEdit *m_urlLineEdit;
        KLineEdit *m_nameLineEdit;

};

class SmartPlaylist : public PlaylistBrowserEntry
{
    public:
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &query );
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name,
                                                        const QString &urls, const QString &tags );
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );

        bool isEditable() const { return !m_xml.isNull(); }

        QString query();
        QString title() { return m_title; }

        QDomElement xml() { return m_xml; }
        void setXml( const QDomElement &xml );

        bool  isDynamic() { return m_dynamic; }
        void  setDynamic( bool );

        int   rtti() const { return RTTI; }
        static const int RTTI = 1004;    //smart playlist item

    private:
        QString m_sqlForTags;

        QString m_title;
        QDomElement m_xml;
        QListViewItem *m_after;
        bool m_dynamic;
};

//this class is used to store information (url, title and length) of a playlist track
class TrackItemInfo
{
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

#endif

