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

class PlaylistTrackItem;
class TrackItemInfo;

/**
 *  RTTI VALUES
 *  1000 - PlaylistCategory
 *  1001 - PlaylistEntry
 *  1002 - PlaylistTrackItem
 *  1003 - StreamEntry
 *  1004 - SmartPlaylist
 *  1005 - PartyEntry
 */


// For saving the current playlist, party
class ItemSaver : public KDialogBase
{
    public:
        ItemSaver( QString title, QWidget *parent, const char *name=0 );

        QString title() const { return m_nameLineEdit->text(); }

    private:
        KLineEdit *m_nameLineEdit;
};


/* A base class to be able to use polymorphism and avoid tons of casts */
class PlaylistBrowserEntry : public KListViewItem
{
    public:
        PlaylistBrowserEntry(QListViewItem *parent, QListViewItem *after)
            :KListViewItem(parent, after) {};
        PlaylistBrowserEntry(QListView *parent, QListViewItem *after)
            :KListViewItem(parent, after) {};
        PlaylistBrowserEntry(QListViewItem *parent, QListViewItem *after, const QString &name )
            :KListViewItem(parent, after, name) {};

        virtual QDomElement xml() { return QDomElement(); };
};

class PartyEntry : public PlaylistBrowserEntry
{
    public:
        PartyEntry( QListViewItem *parent, QListViewItem *after, const QString &title );
        PartyEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition );
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
        PlaylistCategory( QListView *parent, QListViewItem *after, QDomElement xmlDefinition, bool isFolder=false);
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, QDomElement xmlDefinition );

        ~PlaylistCategory() { };

        const QString &title() const { return m_title; }
        bool  isFolder() { return m_folder; }

        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        QDomElement xml();

        int   rtti() const { return RTTI; }
        static const int RTTI = 1000;    //category item

    private:

        void setXml( QDomElement xml );

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
        PlaylistEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition );
        ~PlaylistEntry();
        void load();
        void restore();

        const KURL &url() const                   { return m_url; }
        void        setUrl( const QString &u )    { m_url.setPath( u ); }
        int         trackCount()                  { return m_trackCount; }
        int         length()                      { return m_length; }
        bool        isDynamic()                  { return m_dynamic; }
        bool        isLoaded()                    { return m_loaded; }
        bool        isModified()                  { return m_modified; }

        void        setDynamic( bool );
        void        setLoadingPix( QPixmap *pix ) { m_loadingPix = pix; repaint();}
        void        setModified( bool );

        int         compare( QListViewItem* i, int col ) const; //reimpl.
        KURL::List  tracksURL();    //returns the list of tracks url
        void        insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map );
        void        removeTrack( QListViewItem *item );

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


class StreamEntry : public PlaylistBrowserEntry
{
    public:
        StreamEntry( QListViewItem *parent, QListViewItem *after, const KURL &, const QString &t );
        StreamEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition );
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
        StreamEditor( QWidget *parent, QString title, QString url, const char *name=0 );

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
        SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QDomElement xmlDefinition );

        bool isEditable() const { return !m_xml.isNull(); }

        QString query() { return sqlForTags; }
        QString title() { return m_title; }

        QString sqlForTags;

        QDomElement xml() { return m_xml; }
        void setXml( QDomElement xml );

        bool  isDynamic() { return m_dynamic; }
        void  setDynamic( bool );

        int   rtti() const { return RTTI; }
        static const int RTTI = 1004;    //smart playlist item

    private:
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

