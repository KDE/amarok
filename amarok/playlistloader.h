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

//TODO this is a temporary measure until the new FileBrowser is a bit more finished
//I'm doing it because I miss fast directory entry, but simply omit the definition
//to restore sorting, I think it sorts alphabetically only by default currently
//#define FAST_TRANSLATE

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List&, QListView*, QListViewItem* );
    ~PlaylistLoader();

    void setOptions( bool b1, bool b2, int i ) { options.recurse = b1; options.symlink = b2; options.sortSpec = i; }

    enum EventType { Started = 1000, SomeURL = 1001, Done = 1002 };
    friend class LoaderEvent;


    struct Options {
        bool recurse;
        bool symlink;
        int  sortSpec;
    } options;


    class LoaderEvent : public QCustomEvent
    {
    public:
       LoaderEvent( PlaylistLoader *pl, const KURL &u, MetaBundle* const mb )
         : QCustomEvent( SomeURL )
         , m_thread( pl )
         , m_url( u )
         , m_tags( mb )
         , m_kio( false ) {}

       //TODO attempt to more clearly define the downloading version of this event
       LoaderEvent( PlaylistLoader *pl, const KURL &u )
         : QCustomEvent( SomeURL )
         , m_thread( pl )
         , m_url( u )
         , m_tags( 0 )
         , m_kio( true ) {}

       ~LoaderEvent();

       //const KURL &url() const { return m_url; }
       PlaylistItem *makePlaylistItem( QListView *lv );

    private:
       PlaylistLoader* const m_thread;
       const KURL m_url;
       MetaBundle* const m_tags;
       const bool m_kio;
    };


    class LoaderDoneEvent : public QCustomEvent
    {
    public:
       LoaderDoneEvent( PlaylistLoader *t ) : QCustomEvent( Done ), m_thread( t ) {}
       ~LoaderDoneEvent() { if( m_thread->wait( 2 ) ) delete m_thread; } //TODO better handling
    private:
       PlaylistLoader *m_thread;
    };

private:
    PlaylistLoader( const KURL::List&, QListViewItem* ); //private constructor for placeholder

    void run();
    void process( const KURL::List &, bool = true );

    void postDownloadRequest( const KURL& );
    void postBundle( const KURL&, MetaBundle* = 0 );

    bool isValidMedia( const KURL &, mode_t = KFileItem::Unknown, mode_t = KFileItem::Unknown ) const;
#ifdef FAST_TRANSLATE
    void translate( QString & ); //recursively gets urls from a directory
#else
    void translate( QString &, KFileItemList & ); //turns a directory into a KURL::List
#endif
    int  isPlaylist( const QString & ) const;
    void loadLocalPlaylist( const QString &, int );
    void loadM3u( QTextStream &, const QString & );
    void loadPls( QTextStream & );

    KURL::List     m_list;
    QListViewItem *m_after;  //accessed by GUI thread _only_
    QListViewItem *m_first;
    QListView     *m_listView;
    int            m_recursionCount;
};




#include <qvaluelist.h> //m_Q
#include <qmutex.h>     //stack allocated

class MetaBundle;

class TagReader : public QThread
{
public:
   TagReader( QWidget *w ) : m_parent( w ), m_bool( true ) {}

   void append( PlaylistItem * );
   void remove( PlaylistItem * );
   void cancel();
   void halt() { m_bool = false; } //thread-safe, permanant shutdown

   static MetaBundle *readTags( const KURL &url );

   enum EventType { Started = 2000, SomeTags = 2001, Done = 2002 };


   class TagReaderEvent : public QCustomEvent
   {
   public:
      TagReaderEvent( PlaylistItem* const pi, MetaBundle* const t )
        : QCustomEvent( SomeTags )
        , m_item( pi )
        , m_tags( t )
      {}
      ~TagReaderEvent();

      void bindTags();
      void addSearchTokens( QStringList&, QPtrList<QListViewItem>& );

   private:
      PlaylistItem* const m_item;
      MetaBundle*   const m_tags;
   };

private:
   void run();

   //should qualify for a QValueList value (i.e. have a. copy ctor, b. default ctor, c. assignment operator)
   //TODO this is no longer required as long as it is safe to derefernce playlistItem for: url() const
   struct Bundle
   {
      Bundle() : item(0), url() {}
      Bundle( PlaylistItem *pi, const KURL &u ) : item( pi ), url( u ) {}
      Bundle( const Bundle &b ) : item( b.item ), url( b.url ) {}

      Bundle& operator=( const Bundle &b )
      {
          if (&b != this)
          {
              //nothing to delete: never delete the PlaylistItem! GUI thread only can do that
              this->item = b.item;
              this->url = b.url;
          }
          return *this;
      }
      bool operator==( const Bundle &b ) const { return ( item == b.item ); }
      bool operator==( const PlaylistItem* const pi ) const { return ( item == pi ); }

      PlaylistItem* item;
      KURL url;
   };

   QWidget *m_parent;
   QValueList<Bundle> m_Q;
   QMutex mutex;
   bool m_bool;
};

#endif
