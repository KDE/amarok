// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#define DEBUG_PREFIX "PlaylistLoader"

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "enginecontroller.h"
#include <kapplication.h>
#include <kdirlister.h>
#include <kurl.h>
#include "playlist.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include <qfile.h>       //::loadPlaylist()
#include <qlistview.h>
#include <qstringlist.h>
#include <qtextstream.h> //::loadPlaylist()
#include "statusbar.h"


typedef QValueList<QDomNode> NodeList;


//TODO playlists within playlists, local or remote are legal entries in m3u and pls
//TODO directories from inside playlists


PlaylistLoader::PlaylistLoader( const KURL::List &urls, QListViewItem *after, bool playFirstUrl )
        : ThreadWeaver::DependentJob( Playlist::instance(), "PlaylistLoader" )
        , m_markerListViewItem( new PlaylistItem( Playlist::instance(), after ) )
        , m_playFirstUrl( playFirstUrl )
        , m_block( "PlaylistLoader" )
{
    DEBUG_BLOCK

    amaroK::OverrideCursor cursor;

    setDescription( i18n("Populating playlist") ); //TODO better wording

    amaroK::StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_description )
            .setStatus( i18n("Preparing") )
            .setTotalSteps( 100 );

    m_markerListViewItem->setText( 0, "IF YOU CAN SEE THIS THERE IS A BUG" );

    foreachType( KURL::List, urls )
    {
        const KURL &url = *it;

        if( PlaylistFile::isPlaylistFile( url ) && !url.isLocalFile() ) {
            new RemotePlaylistFetcher( url, after, dependent() );
            continue;
        }

        else if( url.protocol() == "fetchcover" )
            continue;

        else if( url.protocol() == "album" ) {
           // url looks like:   album:<artist_id> @@@ <album_id>
           QString myUrl = url.path();
           if ( myUrl.endsWith( " @@@" ) )
               myUrl += ' ';
           const QStringList list = QStringList::split( " @@@ ", myUrl, true );
           Q_ASSERT( !list.isEmpty() );
           QString artist_id = list.front();
           QString album_id = list.back();

           QStringList trackUrls = CollectionDB::instance()->albumTracks( artist_id, album_id );
           KURL url;
           foreach( trackUrls ) {
                url.setPath( *it );
                m_URLs += url;
           }
        }
        else if( KFileItem( KFileItem::Unknown, KFileItem::Unknown, url ).isDir() )
            // we must check if it's a directory before we call recurse() on it
            m_URLs += recurse( url );

        else
            m_URLs += url;
    }
}

PlaylistLoader::~PlaylistLoader()
{
    delete m_markerListViewItem;
}


class TagsEvent : public QCustomEvent {
public:
    TagsEvent() : QCustomEvent( 1001 ) {}
    TagsEvent( const BundleList &bees ) : QCustomEvent( 1000 ), bundles( bees ) {
        for( BundleList::Iterator it = bundles.begin(), end = bundles.end(); it != end; ++it )
            /// @see MetaBundle for explanation of length < 0
            if( (*it).length() <= 0 ) (*it).readTags( TagLib::AudioProperties::Fast );
    }

    NodeList nodes;
    BundleList bundles;
};

bool
PlaylistLoader::doJob()
{
    setProgressTotalSteps( m_URLs.count() );

    KURL::List urls;
    for( for_iterators( KURL::List, m_URLs ); it != end && !isAborted(); ++it )
    {
        const KURL &url = *it;

        incrementProgress();

        switch( PlaylistFile::format( url.fileName() ) )
        {
        case PlaylistFile::XML:
            loadXml( url );
            break;

        default: {
            PlaylistFile playlist( url.path() );

            if( !playlist.isError() )
                QApplication::postEvent( this, new TagsEvent( playlist.bundles() ) );
            else
                m_badURLs += url;

            } break;

        case PlaylistFile::NotPlaylist:
            (EngineController::canDecode( url ) ? urls : m_badURLs) += url;
        }

        if( urls.count() == OPTIMUM_BUNDLE_COUNT || it == last ) {
            QApplication::postEvent( this, new TagsEvent( CollectionDB::instance()->bundlesByUrls( urls ) ) );
            urls.clear();
        }
    }

    return true;
}

