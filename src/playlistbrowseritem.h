/***************************************************************************
 * copyright            : (c) 2004 Pierpaolo Di Panfilo                    *
 *                        (c) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 * See COPYING file for licensing information                              *
 ***************************************************************************/

#ifndef PLAYLISTBROWSERITEM_H
#define PLAYLISTBROWSERITEM_H

#include "dynamicmode.h"
#include "podcastbundle.h"
#include "podcastsettings.h"

#include <kdialogbase.h> // StreamEditor baseclass
#include <klineedit.h>
#include <klistview.h>
#include <kurl.h>

#include <qptrlist.h>
#include <qdom.h>
#include <qtimer.h>     // Podcast loading animation

class MetaBundle;
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
 *  1005 - DynamicEntry (Dynamic)
 *  1006 - PodcastChannel
 *  1007 - PodcastEpisode
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
        bool    notify() const { return m_notify; }           // use as you like ;-).  eg:
        void    setNotify( const bool n ) { m_notify = n; }   // stop podcasts displaying multiple popups

        virtual void updateInfo() { return; }

    protected:
        /** Interval of the download pixmap animation, in milliseconds */
        static const int ANIMATION_INTERVAL = 250;

    private:

        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

        bool    m_notify;
};

class DynamicEntry : public PlaylistBrowserEntry, public DynamicMode
{
    public:
        DynamicEntry( QListViewItem *parent, QListViewItem *after, const QString &title );
        DynamicEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~DynamicEntry() { };

        virtual QString text( int column ) const;

        QDomElement xml();

        static const int RTTI = 1005;
        int rtti() const { return RTTI; }
};

class PlaylistCategory : public PlaylistBrowserEntry
{
    public:
        PlaylistCategory( QListView *parent, QListViewItem *after, const QString &, bool isFolder=false );
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &, bool isFolder=true );
        PlaylistCategory( QListView *parent, QListViewItem *after, const QDomElement &xmlDefinition, bool isFolder=false);
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &t, const int id );

        ~PlaylistCategory() { };

        const QString &title() const { return m_title; }
        bool  isFolder() { return m_folder; }

        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        void  setId( const int id ) { m_id = id; }
        const int id() const { return m_id; }

        QDomElement xml();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1000;    //category item
        
    protected:
        void  okRename( int col );

    private:

        void setXml( const QDomElement &xml );

        QString m_title;
        int     m_id;
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

        const KURL &url() const                   { return m_url; }
        void        setUrl( const QString &u )    { m_url.setPath( u ); }
        int         trackCount()                  { return m_trackCount; }
        int         length()                      { return m_length; }
        bool        isDynamic()                   { return m_dynamic; }
        bool        isLoaded()                    { return m_loaded; }

        void        setDynamic( bool );
        void        setLoadingPix( QPixmap *pix ) { m_loadingPix = pix; repaint();}

        int         compare( QListViewItem* i, int col ) const; //reimpl.
        KURL::List  tracksURL();    //returns the list of tracks url
        void        insertTracks( QListViewItem *after, KURL::List list );
        void        insertTracks( QListViewItem *after, QValueList<MetaBundle> bundles );
        // isLast is used to avoid saving the playlist to disk every time a track is removed
        // when removing a list of tracks from the playlist
        void        removeTrack( QListViewItem *item, bool isLast = true );

        QPtrList<TrackItemInfo> trackList()       { return m_trackList; }    //returns the list of tracks information
        QPtrList<TrackItemInfo> droppedTracks()   { return tmp_droppedTracks; }

        void  setOpen( bool );
        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        QDomElement xml();

        void  updateInfo();

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
        bool                 m_dynamic;             //the playlist is scheduled for dynamic mode rotation
        QPixmap             *m_dynamicPix;
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

