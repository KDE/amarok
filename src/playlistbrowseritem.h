/***************************************************************************
 * copyright            : (c) 2004 Pierpaolo Di Panfilo                    *
 *                        (c) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 *                        (c) 2006 Bart Cerneels <bart.cerneels@gmail.com> *
 *                        (c) 2006 Adam Pigg <adam@piggz.co.uk>            *
 *                        (c) 2006 Bonne Eggleston <b.eggleston@gmail.com> *
 * See COPYING file for licensing information                              *
 ***************************************************************************/

#ifndef PLAYLISTBROWSERITEM_H
#define PLAYLISTBROWSERITEM_H

#include "dynamicmode.h"
#include "podcastbundle.h"
#include "podcastsettings.h"

#include <kdialogbase.h> // StreamEditor baseclass
#include <kio/job.h>
#include <klineedit.h>
#include <klistview.h>
#include <kurl.h>

#include <qdom.h>
#include <qfile.h>
#include <qhttp.h>
#include <qptrlist.h>
#include <qtimer.h>     // Podcast loading animation
#include <qurl.h>

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
class PlaylistBrowserEntry :  public QObject, public KListViewItem
{
    Q_OBJECT
    public:
        PlaylistBrowserEntry( QListViewItem *parent, QListViewItem *after )
            : KListViewItem( parent, after) { m_kept = true; }
        PlaylistBrowserEntry( QListView *parent, QListViewItem *after )
            : KListViewItem( parent, after) { m_kept = true; }
        PlaylistBrowserEntry( QListViewItem *parent, QListViewItem *after, const QString &name )
            : KListViewItem( parent, after, name) { m_kept = true; }

        virtual QDomElement xml() const { return QDomElement(); }
        QListViewItem* parent() const { return KListViewItem::parent(); }

        bool isKept() const { return m_kept; }  // if kept == true, then it will be saved
        void setKept( bool k );                 // to the cache files. If false, non-renameable

        virtual void updateInfo();
        virtual void setDynamic( bool ) {};

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotRenameItem();
        virtual void slotPostRenameItem( const QString newName );
        virtual void showContextMenu( const QPoint & ) {};

    protected:
        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

        /** Interval of the download pixmap animation, in milliseconds */
        static const int ANIMATION_INTERVAL = 250;

    private:
        bool    m_kept;
};

class DynamicEntry : public PlaylistBrowserEntry, public DynamicMode
{
    Q_OBJECT
    public:
        DynamicEntry( QListViewItem *parent, QListViewItem *after, const QString &title );
        DynamicEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~DynamicEntry() { };

        virtual QString text( int column ) const;

        virtual QDomElement xml() const;

        static const int RTTI = 1005;
        int rtti() const { return RTTI; }

    public slots:
        virtual void slotDoubleClicked();
        virtual void showContextMenu( const QPoint & );
};

class PlaylistCategory : public PlaylistBrowserEntry
{
    Q_OBJECT
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

        virtual QDomElement xml() const;

        int   rtti() const { return RTTI; }
        static const int RTTI = 1000;    //category item

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotRenameItem();
        virtual void showContextMenu( const QPoint & );

    protected:
        void  okRename( int col );

    private:

        void setXml( const QDomElement &xml );

        QString m_title;
        int     m_id;
        bool    m_folder;
};