void
PlaylistLoader::customEvent( QCustomEvent *e )
{
    #define e static_cast<TagsEvent*>(e)
    switch( e->type() ) {
    case 1000:
        foreachType( BundleList, e->bundles ) {
            PlaylistItem *item = new PlaylistItem( *it, m_markerListViewItem );

            if( m_playFirstUrl ) {
                Playlist::instance()->activate( item );
                m_playFirstUrl = false;
            }
        }
        break;

    case 1001:
    {
        const QString QUEUE_INDEX = "queue_index";
        const NodeList::ConstIterator end = e->nodes.end();
        foreachType( NodeList, e->nodes )
        {
            const QDomElement element = QDomNode( *it ).toElement(); //we check it is an element in loadPlaylist
            PlaylistItem* const item = new PlaylistItem( (*it), m_markerListViewItem );

            //TODO scrollbar position
            //TODO previous tracks queue
            //TODO current track position, even if user doesn't have resume playback turned on

            if( element.hasAttribute( QUEUE_INDEX ) ) {
                const int index = element.attribute( QUEUE_INDEX ).toInt();

                if( index == 0 )
                    Playlist::instance()->setCurrentTrack( item );

                else if( index > 0 ) {
                    QPtrList<PlaylistItem> &m_nextTracks = Playlist::instance()->m_nextTracks;
                    int count = m_nextTracks.count();

                    for( int c = count; c < index; c++ )
                        // Append foo values and replace with correct values later.
                        m_nextTracks.append( item );

                    m_nextTracks.replace( index - 1, item );
                }
            }
        }
        break;
    }

    default:
        DependentJob::customEvent( e );
        return;
    }
    #undef e
}

void
PlaylistLoader::completeJob()
{
    KURL::List &list = m_badURLs;

    if ( !list.isEmpty() ) {
        amaroK::StatusBar::instance()->shortLongMessage(
                i18n("Some URLs were not suitable for the playlist."),
                i18n("These URLs could not be loaded into the playlist: " ) );

//        for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
//            debug() << *it << endl;
    }

    //syncronous, ie not using eventLoop
    QApplication::sendEvent( dependent(), this );
}

#include <qeventloop.h>
KURL::List
PlaylistLoader::recurse( const KURL &url )
{
    typedef QMap<QString, KURL> FileMap;

    KDirLister lister( false );
    lister.setAutoUpdate( false );
    lister.setAutoErrorHandlingEnabled( false, 0 );
    lister.openURL( url );

    while( !lister.isFinished() )
        kapp->eventLoop()->processEvents( QEventLoop::ExcludeUserInput );

    KFileItemList items = lister.items(); //returns QPtrList, so we MUST only do it once!
    KURL::List urls;
    FileMap files;
    for( KFileItem *item = items.first(); item; item = items.next() ) {
        if( item->isFile() ) { files[item->name()] = item->url(); continue; }
        if( item->isDir() ) urls += recurse( item->url() );
    }

    foreachType( FileMap, files )
        urls += *it;

    return urls;
}

void
PlaylistLoader::loadXml( const KURL &url )
{
    QFile file( url.path() );
    if( !file.open( IO_ReadOnly ) ) {
        m_badURLs += url;
        return;
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    if( !d.setContent( stream.read() ) ) {
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n(
                //TODO add a link to the path to the playlist
                "The XML in the playlist was invalid. Please report this as a bug to the amaroK "
                "developers. Thank you." ) );
        return;
    }

    NodeList nodes;
    TagsEvent *e = new TagsEvent;
    const QString ITEM( "item" ); //so we don't construct this QString all the time
    for( QDomNode n = d.namedItem( "playlist" ).firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != ITEM ) continue;

        if( !n.toElement().isNull() )
            e->nodes += n;

        if( e->nodes.count() == OPTIMUM_BUNDLE_COUNT ) {
            QApplication::postEvent( this, e );
            e = new TagsEvent;
        }
    }

    QApplication::postEvent( this, e );
};



/// @class PlaylistFile

PlaylistFile::PlaylistFile( const QString &path )
        : m_path( path )
{
    QFile file( path );
    if( !file.open( IO_ReadOnly ) ) {
        m_error = i18n( "amaroK could not open the file." );
        return;
    }

    QTextStream stream( &file );

    switch( format( m_path ) ) {
    case M3U: loadM3u( stream ); break;
    case PLS: loadPls( stream ); break;
    case XML:
        m_error = i18n( "This component of amaroK cannot translate XML playlists." );
        return;
    default:
        m_error = i18n( "amaroK does not support this playlist format." );
        return;
    }

    if( m_error.isEmpty() && m_bundles.isEmpty() )
        m_error = i18n( "The playlist did not contain any references to files." );
}

