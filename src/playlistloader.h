// Author: Max Howell (C) Copyright 2003
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include "metabundle.h"   //TagsEvent

#include <qdom.h>         //stack alloc
#include <qevent.h>       //baseclass
#include <qthread.h>      //baseclass
#include <kurl.h>         //KURL::List

class CollectionDB;
class QListView;
class QListViewItem;
class KDirLister;

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List&, QListView*, QListViewItem*, bool playFirstUrl = false );
    ~PlaylistLoader();

    enum Format { M3U, PLS, XML, UNKNOWN };
    enum EventType { Started = 1010, Done, Item, DomItem };

    static void stop() { s_stop = true; }
    static void downloadPlaylist( const KURL&, QListView*, QListViewItem*, bool directPlay = false );
    static bool isPlaylist( const KURL& );        //inlined
    static Format playlistType( const QString& ); //inlined

    class StartedEvent : public QCustomEvent
    {
    public:
        StartedEvent( QListViewItem* item, bool _directPlay )
            : QCustomEvent( Started )
            , afterItem( item )
            , directPlay( _directPlay )
        {}

        QListViewItem* const afterItem;
        bool directPlay;
    };

    class ItemEvent : public QCustomEvent
    {
    public:
        ItemEvent( const MetaBundle &bundle )
            : QCustomEvent( Item )
            , bundle( bundle )
        {}

        MetaBundle bundle;
    };

    class DomItemEvent : public QCustomEvent
    {
    public:
        DomItemEvent( const KURL& _url, const QDomNode& _node )
            : QCustomEvent( DomItem )
            , url( _url )
            , node( _node )
        {}

        KURL url;
        QDomNode node;
    };

    class DoneEvent : public QCustomEvent
    {
        PlaylistLoader *m_loader;
        KURL::List      m_badURLs;
    public:
        DoneEvent( PlaylistLoader *loader )
            : QCustomEvent( Done )
            , m_loader( loader )
            , m_badURLs( loader->m_badURLs )
        {}

        KURL::List &badURLs() { return m_badURLs; }

        ~DoneEvent() { delete m_loader; }
    };

    friend class DoneEvent;

protected:
    virtual void run();
    virtual void postItem( const KURL&, const QString&, const uint );
    virtual void postItem( const KURL&, const uint );

    bool loadPlaylist( const QString& ); //inlined
    bool loadPlaylist( const QString&, Format );

private:
    bool recurse( const KURL&, bool recursing = false );
    void postItem( const KURL& );
    void addBadURL( const KURL &url ) { m_badURLs += url; }

private:
    static bool s_stop;

    const KURL::List m_URLs;
          KURL::List m_badURLs;

    KURL::List m_fileURLs;

    QListViewItem *m_afterItem;
    bool m_playFirstUrl;
    CollectionDB* m_db;
    KDirLister* m_dirLister;

protected:
    PlaylistLoader( const PlaylistLoader& ); //undefined
    PlaylistLoader &operator=( const PlaylistLoader& ); //undefined
};


inline bool
PlaylistLoader::isPlaylist( const KURL &url )
{
    return playlistType( url.path() ) != UNKNOWN;
}

inline PlaylistLoader::Format
PlaylistLoader::playlistType( const QString &path )
{
    const QString ext = path.right( 4 ).lower();

         if( ext == ".m3u" ) return M3U;
    else if( ext == ".pls" ) return PLS;
    else if( ext == ".xml" ) return XML;
    else return UNKNOWN;
}

inline bool
PlaylistLoader::loadPlaylist( const QString &path )
{
    Format type = playlistType( path );

    if ( type != UNKNOWN )
        //TODO add to bad list if it fails
        return loadPlaylist( path, type );

    return false;
}

#endif
