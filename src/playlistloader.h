// Author: Max Howell (C) Copyright 2003
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <kurl.h>         //KURL::List
#include "metabundle.h"   //TagsEvent
#include <qevent.h>       //baseclass
#include <qthread.h>      //baseclass

class QListView;
class QListViewItem;

class PlaylistLoader : public QThread
{
public:
    PlaylistLoader( const KURL::List&, QListView*, QListViewItem* );

    enum Format { M3U, PLS, XML, UNKNOWN };
    enum EventType { Started = 1010, Done, Item, Tags };

    static void stop() {} //TODO
    static void downloadPlaylist( const KURL&, QListView*, QListViewItem* );
    static bool isPlaylist( const KURL& );        //inlined
    static Format playlistType( const QString& ); //inlined

    class TagsEvent : public QCustomEvent
    {
    public:
        TagsEvent( const KURL &url, PlaylistItem *item )
            : QCustomEvent( PlaylistLoader::Tags )
            , item( item )
            , bundle( url )
        {}

        PlaylistItem* const item;
        MetaBundle bundle;
    };

    class DoneEvent : public QCustomEvent
    {
        PlaylistLoader *m_loader;
    public:
        DoneEvent( PlaylistLoader *loader )
            : QCustomEvent( Done )
            , m_loader( loader )
        {}

        ~DoneEvent() { delete m_loader; }
    };

protected:
    virtual void createPlaylistItem( const KURL&, const QString&, const uint );
    virtual void run();

    bool loadPlaylist( const QString& ); //inlined
    bool loadPlaylist( const QString&, Format );

private:
    void recurse( QString );
    void createPlaylistItem( const KURL& );

private:
    PlaylistItem *m_markey;

    const KURL::List m_URLs;
          KURL::List m_brokenURLs;

    typedef QPair<KURL,PlaylistItem*> Pair;
    typedef QValueList<Pair> List;
    List m_pairs;

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