class PlaylistEntry :  public PlaylistBrowserEntry
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

        const KURL &url()                      const { return m_url; }
        void        setUrl( const QString &u )       { m_url.setPath( u ); }
        int         trackCount()               const { return m_trackCount; }
        int         length()                   const { return m_length; }
        bool        isDynamic()                const { return m_dynamic; }
        bool        isLoaded()                 const { return m_loaded; }

        void        setDynamic( bool );

        int         compare( QListViewItem* i, int col ) const; //reimpl.
        KURL::List  tracksURL();    //returns the list of tracks url
        void        insertTracks( QListViewItem *after, KURL::List list );
        void        insertTracks( QListViewItem *after, QValueList<MetaBundle> bundles );
        // isLast is used to avoid saving the playlist to disk every time a track is removed
        // when removing a list of tracks from the playlist
        void        removeTrack( QListViewItem *item, bool isLast = true );

        //returns a list of track information
        QPtrList<TrackItemInfo> trackList()     const { return m_trackList; }
        QPtrList<TrackItemInfo> droppedTracks() const { return tmp_droppedTracks; }

        void  setOpen( bool );
        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        virtual QDomElement xml() const;

        virtual void  updateInfo();

        //rtti is used to distinguish different kinds of list view items
        int   rtti() const { return RTTI; }
        static const int RTTI = 1001;    //playlist item

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotPostRenameItem( const QString newName );
        virtual void showContextMenu( const QPoint & );

    signals:
        void startingLoading();
        void loaded();

    private slots:
        void slotAnimation();

    private:
        void customEvent( QCustomEvent* e );
        void startAnimation();
        void stopAnimation();

        KURL                 m_url;                 //playlist url
        int                  m_length;              //total length in seconds
        int                  m_trackCount;          //track counter
        QPtrList<TrackItemInfo> m_trackList;        //tracks in playlist
        QPtrList<TrackItemInfo> tmp_droppedTracks;  //tracks dropped to the playlist while it wasn't been loaded
        bool                 m_loading;
        bool                 m_loaded;              //playlist loaded
        bool                 m_dynamic;             //the playlist is scheduled for dynamic mode rotation
        QPixmap             *m_loading1, *m_loading2;    //icons for loading animation
        QTimer               m_animationTimer;
        uint                 m_iconCounter;
        PlaylistTrackItem   *m_lastTrack;
};

class PlaylistTrackItem : public PlaylistBrowserEntry
{
    Q_OBJECT
    friend class TrackItemInfo;

    public:
        PlaylistTrackItem( QListViewItem *parent, QListViewItem *after, TrackItemInfo *info );

        const KURL    &url();
        TrackItemInfo *trackInfo() const { return m_trackInfo; }

        int rtti() const { return RTTI; }
        static const int RTTI = 1002;    //track item

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotRenameItem() { /* Do nothing */ };
        virtual void showContextMenu( const QPoint & );

    private:
        TrackItemInfo *m_trackInfo;
};

/// Stored in the database
class PodcastEpisode : public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastEpisode( QListViewItem *parent, QListViewItem *after, const QDomElement &xml,
                        const int feedType, const bool &isNew=false );
        PodcastEpisode( QListViewItem *parent, QListViewItem *after, PodcastEpisodeBundle &bundle );

        void  downloadMedia();
        void setOnDisk( bool d = true );
        QListViewItem *itemChannel() { return m_parent; }


        const bool isNew() const { return m_bundle.isNew(); }

        void setNew( const bool &n = true );
        void setListened( const bool &n = true ) { setNew( !n ); }

        // for convenience
        const int     dBId()        const { return m_bundle.dBId(); }
        const KURL    url()         const { return m_bundle.url(); }
        const QString title()       const { return m_bundle.title(); }
        const QString author()      const { return m_bundle.author(); }
        const QString date()        const { return m_bundle.date(); }
        const QString type()        const { return m_bundle.type(); }
        const QString description() const { return m_bundle.description(); }
        const QString guid()        const { return m_bundle.guid(); }
        const int     duration()    const { return m_bundle.duration(); }
        const KURL   &localUrl()    const { return m_bundle.localUrl(); }
        void  setLocalUrlBase( const QString &s );
        void setLocalUrl( const KURL &localUrl );

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        virtual void  updateInfo();

        void addToMediaDevice();

        int    rtti() const { return RTTI; }
        static const int RTTI = 1007;              //PodcastEpisode
        static void createLocalDir( const KURL &localDir );

    signals:
        void downloadFinished();
        void downloadAborted();

    public slots:
        const bool isOnDisk();
        virtual void slotDoubleClicked();
        virtual void slotRenameItem() { /* Do nothing */ };
        virtual void showContextMenu( const QPoint & );

    private slots:
        void abortDownload();
        void downloadResult( KIO::Job * transferJob );
        void slotAnimation();
        void redirected( KIO::Job * job,const KURL & redirectedUrl );

    private:
        enum FeedType{ RSS=0, ATOM=1 };

        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

        void associateWithLocalFile();

        void startAnimation();
        void stopAnimation();
        void updatePixmap();

        QListViewItem *m_parent;           //podcast channel it belongs to
        PodcastEpisodeBundle m_bundle;
        KURL        m_localUrl;

        bool        m_fetching;
        QTimer      m_animationTimer;
        uint        m_iconCounter;

        KIO::StoredTransferJob* m_podcastEpisodeJob;
        QString m_filename;

        bool        m_downloaded;       //marked as downloaded in cached xml
        bool        m_onDisk;
};

