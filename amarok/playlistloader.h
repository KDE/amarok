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
#include <kurl.h>      //need KURL::List
#include <kfileitem.h> //need the enum

class QListView;
class QListViewItem;
class QWidget;
class QTextStream;
class QMutex;
class PlaylistItem;
class MetaBundle;


class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List &, QWidget *, PlaylistItem *, bool =false );
    ~PlaylistLoader();

    struct Options
    {
      bool recurse;
      bool symlink;
      bool meta;
    } options;

    void setOptions( bool b1, bool b2, bool b3 ) { options.recurse = b1;
                                                   options.symlink = b2;
                                                   options.meta    = b3; }

    class LoaderEvent : public QCustomEvent
    {
    public:
       LoaderEvent( PlaylistLoader *pl, const KURL &u, MetaBundle* const mb ) : QCustomEvent( 65432 ), m_thread( pl ), m_url( u ), m_tags( mb ), m_kio( false ) {}
       LoaderEvent( PlaylistLoader *pl, const KURL &u ) : QCustomEvent( 65432 ), m_thread( pl ), m_url( u ), m_tags( 0 ), m_kio( true ) {}

       //TODO attempt to more clearly define the downloading version of this event 

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
       LoaderDoneEvent( PlaylistLoader *t ) : QCustomEvent( 65433 ), m_thread( t ) {}
       void dispose() { if( m_thread->running() ) m_thread->wait(); delete m_thread; } //FIXME will stall UI

    private:
       PlaylistLoader *m_thread;
    };

    friend class PlaylistEvent;

private:
    void run();
    void process( KURL::List &, bool = true );

    bool isValidMedia( const KURL &, mode_t = KFileItem::Unknown, mode_t = KFileItem::Unknown );
    void translate( QString &, KURL::List & );
    int  isPlaylist( const QString & );
    void loadLocalPlaylist( const QString &, int );
    void loadM3u( QTextStream &, const QString & );
    void loadPls( QTextStream & );

    KURL::List     m_list;
    QWidget       *m_parent;
    QListViewItem *m_after;  //accessed by GUI thread _only_
    PlaylistItem  *m_first;
};


#include <deque> //stack allocated

class TagReader : public QThread
{
public:
   TagReader( QWidget *w ) : m_parent( w ), m_bool( true ) {}

   void append( PlaylistItem * );
   void remove( PlaylistItem * );
   void cancel();
   void halt() { if( running() ) m_bool = false; } //thread-safe shutdown

   //FIXME rename dumbarse!
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

private:
   void run();
   MetaBundle *readTags( const KURL &url, MetaBundle *tags );

   struct Bundle
   {
      //FIXME you want const data members, but then you can't do assignment, and std::remove() needs assignment!
      //      try copy constructor
      Bundle( PlaylistItem *pi, const KURL &u, MetaBundle* mb ) : item( pi ), url( u ), tags( mb ) {}

      bool operator==( const Bundle &b ) const { return ( item == b.item ); }
      bool operator==( const PlaylistItem* const pi ) const { return ( item == pi ); }

      PlaylistItem* item;
      KURL url;
      MetaBundle* tags;
   };

   QWidget *m_parent;
   std::deque<Bundle> m_Q;
   QMutex mutex;
   bool m_bool;
};

#endif