bool
PlaylistFile::loadM3u( QTextStream &stream )
{
    const QString directory = m_path.left( m_path.findRev( '/' ) + 1 );
    MetaBundle b;

    for( QString line; !stream.atEnd(); line = stream.readLine() )
    {
        if( line.isEmpty() ) continue;

        if( line.startsWith( "#EXTINF" ) ) {
            const QString extinf = line.section( ':', 1 );
            const int length = extinf.section( ',', 0, 0 ).toInt();
            b.setTitle( extinf.section( ',', 1 ) );
            b.setLength( length <= 0 ? /*MetaBundle::Undetermined HACK*/ -2 : length );
        }

        else if( !line.startsWith( "#" ) )
        {
            // KURL::isRelativeURL() expects a protocol, so prepend it if missing
            QString url = line;
            if( url.startsWith( "/" ) )
                url.prepend( "file://" );

            if( KURL::isRelativeURL( url ) )
                b.setPath( directory + line );
            else
                b.setUrl( KURL::fromPathOrURL( line ) );

            m_bundles += b;
            b = MetaBundle();
        }
    }

    return true;
}

bool
PlaylistFile::loadPls( QTextStream &stream )
{
    for( QString line; !stream.atEnd(); line = stream.readLine() )
    {
        if( line.startsWith( "File" ) ) {
            MetaBundle b;

            b.setUrl( KURL::fromPathOrURL( line.section( "=", -1 ) ) );
            line = stream.readLine();

            if( line.startsWith( "Title" ) ) {
                b.setTitle( line.section( "=", -1 ) );
                line = stream.readLine();
            }

            if( line.startsWith( "Length" ) )
                b.setLength( line.section( "=", -1 ).toInt() );

            m_bundles += b;
        }
    }

    return true;
}


/// @class RemotePlaylistFetcher

#include <ktempfile.h>
#include <kio/job.h>
#include <klocale.h>

RemotePlaylistFetcher::RemotePlaylistFetcher( const KURL &source, QListViewItem *after, QObject *playlist )
        : QObject( playlist )
        , m_source( source )
        , m_after( after )
{
    //We keep the extension so the PlaylistLoader knows what file type it is
    QString path = source.path();
    KTempFile tempfile( QString::null /*use default prefix*/, path.mid( path.findRev( '.' ) ) );

    m_destination.setPath( tempfile.name() );

    KIO::Job *job = KIO::file_copy( m_source, m_destination,
            -1,      /* permissions, this means "do what you think" */
            true,    /* overwrite */
            false,   /* resume download */
            false ); /* don't show stupid UIServer dialog */

    amaroK::StatusBar::instance()->newProgressOperation( job )
            .setDescription( i18n("Retrieving Playlist") );

    connect( job, SIGNAL(result( KIO::Job* )), SLOT(result( KIO::Job* )) );
    connect( playlist, SIGNAL(aboutToClear()), SLOT(abort()) );

    //TODO delete the tempfile
}

void
RemotePlaylistFetcher::result( KIO::Job *job )
{
    if ( job->error() )
        return;

    debug() << "Playlist was downloaded successfully\n";

    const KURL url = static_cast<KIO::FileCopyJob*>(job)->destURL();
    ThreadWeaver::instance()->queueJob( new PlaylistLoader( url, m_after ) );

    deleteLater();
}



/// @class SqlLoader

SqlLoader::SqlLoader( const QString &sql, QListViewItem *after )
    : PlaylistLoader( KURL::List(), after, false )
    , m_sql( sql )
{
    setDescription( i18n("Populating playlist") );
}

bool
SqlLoader::doJob()
{
    DEBUG_BLOCK

    BundleList bundles;
    QStringList values = CollectionDB::instance()->query( m_sql );

    setProgressTotalSteps( values.count() );

    uint x = 0;
    for( for_iterators( QStringList, values ); it != end || isAborted(); ++it ) {
        setProgress( ++x * 11 );

        MetaBundle b;
        b.setAlbum     (    *it );
        b.setArtist    (  *++it );
        b.setGenre     (  *++it );
        b.setTitle     (  *++it );
        b.setYear      (  *++it );
        b.setComment   (  *++it );
        b.setTrack     (  *++it );
        b.setBitrate   ( (*++it).toInt() );
        b.setLength    ( (*++it).toInt() );
        b.setSampleRate( (*++it).toInt() );
        b.setPath      (  *++it );

        bundles += b;

        if( false && b.length() <= 0 ) {
            // we try to read the tags, despite the slow-down
            debug() << "Audioproperties not known for: " << b.url().fileName() << endl;
            b.readTags( TagLib::AudioProperties::Fast );
        }

        if( bundles.count() == OPTIMUM_BUNDLE_COUNT || it == last ) {
            QApplication::postEvent( this, new TagsEvent( bundles ) );
            bundles.clear();
        }
    }

    setProgress100Percent();

    return true;
}

#include "playlistloader.moc"