/// Stored in the database
class PodcastChannel : public PlaylistBrowserEntry
{
        Q_OBJECT

    public:
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url,
                        const QDomNode &channelSettings );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url,
                        const QDomNode &channelSettings, const QDomDocument &xml );
        PodcastChannel( QListViewItem *parent, QListViewItem *after, const PodcastChannelBundle &pcb );

        enum MediaFetch{ STREAM=0, AUTOMATIC=1 };

        void  setNew( const bool n = true );
        bool  hasNew() const { return m_new; }
        // iterate over all children and explicitly check if there are any episodes which have not been listened
        // to.  Mark the channel as new/listened after doing this.
        void  checkAndSetNew();

        void  setListened( const bool n = true ); // over rides each child so it has been listened

        void  setOpen( bool open ); // if !m_polished, load the children. Lazy loading to improve start times
        void  load();
        const bool isPolished() const { return m_polished; }

        void  configure();
        void  fetch();
        void  rescan();

        const KURL   &url()         const { return m_url; }
        const KURL    link()        const { return m_bundle.link(); }
        const QString title()       const { return m_bundle.title(); }
        const QString description() const { return m_bundle.description(); }
        const QString copyright()   const { return m_bundle.copyright(); }

        const bool autoscan()     const { return m_bundle.autoscan(); }
        const bool autotransfer() const { return m_bundle.autotransfer(); }
        const int  fetchType()    const { return m_bundle.fetchType(); }
        const bool hasPurge()     const { return m_bundle.hasPurge(); }
        const int  purgeCount()   const { return m_bundle.purgeCount(); }
        const QString &saveLocation() const { return m_bundle.saveLocation(); }
        PodcastSettings *getSettings() const
        {
            return new PodcastSettings( title(), saveLocation(),
                                        autoscan(), fetchType(), autotransfer(),
                                        hasPurge(), purgeCount() );
        }

        void setParent( PlaylistCategory *newParent );
        void setSettings( PodcastSettings *settings );
        void setXml( const QDomNode &xml, const int feedType );

        virtual void  updateInfo();

        int rtti() const { return RTTI; }
        static const int RTTI = 1006;              //podcastchannel

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotRenameItem() { /* Do nothing */ };
        virtual void showContextMenu( const QPoint & );

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
        void restorePurged();
        void removeChildren();
        void setDOMSettings( const QDomNode &channelSettings );
        void startAnimation();
        void stopAnimation();

        PodcastChannelBundle     m_bundle;

        /// loading all of the podcast episodes during startup can be very inefficient.
        /// When the user expands the podcast for the first time, we load up the episodes.
        bool                     m_polished;

        KURL                     m_url;            //remote xml url
        bool                     m_fetching;
        bool                     m_updating;
        QTimer                   m_animationTimer;
        uint                     m_iconCounter;
        bool                     m_new;
        bool                     m_hasProblem;

        KIO::TransferJob        *m_podcastJob;
        PlaylistCategory        *m_parent;        // category it belongs to
        QString                  m_podcastCurrentUrl;
        QPtrList<PodcastEpisode> m_podcastDownloadQueue;
        bool                     m_settingsValid;
};

class StreamEntry : public PlaylistBrowserEntry
{
    Q_OBJECT
    public:
        StreamEntry( QListViewItem *parent, QListViewItem *after, const KURL &, const QString &t );
        StreamEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );
        ~StreamEntry() { };

        void  setURL  ( KURL u )    { m_url = u; }
        void  setTitle( QString t ) { m_title = t; }

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        const KURL    &url()   const { return m_url; }
        const QString &title() const { return m_title; }

        virtual QDomElement xml() const;

        virtual void updateInfo();

        int    rtti() const { return RTTI; }
        static const int RTTI = 1003;    //stream item

    public slots:
        virtual void slotDoubleClicked();
        virtual void showContextMenu( const QPoint & );

    protected:
        QString m_title;
        KURL    m_url;
};

class LastFmEntry : public StreamEntry
{
    Q_OBJECT
    public:
        LastFmEntry( QListViewItem *parent, QListViewItem *after, const KURL &u, const QString &t )
            : StreamEntry( parent, after, u, t ) { }
        LastFmEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition )
            : StreamEntry( parent, after, xmlDefinition ) { }
        virtual QDomElement xml() const;

    public slots:
        virtual void slotRenameItem() { /* Do nothing */ }

    public:
        int    rtti() const { return RTTI; }
        static const int RTTI = 1008;    //lastfm item
};

