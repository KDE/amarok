// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include "debug.h"        //stack allocated
#include <kurl.h>         //KURL::List
#include "metabundle.h"   //stack allocated
#include "threadweaver.h" //baseclass

class QDomNode;
class QListViewItem;
class PlaylistItem;

namespace KIO { class Job; }


/**
 * @author Max Howell
 * @short Abstract base class for extracting data from _local_ Playlist files
 *
 * Reimplement postItem() and call loadPlaylist()
 *
 * TODO be able to load directories too, it's in the spec
 * TODO and playlists within playlists, remote and local
 */
class PlaylistFileTranslator
{
public:
    enum Format { M3U, PLS, XML, UNKNOWN };

    static inline bool isPlaylistFile( const KURL &url ) { return isPlaylistFile( url.fileName() ); }
    static inline bool isPlaylistFile( const QString &fileName ) { return playlistType( fileName ) != UNKNOWN; }

    static inline Format playlistType( const QString &fileName );

protected:
    /// pure virtual, called for every entry
    virtual void postItem( const KURL&, const QString &title, uint length ) = 0;

    /// specially called for amaroK-XML playlists
    virtual void postXmlItem( const KURL&, const QDomNode& ) = 0;

//    virtual bool isAborted() const = 0;

    bool loadPlaylist( const QString &path );
    bool loadPlaylist( const QString &path, Format );
};

inline PlaylistFileTranslator::Format
PlaylistFileTranslator::playlistType( const QString &fileName )
{
    const QString ext = fileName.right( 4 ).lower();

    if( ext == ".m3u" ) return M3U;
    else if( ext == ".pls" ) return PLS;
    else if( ext == ".xml" ) return XML;
    else return UNKNOWN;
}

inline bool
PlaylistFileTranslator::loadPlaylist( const QString &path )
{
    Format type = playlistType( path );

    return type != UNKNOWN ? loadPlaylist( path, type ) : false;
}



/**
 * @author Max Howell
 * @author Mark Kretschmann
 * @short Populates the Playlist-view with URLs
 *
 * Will load playlists, remote and local, and tags (if local)
 */
class PlaylistLoader
    : public ThreadWeaver::DependentJob
    , protected PlaylistFileTranslator
{
public:
    PlaylistLoader( QObject *dependent, const KURL::List&, QListViewItem*, bool playFirstUrl = false );
   ~PlaylistLoader();

    enum EventType { Item = 3000, DomItem };

protected:
    /// reimplemented from ThreadWeaver::Job
    virtual bool doJob();
    virtual void completeJob();
    virtual void customEvent( QCustomEvent* );

    /// reimplemented from PlaylistFileTranslator
    virtual void postItem( const KURL&, const QString &title, uint length );
    virtual void postXmlItem( const KURL&, const QDomNode& );

private:
    KURL::List recurse( const KURL& );

private:
    KURL::List m_badURLs;
    KURL::List m_URLs;

    PlaylistItem *m_markerListViewItem;

    bool m_playFirstUrl;

    Debug::Block m_block;

protected:
    PlaylistLoader( const PlaylistLoader& ); //undefined
    PlaylistLoader &operator=( const PlaylistLoader& ); //undefined
};



/**
 * @author Max Howell
 * @short Populates the Playlist-view using the result of a single SQL query
 *
 * The format of the query must be in a set order, see doJob()
 */
class SqlLoader : public PlaylistLoader
{
    const QString m_sql;

public:
    SqlLoader( const QString &sql, QListViewItem *after );

    virtual bool doJob();
};


/**
 * @author Max Howell
 * @short Fetches a playlist-file from any location, and then loads it into the Playlist-view
 */
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


#endif
