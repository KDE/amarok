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

// Simple subclass for categories/folders and other structures for organising data

class PlaylistCategory : public KListViewItem
{
    public:
        PlaylistCategory( KListView *parent, QListViewItem *after, const QString &, bool isFolder=false );
        PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &, bool isFolder=true );
        ~PlaylistCategory() { };

        const QString &title() const { return m_title; }
        bool  isFolder() { return m_folder; }
        uint  folderCount() { return m_folderCount; }
        void  setFolderCount( uint i ) { m_folderCount = i; }

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        int   rtti() const { return RTTI; }
        static const int RTTI = 1000;    //category item

    private:
        QString m_title;
        bool    m_folder;
        uint    m_folderCount;

};


class PlaylistEntry :  public QObject, public KListViewItem
{
        Q_OBJECT

    friend class PlaylistTrackItem;
    friend class TrackItemInfo;
    friend class PlaylistCategory;

    public:
        PlaylistEntry( KListViewItem *parent, QListViewItem *after, const KURL &, int tracks=0, int length=0 );
        ~PlaylistEntry();
        void load();
        void restore();

        const KURL &url() const                   { return m_url; }
        void        setUrl( const QString &u )    { m_url.setPath( u ); }
        int         trackCount()                  { return m_trackCount; }
        int         length()                      { return m_length; }
        bool        isModified()                  { return m_modified; }
        void        setModified( bool );
        void        setLoadingPix( QPixmap *pix ) { m_loadingPix = pix; repaint();}

        int         compare( QListViewItem* i, int col ) const; //reimpl.
        KURL::List  tracksURL();    //returns the list of tracks url
        void        insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map );
        void        removeTrack( QListViewItem *item );

        QPtrList<TrackItemInfo> trackList()       { return m_trackList; }    //returns the list of tracks information
        QPtrList<TrackItemInfo> droppedTracks()   { return tmp_droppedTracks; }

        void  setOpen( bool );
        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        //rtti is used to distinguish different kinds of list view items
        int   rtti() const { return RTTI; }
        static const int RTTI = 1001;    //playlist item

    signals:
        void startingLoading();
        void loaded();

    private:
        void customEvent( QCustomEvent* e );

        KURL                 m_url;  //playlist url
        int                  m_length;    //total length in seconds
        int                  m_trackCount;    //track counter
        QPtrList<TrackItemInfo> m_trackList;    //tracks in playlist
        QPtrList<TrackItemInfo> tmp_droppedTracks;    //tracks dropped to the playlist while it wasn't been loaded
        bool                 m_loading;
        bool                 m_loaded;    //playlist loaded
        bool                 m_modified;    //the playlist has been modified
        QPixmap             *m_savePix;
        QPixmap             *m_loadingPix;
        PlaylistTrackItem   *m_lastTrack;
};

// For saving the current playlist
class PlaylistSaver : public KDialogBase
{
    public:
        PlaylistSaver( QString title, QWidget *parent, const char *name=0 );

        QString title() const { return m_nameLineEdit->text(); }

    private:
        KLineEdit *m_nameLineEdit;

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

class StreamEntry : public KListViewItem
{
    public:
        StreamEntry( KListViewItem *parent, QListViewItem *after, const KURL &, const QString &t );
        ~StreamEntry() { };

        void  setURL  ( KURL u )    { m_url = u; }
        void  setTitle( QString t ) { m_title = t; }

        void  setup();
        void  paintCell( QPainter*, const QColorGroup&, int, int, int );

        const KURL &url()      { return m_url; }
        const QString &title() { return m_title; }

        int   rtti() const { return RTTI; }
        static const int RTTI = 1003;    //stream item

    private:
        QString m_title;
        KURL    m_url;

};

class StreamEditor : public KDialogBase
{
    public:
        StreamEditor( QString name, QWidget *parent, const char *name=0 );
        StreamEditor( QWidget *parent, QString title, QString url, const char *name=0 );

        KURL    url()  const { return KURL::KURL( m_urlLineEdit->text() ); }
        QString name() const { return m_nameLineEdit->text(); }

    private:
        KLineEdit *m_urlLineEdit;
        KLineEdit *m_nameLineEdit;

};

class SmartPlaylist : public KListViewItem
{
    public:
        SmartPlaylist( KListViewItem *parent, QListViewItem *after, const QString &name, const QString &query );
        SmartPlaylist( KListViewItem *parent, QListViewItem *after, const QString &name,
                                                        const QString &urls, const QString &tags );

        void setCustom( bool b ) { m_custom = b; setDragEnabled( true ); }
        bool isCustom() const { return m_custom; }

        QString query() { return isCustom() ? sqlForUrls : sqlForTags; }
        QString title() { return m_title; }

        KURL::List urlList() const;

        QString sqlForUrls;
        QString sqlForTags;

        int   rtti() const { return RTTI; }
        static const int RTTI = 1004;    //smart playlist item

    private:
        bool m_custom;
        QString m_title;
};

#endif

