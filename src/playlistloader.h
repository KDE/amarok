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
class PlaylistItem;

namespace KIO { class Job; }


class PlaylistLoader : public ThreadWeaver::DependentJob
{
public:
    PlaylistLoader( QObject *dependent, const KURL::List&, QListViewItem*, bool playFirstUrl = false );
   ~PlaylistLoader();

    enum Format { M3U, PLS, XML, UNKNOWN };
    enum EventType { Item = 3000, DomItem };

    static bool isPlaylist( const KURL& );        //inlined
    static Format playlistType( const QString& ); //inlined

protected:
    virtual bool doJob();
    virtual void completeJob();
    virtual void customEvent( QCustomEvent* );
    virtual void postItem( const KURL&, const QString &title, const uint length );

    bool loadPlaylist( const QString& ); //inlined
    bool loadPlaylist( const QString&, Format );

private:
    bool recurse( const KURL&, bool recursing = false );

private:
    KURL::List m_badURLs;
    KURL::List m_URLs;

    KDirLister   *m_dirLister;
    PlaylistItem *m_markerListViewItem;

    bool m_playFirstUrl;

protected:
    PlaylistLoader( const PlaylistLoader& ); //undefined
    PlaylistLoader &operator=( const PlaylistLoader& ); //undefined
};



class RemotePlaylistFetcher : public QObject {
    Q_OBJECT

    const KURL m_source;
    KURL m_destination;
    QListViewItem *m_after;

public:
    RemotePlaylistFetcher( const KURL &source, QListViewItem *after, QObject *playlist );

private slots:
    void result( KIO::Job* );
    void abort() { delete this; }
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
