//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H


#include <qthread.h>    //baseclass
#include <qevent.h>     //baseclass
#include <kurl.h>       //KURL::List
#include <kfileitem.h>  //need the enum

class QListView;
class QListViewItem;
class QWidget;
class QTextStream;
class PlaylistItem;
class MetaBundle;

//TODO this is a temporary measure until the new FileBrowser is a bit more finished
//I'm doing it because I miss fast directory entry, but simply omit the definition
//to restore sorting, I think it sorts alphabetically only by default currently
#define FAST_TRANSLATE

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List&, QListView*, QListViewItem* );
    ~PlaylistLoader();

    void setOptions( bool b1, bool b2, int i ) { options.recurse = b1; options.symlink = b2; options.sortSpec = i; }

    class SomeUrlEvent;
    friend class PlaylistLoader::SomeUrlEvent;

private:
    PlaylistLoader( const KURL::List&, QListViewItem* ); //private constructor for placeholder

    void run();
    void process( const KURL::List &, bool = true );

    void postDownloadRequest( const KURL& );
    void postBundle( const KURL& );
    void postBundle( const KURL&, const QString&, const int );

    bool isValidMedia( const KURL &, mode_t = KFileItem::Unknown, mode_t = KFileItem::Unknown ) const;
#ifdef FAST_TRANSLATE
    void translate( QString & ); //recursively gets urls from a directory
#else
    void translate( QString &, KFileItemList & ); //turns a directory into a KURL::List
#endif
    int  isPlaylist( const QString & ) const;
    void loadLocalPlaylist( const QString &, int );
    KURL::List loadM3u( QTextStream &, const QString & );
    void loadPls( QTextStream & );

    KURL::List     m_list;
    QListViewItem *m_after;  //accessed by GUI thread _only_
    QListViewItem *m_first;
    QListView     *m_listView;
    int            m_recursionCount;

public:
    struct Options {
        bool recurse;
        bool symlink;
        int  sortSpec;
    } options;


    enum EventType { Started = 1000, SomeUrl = 1001, PlaylistFound = 1002, Done = 1010 };

    class SomeUrlEvent: public QCustomEvent
    {
    public:
       SomeUrlEvent( PlaylistLoader *pl, const KURL &u, const QString &s, int i )
         : QCustomEvent( SomeUrl )
         , m_thread( pl )
         , m_url( u )
         , m_title( s )
         , m_length ( i )
         , m_kio( false ) {}

       //TODO attempt to more clearly define the downloading version of this event
       SomeUrlEvent( PlaylistLoader *pl, const KURL &u )
         : QCustomEvent( SomeUrl )
         , m_thread( pl )
         , m_url( u )
         , m_length( 0 )
         , m_kio( true ) {}

       ~SomeUrlEvent();

       //const KURL &url() const { return m_url; }
       PlaylistItem *makePlaylistItem( QListView* );

    private:
       PlaylistLoader* const m_thread;
       const KURL m_url;
       QString    m_title;
       int        m_length;
       const bool m_kio;
    };

    class DoneEvent : public QCustomEvent
    {
    public:
       DoneEvent( PlaylistLoader *t ) : QCustomEvent( Done ), m_thread( t ) {}
       ~DoneEvent() { if( m_thread->wait( 2 ) ) delete m_thread; } //TODO better handling
    private:
       PlaylistLoader *m_thread;
    };

    class PlaylistFoundEvent : public QCustomEvent
    {
    public:
       PlaylistFoundEvent( KURL::List &l ) : QCustomEvent( PlaylistFound ), m_list( l ) {}
       PlaylistFoundEvent( KURL &u ) : QCustomEvent( PlaylistFound ), m_list( u ) {}
       const KURL::List &playlist() const { return m_list; }
    private:
       KURL::List m_list;
    };
};




#include <qvaluelist.h> //m_Q
#include <qmutex.h>     //stack allocated

#include <kdebug.h>     //FIXME
class ThreadWeaver : public QThread
{
public:
   ThreadWeaver( QWidget *w ) : m_parent( w ), m_bool( true ) {}

   class Job;

   void append( Job* const, bool = false );
   bool remove( Job* const ); //TODO implement virtual operator== ?
   void cancel();
   void halt() { m_bool = false; } //thread-safe, permanant shutdown

   //TODO guarentee done is sent before started
   enum EventType { Started = 2000, Done = 2001 };


   class Job : public QCustomEvent
   {
   public:
      friend class ThreadWeaver;
      enum JobType { TagReader = 3000, AudioPropertiesReader, TagWriter, PLStats };

      Job( QObject *obj, JobType type )
        : QCustomEvent( type )
        , m_target( obj )
      {}
      virtual ~Job() {}

   protected:
      Job();
      Job( const Job& );
      bool operator==( const Job &j ) const;

   private:
      virtual bool doJob() = 0;
      void postJob();

      QObject *m_target;
   };

private:
   void run();

   QWidget* const m_parent; //TODO can const this?
   bool           m_bool;
   QPtrList<Job>  m_Q;

   QMutex mutex;
};



class MetaBundle;

class TagReader : public ThreadWeaver::Job
{
public:
    TagReader( QObject*, PlaylistItem* );
    ~TagReader();

    bool doJob();
    static MetaBundle* readTags( const KURL&, bool = false );

    void bindTags();
    void addSearchTokens( QStringList&, QPtrList<QListViewItem>& );

private:
    PlaylistItem* const m_item;
    const KURL  m_url;
    MetaBundle* m_tags;
};


class AudioPropertiesReader : public ThreadWeaver::Job
{
public:
    AudioPropertiesReader( QObject*, PlaylistItem* );
    bool doJob();
    void bindTags();
private:
    PlaylistItem* const m_item;
    QListView*    const m_listView;
    const KURL m_url;
    QString    m_length;
    QString    m_bitrate;
};


class TagWriter : public ThreadWeaver::Job
{
public:
    TagWriter( QObject*, PlaylistItem*, const QString&, const int );

    bool doJob();
    void updatePlaylistItem();

private:
    PlaylistItem* const m_item;
    const QString m_tagString;
    const int     m_tagType;
};

#endif
