//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <qthread.h>
#include <qmutex.h>
#include <qevent.h>
#include <kurl.h>      //need KURL::List
#include <kfileitem.h> //need the enum


class QListView;
class QListViewItem;
class QWidget;
class QTextStream;
class QMutex;
class PlaylistItem;
struct Tags;


#include <kdebug.h> //FIXME remove at some point, only used for member definitions

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List &ul, QWidget *w, QListViewItem *lvi, bool b=false ) : m_list( ul ), m_parent( w ), m_after( lvi ), m_first( b ? m_after : 0 ) {}
    ~PlaylistLoader() { kdDebug() << "[loader] Done!\n"; delete m_first; } //FIXME deleting m_first is dangerous as user may have done it for us!

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
       LoaderEvent( PlaylistLoader *pl, const KURL &u, Tags* const t ) : QCustomEvent( 65432 ), m_thread( pl ), m_url( u ), m_tags( t ), m_kio( false ) {}
       LoaderEvent( PlaylistLoader *pl, const KURL &u ) : QCustomEvent( 65432 ), m_thread( pl ), m_url( u ), m_tags( 0 ), m_kio( true ) {}

       //TODO attempt to more clearly define the downloading version of this event 

       const KURL &url() const { return m_url; }
       PlaylistItem *makePlaylistItem( QListView *lv );

    private:
       PlaylistLoader* const m_thread;
       const KURL m_url;
       Tags* const m_tags;
       const bool m_kio;
    };

    class LoaderDoneEvent : public QCustomEvent
    {
    public:
       LoaderDoneEvent( QThread *t ) : QCustomEvent( 65433 ), m_thread( t ) {}
       void dispose() { if( m_thread->running() ) m_thread->wait(); delete m_thread; } //FIXME will stall UI

    private:
       QThread *m_thread;
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
    //this is accessed by the GUI thread, but crucially, it isn't accessed by any other thread
    //and thus access is serialised.
    QListViewItem *m_after;
    QListViewItem *m_first;
};


#include <deque>

class TagReader : public QThread
{
public:
   TagReader( QWidget *w ) : m_parent( w ), m_bool( true ) {}

   void append( PlaylistItem * );
   void remove( PlaylistItem * );
   void cancel();
   void halt() { if( running() ) m_bool = false; } //thread-safe shutdown

   class TagReaderEvent : public QCustomEvent
   {
   public:
      TagReaderEvent( PlaylistItem* const pi, Tags* const t ) : QCustomEvent( 65434 ), m_item( pi ), m_tags( t ) {}

      void bindTags();

   private:
      PlaylistItem* const m_item;
      Tags* const m_tags;
   };

private:
   void run();
   Tags *readTags( const KURL &url, Tags *tags );

   struct Bundle
   {
      //FIXME you want const data members, but then you can't do assignment, and std::remove() needs assignment!
      Bundle( PlaylistItem *pi, const KURL &u, Tags* t ) : item( pi ), url( u ), tags( t ) {}

      bool operator==( const Bundle &b ) const { return ( item == b.item ); }
      bool operator==( const PlaylistItem* const pi ) const { return ( item == pi ); }

      PlaylistItem* item;
      KURL url;
      Tags* tags;
   };

   QWidget *m_parent;
   std::deque<Bundle> m_Q;
   QMutex mutex;
   bool m_bool;
};

#endif
