//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <qthread.h>
#include <qevent.h>
#include <kurl.h>


class QListView;
class QListViewItem;
class PlaylistItem;
class QWidget;
class QTextStream;
class QString;
struct Tags;


#include <kdebug.h> //FIXME remove at some point, only used for member definitions

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List &ul, QWidget *w, QListViewItem *lvi, bool b=false ) : m_list( ul ), m_parent( w ), m_after( lvi ), m_first( 0 ) { if( b ) m_first = m_after; kdDebug() << "[LOADER] Creating thread..\n"; }
    ~PlaylistLoader() { kdDebug() << "[LOADER] Deleting thread..\n"; }

    void run();

    class PlaylistEvent : public QCustomEvent
    {
    public:
       PlaylistEvent( QListViewItem **lvi, const KURL &u, Tags* const t ) : QCustomEvent( 65432 ), m_after( lvi ), m_url( u ), m_tags( t ), m_kio( false ) {}
       PlaylistEvent( QListViewItem **lvi, const KURL &u ) : QCustomEvent( 65432 ), m_after( lvi ), m_url( u ), m_tags( 0 ), m_kio( true ) {}

       const KURL &url() const { return m_url; }
       PlaylistItem *makePlaylistItem( QListView *lv );

    private:
       QListViewItem** const m_after;
       const KURL m_url;
       Tags* const m_tags;
       const bool m_kio;
    };

    friend class PlaylistEvent;

private:
    void process( KURL::List &, bool = true );

    bool isValidMedia( const KURL & );
    void translate( const KURL &, KURL::List & );
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

#endif