class StreamEditor : public KDialogBase
{
    public:
        StreamEditor( QWidget *parent, const QString &title, const QString &url, bool readonly = false );

        KURL    url()  const { return KURL( m_urlLineEdit->text() ); }
        QString name() const { return m_nameLineEdit->text().replace( "\n", " " ); }

    private:
        KLineEdit *m_urlLineEdit;
        KLineEdit *m_nameLineEdit;

};

class SmartPlaylist : public PlaylistBrowserEntry
{
    Q_OBJECT
    public:
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &query );
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name,
                         const QString &urls, const QString &tags );
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition );

        bool        isDynamic()     const { return m_dynamic; }
        bool        isEditable()    const { return !m_xml.isNull(); }
        bool        isTimeOrdered(); //returns yes if the ordering is based on a time attribute
        QString     query();
        QString     title()         const { return m_title; }
        virtual QDomElement xml()   const { return m_xml;   }

        int   length();
        void  setDynamic( bool );
        void  setXml( const QDomElement &xml );

        int   rtti() const { return RTTI; }
        static const int RTTI = 1004;    //smart playlist item

    public slots:
        virtual void slotDoubleClicked();
        virtual void slotPostRenameItem( const QString newName );
        virtual void showContextMenu( const QPoint & );

    private:
        // for xml playlists, this member is computed on demand
        QString         m_sqlForTags;

        QString         m_title;
        QDomElement     m_xml;
        QListViewItem  *m_after;
        bool            m_dynamic;

        // Build the query for a given xml object. If \p for expand is true,
        // insert (*ExpandString*) as placeholders for childrens' filters
        static QString xmlToQuery( const QDomElement &xml, bool forExpand = false );
};

//this class is used to store information of a playlist track
class TrackItemInfo
{
    public:
        TrackItemInfo( const MetaBundle &mb );
        ~TrackItemInfo() {}
        const KURL    &url()    const { return m_url;    }
        const QString &album()  const { return m_album; }
        const QString &artist() const { return m_artist;  }
        const QString &title()  const { return m_title;  }
        const int     length()  const { return m_length; }

    private:
        KURL    m_url;
        QString m_artist;
        QString m_album;
        QString m_title;
        int     m_length;
};

/*!
    @brief Implement a shoutcast playlist category

    On open, download the shoutcast genre XML file.

    Process the file and add each genre as a ShoutcastGenre
    style PlaylistCategory
*/
class ShoutcastBrowser : public PlaylistCategory
{
        Q_OBJECT
    public:
        ShoutcastBrowser( PlaylistCategory* parent );
        void setOpen( bool open );

    public slots:
        virtual void slotDoubleClicked();

    private slots:
        void doneGenreDownload( KIO::Job *job, const KURL &from, const KURL &to, bool directory, bool renamed );
        void jobFinished( KIO::Job *job );
        void slotAnimation();

    private:
        bool          m_downloading;
        KIO::CopyJob *m_cj;
        QPixmap      *m_loading1, *m_loading2;    //icons for loading animation
        QTimer        m_animationTimer;
};

/*!
    @brief Implement a shoutcast genre category

    On open, download the shoutcast station list XML file.

    Process the file and add each station as a StreamEntry
*/
class ShoutcastGenre : public PlaylistCategory
{
        Q_OBJECT
    public:
        ShoutcastGenre( ShoutcastBrowser *browser, QListViewItem *after, QString genre );
        void setOpen( bool open );
        void appendAlternateGenre( QString alternateGenre ) { m_alternateGenres << alternateGenre; }

    public slots:
        virtual void slotDoubleClicked();

    private slots:
        void doneListDownload( KIO::Job *job, const KURL &from, const KURL &to, bool directory, bool renamed );
        void jobFinished( KIO::Job *job );
        void slotAnimation();

    private:
        void          startGenreDownload( QString genre, QString tmppath );
        bool          m_downloading;
        QString       m_genre;
        QPixmap      *m_loading1, *m_loading2;    //icons for loading animation
        QTimer        m_animationTimer;
        QStringList   m_alternateGenres;
        QStringList   m_stations;
        int           m_totalJobs;
        int           m_completedJobs;
};

#endif
