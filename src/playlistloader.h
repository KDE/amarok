//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//


//Hi, this class is messy. Sorry about that.


#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <qthread.h>    //baseclass
#include <kurl.h>       //KURL::List
#include <kfileitem.h>  //need the enum

class QListViewItem;
class QWidget;
class QTextStream;
class PlaylistItem;
class Playlist;

//TODO this is a temporary measure until the new FileBrowser is a bit more finished
//I'm doing it because I miss fast directory entry, but simply omit the definition
//to restore sorting, I think it sorts alphabetically only by default currently
#define FAST_TRANSLATE

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List&, QObject *, PlaylistItem* );
    PlaylistLoader( const KURL::List&, QObject * );
    ~PlaylistLoader();

    void setOptions( bool b1, bool b2, int i ) { options.recurse = b1; options.playFirstItem = b2; options.sortSpec = i; }

    static bool isValidMedia( const KURL &, mode_t = KFileItem::Unknown, mode_t = KFileItem::Unknown );
    static inline int isPlaylist( const QString & );
    static void stop() { m_stop = true; }
    
private:
    PlaylistLoader( const KURL::List&, PlaylistItem* ); //private constructor for placeholder

    void run();
    void process( const KURL::List &, const bool = true );

    void postDownloadRequest( const KURL& );
    void postBundle( const KURL& );
    void postBundle( const KURL&, const QString&, const int );

#ifdef FAST_TRANSLATE
    void translate( QString&, const QCString& ); //recursively gets urls from a directory
#else
    void translate( QString &, KFileItemList & ); //turns a directory into a KURL::List
#endif
    void loadLocalPlaylist( const QString &, int );
    void loadM3U( QTextStream &, const QString & );
    void loadPLS( QTextStream & );
    void loadXML( QTextStream & );

    static bool m_stop;
    
    KURL::List      m_list;
    PlaylistItem   *m_after;  //accessed by GUI thread _only_
    PlaylistItem   *m_first;
    Playlist       *m_listView;
    int             m_recursionCount;
    QObject *m_receiver;
    
public:
    struct Options {
        Options() : recurse( true ), playFirstItem( false ), symlink( true ), sortSpec( 0 ) {} //options suitable for m3u files
        bool recurse;
        bool playFirstItem;
        bool symlink;
        int  sortSpec;
    } options;

    enum EventType { Started = 1000, MakeItem = 1001, PlaylistFound = 1002, Done = 1010 };

    class MakeItemEvent: public QCustomEvent
    {
    public:
       MakeItemEvent( PlaylistLoader *pl, const KURL &u, const QString &s, const int i )
         : QCustomEvent( MakeItem )
         , m_thread( pl )
         , m_url( u )
         , m_title( s )
         , m_length( i )
       {}
       const KURL &url() { return m_url; }
       const QString &title() { return m_title; }
       const int length() { return m_length; }
       virtual PlaylistItem *makePlaylistItem( Playlist* );
       bool playMe() { bool b = m_thread->options.playFirstItem; if( b ) m_thread->options.playFirstItem = false; return b; }
    protected:
       PlaylistLoader* const m_thread;
       const KURL m_url;
       const QString m_title;
       const int m_length;
    };
    
    class DownloadPlaylistEvent : public MakeItemEvent
    {
    public:
        DownloadPlaylistEvent( PlaylistLoader *pl, const KURL &u )
          : MakeItemEvent( pl, u, "Retrieving Playlist..", -2 )
        {}
        PlaylistItem *makePlaylistItem( Playlist* );
    };

    class PlaylistFoundEvent : public QCustomEvent
    {
    public:
       PlaylistFoundEvent( const QString &s, const KURL::List &l ) : QCustomEvent( PlaylistFound ), m_path( s ), m_list( l ) {}
       const KURL url() const { KURL url; url.setPath( m_path ); return url; }
       const KURL::List &contents() const { return m_list; }
    private:
       QString    m_path;
       KURL::List m_list;
    };

    class DoneEvent : public QCustomEvent
    {
    public:
       DoneEvent( PlaylistLoader *t ) : QCustomEvent( Done ), m_thread( t ) {}
       ~DoneEvent() { if( m_thread->wait( 2 ) ) delete m_thread; } //TODO better handling
    private:
       PlaylistLoader *m_thread;
    };

    class ProgressEvent : public QCustomEvent
    {
        public:
            enum State { Start = -1, Stop = -2, Total = -3, Progress = -4 };
    
            ProgressEvent( int state, int value = -1 )
            : QCustomEvent( 60000 )
            , m_state( state )
            , m_value( value ) {}
            int state() { return m_state; }
            int value() { return m_value; }
        private:
            int m_state;
            int m_value;
    };

    friend class MakeItemEvent; //for access to m_after
    friend class DownloadPlaylistEvent; //for access to placeHolder ctor
};

#endif