/// Stored in the database
class PodcastEpisode : public QObject, public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastEpisode( QListViewItem *parent, QListViewItem *after, const QDomElement &xml, 
                        const int feedType, const bool &isNew=false );
        PodcastEpisode( QListViewItem *parent, QListViewItem *after, PodcastEpisodeBundle &bundle );

        void  downloadMedia();
        const bool isOnDisk();
        void setOnDisk( bool d = true );
        QListViewItem *itemChannel() { return m_parent; }

        
        const bool isNew()         const { return m_bundle.isNew(); }
        
        void setNew( const bool &n = true );
        void setListened( const bool &n = true );

        // for convenience
        const int     dBId()        { return m_bundle.dBId(); }
        const KURL    url()         { return m_bundle.url(); }
        const QString title()       { return m_bundle.title(); }
        const QString author()      { return m_bundle.author(); }
        const QString date()        { return m_bundle.date(); }
        const QString type()        { return m_bundle.type(); }
        const QString description() { return m_bundle.description(); }
        const QString guid()        { return m_bundle.guid(); }
        const int     duration()    { return m_bundle.duration(); }
        const KURL    &localUrl()   { return m_localUrl; }
        void  setLocalUrlBase( const QString &s );

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        void  updateInfo();

        void addToMediaDevice();

        int rtti() const { return RTTI; }
        static const int RTTI = 1007;              //PodcastEpisode
        static void createLocalDir( const KURL &localDir );

    signals:
        void downloadFinished();
        void downloadAborted();

    private slots:
        void abortDownload();
        void downloadResult( KIO::Job* job );
        void slotAnimation();

    private:
        enum FeedType{ RSS=0, ATOM=1 };

        void startAnimation();
        void stopAnimation();
        void updatePixmap();
        KURL saveURL( const KURL &base, const QString &filetype, const KURL &link );

        QListViewItem *m_parent;           //podcast channel it belongs to
        PodcastEpisodeBundle m_bundle;
        KURL        m_localUrl;

        bool        m_fetching;
        QTimer      m_animationTimer;
        uint        m_iconCounter;

        KIO::CopyJob* m_podcastEpisodeJob;

        bool        m_downloaded;       //marked as downloaded in cached xml
        bool        m_onDisk;
};

/// Stored in the database
class PodcastChannel : public QObject, public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url, const QDomNode &channelSettings );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url,
                        const QDomNode &channelSettings, const QDomDocument &xml );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const PodcastChannelBundle &pcb );

        enum MediaFetch{ STREAM=0, AUTOMATIC=1 };

        void sortChildItems( int /*column*/, bool /*ascending*/ ) { /* Don't sort its children */ }; //reimplemented

        void setNew( bool n = true );
        bool hasNew() { return m_new; }

        void  configure();
        void  fetch();
        void  rescan();

        const KURL     url()         { return m_url; }
        const KURL     link()        { return m_bundle.link(); }
        const QString  title()       { return m_bundle.title(); }
        const QString  description() { return m_bundle.description(); }
        const QString  copyright()   { return m_bundle.copyright(); }

        const int  fetchType()      { return m_bundle.fetchType(); }
        const bool autoscan()       { return m_bundle.autoscan(); }
        const bool autotransfer()   { return m_bundle.autotransfer(); }
        const bool hasPurge()       { return m_bundle.hasPurge(); }
        const int  purgeCount()     { return m_bundle.purgeCount(); }
        const KURL saveLocation()   { return m_bundle.saveLocation(); }

        void  setXml( const QDomNode &xml, const int feedType );
        void  setParent( PlaylistCategory *newParent );

        void  updateInfo();

        int rtti() const { return RTTI; }
        static const int RTTI = 1006;              //podcastchannel

    private slots:
        void abortFetch();
        void downloadChildQueue();
        void fetchResult( KIO::Job* job );
        void slotAnimation();

    private:
        enum FeedType{ RSS=0, ATOM=1 };
        static const int EPISODE_LIMIT = 10; //Maximum number of episodes initially shown

        bool containsItem( QDomElement xml );
        void downloadChildren();
        const bool episodeExists( const QDomNode &xml, const int feedType );
        void purge();
        void removeChildren();
        void setDOMSettings( const QDomNode &channelSettings );
        void startAnimation();
        void stopAnimation();

        PodcastChannelBundle m_bundle;

        KURL        m_url;                         //remote xml url
        bool        m_fetching;
        bool        m_updating;
        QTimer      m_animationTimer;
        uint        m_iconCounter;
        bool        m_new;
        bool        m_hasProblem;

        KIO::TransferJob        *m_podcastJob;
        PlaylistCategory        *m_parent; // category it belongs to
        QString                  m_podcastCurrentUrl;
        QPtrList<PodcastEpisode> m_podcastDownloadQueue;
        bool        m_settingsValid;
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

        void  updateInfo();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1003;    //stream item

    private:
        QString m_title;
        KURL    m_url;

};

class StreamEditor : public KDialogBase
{
    public:
        StreamEditor( QWidget *parent, const QString &title, const QString &url, bool readonly = false );

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

