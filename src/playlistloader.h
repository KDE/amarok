// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include <kurl.h>         //KURL::List
#include "metabundle.h"   //stack allocated
#include <qdom.h>         //stack allocated
#include <qevent.h>       //baseclass
#include "threadweaver.h" //baseclass


class CollectionDB;
class QListView;
class QListViewItem;
class KDirLister;

class PlaylistLoader : public ThreadWeaver::DependentJob
{
public:
    PlaylistLoader( QObject *dependent, const KURL::List&, QListViewItem*, bool playFirstUrl = false );
   ~PlaylistLoader();

    enum Format { M3U, PLS, XML, UNKNOWN };
    enum EventType { Item = 3000, DomItem };

    static bool isPlaylist( const KURL& );        //inlined
    static Format playlistType( const QString& ); //inlined

    class ItemEvent : public QCustomEvent {
    public:
        ItemEvent( const MetaBundle &bundle, QListViewItem *item, bool play = false )
            : QCustomEvent( Item )
            , bundle( bundle )
            , beforeThisItem( item )
            , playThisUrl( play )
        {}

        MetaBundle bundle;
        QListViewItem *beforeThisItem;
        bool playThisUrl;
    };

    class DomItemEvent : public QCustomEvent {
    public:
        DomItemEvent( const KURL &u, const QDomNode &n, QListViewItem *item, bool play = false )
            : QCustomEvent( DomItem )
            , url( u )
            , node( n )
            , beforeThisItem( item )
            , playThisUrl( play )
        {}

        KURL url;
        QDomNode node;
        QListViewItem *beforeThisItem;
        bool playThisUrl;
    };

protected:
    virtual bool doJob();
    virtual void completeJob();

    virtual void postItem( const KURL&, const QString&, const uint );

    bool loadPlaylist( const QString& ); //inlined
    bool loadPlaylist( const QString&, Format );

private:
    bool recurse( const KURL&, bool recursing = false );
    void postItem( const KURL& );

private:
    KURL::List m_badURLs;
    KURL::List m_fileURLs;

    KDirLister *m_dirLister;

    class PlaylistItem *m_markerListViewItem;
    bool m_playFirstUrl;

protected:
    PlaylistLoader( const PlaylistLoader& ); //undefined
    PlaylistLoader &operator=( const PlaylistLoader& ); //undefined
};


class RemotePlaylistFetcher : public QObject {
    Q_OBJECT

    const KURL m_source;
    KURL m_destination;

public:
    RemotePlaylistFetcher( QObject *parent, const KURL &source );

private slots:
    void result( KIO::Job* );
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
