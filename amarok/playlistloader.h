//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <qthread.h>   //baseclass
#include <qmutex.h>    //stack allocated
#include <qevent.h>    //baseclass
#include <qvaluelist.h>
#include <kurl.h>      //need KURL::List
#include <kfileitem.h> //need the enum

class PlayerApp;
extern PlayerApp *pApp;

class QListView;
class QListViewItem;
class QWidget;
class QTextStream;
class QMutex;
class PlaylistItem;
class MetaBundle;
//class KFileItemList;


class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List &, QWidget *, PlaylistItem *, bool =false );
    ~PlaylistLoader();

    struct Options {
        bool recurse;
        bool symlink;
        int  sortSpec;
    } options;

    void setOptions( bool b1, bool b2, int i ) { options.recurse = b1; options.symlink = b2; options.sortSpec = i; }

    ////////CUSTOMEVENTS
    
    class LoaderEvent : public QCustomEvent
    {
    public:
       LoaderEvent( PlaylistLoader *pl, const KURL &u, MetaBundle* const mb )
         : QCustomEvent( 65432 )
         , m_thread( pl )
         , m_url( u )
         , m_tags( mb )
         , m_kio( false ) {}

       //TODO attempt to more clearly define the downloading version of this event                
       LoaderEvent( PlaylistLoader *pl, const KURL &u )
         : QCustomEvent( 65432 )
         , m_thread( pl )
         , m_url( u )
         , m_tags( 0 )
         , m_kio( true ) {}

       const KURL &url() const { return m_url; }
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
       LoaderDoneEvent( PlaylistLoader *t )
         : QCustomEvent( 65433 )
         , m_thread( t ) {}
         
       void dispose() { if( m_thread->running() ) m_thread->wait(); delete m_thread; } //FIXME will stall UI

    private:
       PlaylistLoader *m_thread;
    };

    ////////////////////

    friend class LoaderEvent;

private:
    virtual void run();
    void process( const KURL::List &, bool = true );

    bool isValidMedia( const KURL &, mode_t = KFileItem::Unknown, mode_t = KFileItem::Unknown );
    void translate( QString &, KFileItemList & ); //turns a directory into a KURL::List
    int  isPlaylist( const QString & );
    void loadLocalPlaylist( const QString &, int );
    void loadM3u( QTextStream &, const QString & );
    void loadPls( QTextStream & );

    KURL::List     m_list;
    QWidget       *m_parent;
    QListViewItem *m_after;  //accessed by GUI thread _only_
    PlaylistItem  *m_first;
    int            m_recursionCount;
};


class TagReader : public QThread
{
public:
   TagReader( QWidget *w ) : m_parent( w ), m_bool( true ) {}

   void append( PlaylistItem * );
   void remove( PlaylistItem * );
   void cancel();
   void halt() { m_bool = false; } //thread-safe, permanant shutdown

   class TagReaderEvent : public QCustomEvent
   {
   public:
      TagReaderEvent( PlaylistItem* const pi, MetaBundle* const t )
        : QCustomEvent( 65434 )
        , m_item( pi )
        , m_tags( t )
      {}
      ~TagReaderEvent();

      void bindTags();

   private:
      PlaylistItem* const m_item;
      MetaBundle* const m_tags;
   };
   
   //does nothing this one, just used by playlistWidget to find out when tagReader has finished
   class TagReaderDoneEvent : public QCustomEvent
   {
   public:
      TagReaderDoneEvent() : QCustomEvent( 65435 ) {}
   };


private:
   virtual void run();
   MetaBundle *readTags( const KURL &url );

   // should qualify for a QValueList value (i.e. have a. copy ctor, b. default ctor, c. assignment operator)
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
