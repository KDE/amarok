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
#include "playlist.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include "statusbar.h"

#include <qfile.h>       //::loadPlaylist()
#include <qfileinfo.h>
#include <qlistview.h>
#include <qmap.h>        //::recurse()
#include <qstringlist.h>
#include <qtextstream.h> //::loadPlaylist()

#include <kapplication.h>
#include <kdirlister.h>
#include <kurl.h>


//TODO playlists within playlists, local or remote are legal entries in m3u and pls


PlaylistLoader::PlaylistLoader( QObject *dependent, const KURL::List &urls, QListViewItem *after, bool playFirstUrl )
    : ThreadWeaver::DependentJob( dependent, "PlaylistLoader" )
    , m_dirLister( new KDirLister() )
    , m_markerListViewItem( new PlaylistItem( Playlist::instance(), after ) )
    , m_playFirstUrl( playFirstUrl )
{
    setDescription( i18n("Loading media") ); //TODO better wording

    amaroK::StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_description )
            .setStatus( i18n("Preparing") )
            .setTotalSteps( 100 );

    m_markerListViewItem->setText( 0, "IF YOU CAN SEE THIS THERE IS A BUG" );

    m_dirLister->setAutoUpdate( false );
    m_dirLister->setAutoErrorHandlingEnabled( false, 0 );

    // BEGIN Read folders recursively
    amaroK::OverrideCursor cursor;
    KURL::List::ConstIterator it;
    const KURL::List::ConstIterator end = urls.end();

    for( it = urls.begin(); it != end && !isAborted(); ++it )
    {
        const KURL &url = *it;
        const bool isLocalFile = url.isLocalFile();

        if ( isPlaylist( url ) && !isLocalFile ) {
            new RemotePlaylistFetcher( url, after, dependent );
            continue;
        }

        if ( url.protocol() == "fetchcover" ) {
            // ignore
            continue;
        }
        if ( url.protocol() == "album" ) {
           // url looks like:   album:<artist_id> @@@ <album_id>
           // extract artist_id, album_id
           QString myUrl = url.path();
           if ( myUrl.endsWith( " @@@" ) )
               myUrl += ' ';
           const QStringList list = QStringList::split( " @@@ ", myUrl, true );
           Q_ASSERT( !list.isEmpty() );
           QString artist_id = list.front();
           QString album_id  = list.back();

           // get tracks for album, and add them to the playlist
           QStringList trackUrls = CollectionDB::instance()->albumTracks( artist_id, album_id );
           KURL url;
           foreach( trackUrls ) {
                url.setPath( *it );
                m_URLs.append( url );
           }
        }
        else if ( isLocalFile ) {
            if ( QFileInfo( url.path() ).isDir() )
                    recurse( url );
            else
                m_URLs.append( url );
        }
        else if ( !recurse( url ) )
            m_URLs.append( url );
    }

    delete m_dirLister;
}


PlaylistLoader::~PlaylistLoader()
{}



/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

class TagsEvent : public QCustomEvent {
public:
    TagsEvent( const MetaBundle &b, PlaylistItem *i )
            : QCustomEvent( 1000 )
            , item( i )
            , bundle( b ) {}

    TagsEvent( const KURL &u, const QDomNode &n )
            : QCustomEvent( 1001 )
            , item( 0 )
            , url( u )
            , node( n ) {}

    PlaylistItem* const item;
    MetaBundle bundle;
    KURL url;
    QDomNode node;
};

bool
PlaylistLoader::doJob()
{
    typedef QValueList<MetaBundle> BundleList;
    typedef QPair<MetaBundle, PlaylistItem*> Pair;
    typedef QValueList<Pair> PairList;
    PairList pairs;

    setProgressTotalSteps( m_URLs.count() );
    setStatus( i18n("Populating playlist") );

    //TODO should I only send it local urls?
    BundleList bundles = CollectionDB::instance()->bundlesByUrls( m_URLs );

    // 1st pass create items
    BundleList::ConstIterator it;
    BundleList::ConstIterator end = bundles.end();

    for ( it = bundles.begin(); it != end && !isAborted(); ++it ) {
        incrementProgress();

        const KURL &url = (*it).url();

        if ( isPlaylist( url ) ) {
            if ( !loadPlaylist( url.path() ) )
                m_badURLs += url;
            continue;
        }

        if ( EngineController::canDecode( url ) ) {
            PlaylistItem *item = new PlaylistItem( url, m_markerListViewItem );
            pairs += Pair( *it, item );
        }
        else
            m_badURLs += url;
   }

   // people think things work faster if the statusbar fills up
   // multiple times weird but true
   setProgress( 0 );
   setStatus( i18n("Filling in tags") );

   // 2nd pass, fill in tags
   for( PairList::ConstIterator it = pairs.begin(), end = pairs.end(); it != end && !isAborted(); ++it ) {
       incrementProgress();

//        if ( (*it).first.url().isLocalFile() )
           QApplication::postEvent( this, new TagsEvent( (*it).first, (*it).second ) );
   }

    return true;
}

void
PlaylistLoader::customEvent( QCustomEvent *e )
{
    #define e static_cast<TagsEvent*>(e)
    switch( e->type() )
    {
    case 1000:
        e->item->setText( e->bundle );
        break;

    case 1001: {
        PlaylistItem *item = new PlaylistItem( e->url, m_markerListViewItem, e->node );
        QString attribute  = e->node.toElement().attribute( "queue_index" );

        //TODO scrollbar position
        //TODO previous tracks queue
        //TODO current track position, even if user doesn't have resum playback turned on

        if ( !attribute.isEmpty() ) { /// Setting current track, and filling nextTracks queue
            const int index = attribute.toInt();

            if ( index == 0 )
                Playlist::instance()->setCurrentTrack( item );

            else if ( index > 0 ) {
                QPtrList<PlaylistItem> &m_nextTracks = Playlist::instance()->m_nextTracks;
                int count = m_nextTracks.count();

                for( int c = count; c < index; c++ )
                    // Append foo values and replace with correct values later.
                    m_nextTracks.append( item );

                m_nextTracks.replace( index - 1, item );
            }
        }

        break; }

    default:
        DependentJob::customEvent( e );
        return;
    }

    if( m_playFirstUrl )
    {
        Playlist::instance()->activate( e->item );
        m_playFirstUrl = false;
    }

    #undef e
}

void
PlaylistLoader::completeJob()
{
    delete m_markerListViewItem; //TODO doesn't get deleted if this isn't called

    KURL::List &list = m_badURLs;

    if ( !list.isEmpty() ) {
        amaroK::StatusBar::instance()->shortLongMessage(
                i18n("Some URLs were not suitable for the playlist."),
                i18n("These URLs could not be loaded into the playlist: " ) ); //TODO

        for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
            debug() << *it << endl;
    }

    //syncronous, ie not using eventLoop
    QApplication::sendEvent( dependent(), this );
}

bool
PlaylistLoader::loadPlaylist( const QString &path, Format type )
{
    QFile file( path );
    if( !file.open( IO_ReadOnly ) ) return false;
    QTextStream stream( &file );

    switch( type ) {
    case M3U: {
        const QString dir = path.left( path.findRev( '/' ) + 1 );
        QString str, title;
        int length = MetaBundle::Undetermined; // = -2

        while( !( str = stream.readLine() ).isNull() && !isAborted() )
        {
            if ( str.startsWith( "#EXTINF" ) )
            {
                QString extinf = str.section( ':', 1);
                length = extinf.section( ',', 0, 0 ).toInt();
                title = extinf.section( ',', 1 );

                if ( length == 0 ) length = MetaBundle::Undetermined;
            }
            else if ( !str.startsWith( "#" ) && !str.isEmpty() )
            {

                if ( !( str[ 0 ] == '/' || str.contains( "://" ) ) )
                    str.prepend( dir );

                postItem( KURL::fromPathOrURL( str ), title, length );

                length = MetaBundle::Undetermined;
                title = QString();
            }
        }
        break;
    }
    case PLS:
        for( QString line = stream.readLine(); !line.isNull() && !isAborted(); line = stream.readLine() ) {
            if ( line.startsWith( "File" ) ) {
                const KURL url = KURL::fromPathOrURL( line.section( "=", -1 ) );
                QString title;
                int length = 0;

                line = stream.readLine();

                if ( line.startsWith( "Title" ) ) {
                    title = line.section( "=", -1 );
                    line  = stream.readLine();
                }

                if ( line.startsWith( "Length" ) )
                    length = line.section( "=", -1 ).toInt();

                postItem( url, title, length );
            }
        }
        break;

    case XML: {
        stream.setEncoding( QTextStream::UnicodeUTF8 );

        QDomDocument d;
        if( !d.setContent( stream.read() ) ) return false;

        const QString ITEM( "item" ); //so we don't construct these QStrings all the time
        const QString URL( "url" );

        for( QDomNode n = d.namedItem( "playlist" ).firstChild();
             !n.isNull() && n.nodeName() == ITEM && !isAborted();
             n = n.nextSibling() )
        {
            const QDomElement e = n.toElement();

            if ( !e.isNull() )
                QApplication::postEvent( this, new TagsEvent( KURL(e.attribute( URL )), n ) );
        }
    }
    default:
        ;
    } //switch

    return true;
}

void
PlaylistLoader::postItem( const KURL &url, const QString &title, const uint length )
{
    PlaylistItem *item = new PlaylistItem( url, m_markerListViewItem );

    if ( url.protocol() == "file" )
        QApplication::postEvent( this, new TagsEvent( url, item ) );

    else {
        TagsEvent *e = new TagsEvent( KURL(), item );

        e->url = url;
        e->bundle.setTitle( title );
        e->bundle.setLength( length );

        QApplication::postEvent( this, e );
    }
}



/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
/////////////////////////////////////////////////////////////////////////////////////

bool
PlaylistLoader::recurse( const KURL &url, bool recursing )
{
        static bool success;
        if ( !recursing ) success = false;

        typedef QMap<QString, KURL> FileMap;

        KURL::List dirs;
        FileMap files;

        m_dirLister->openURL( url );

        while ( !m_dirLister->isFinished() )
            //FIXME this is a crash waiting to happen
            kapp->processEvents( 100 );

        KFileItem* item;
        KFileItemList items = m_dirLister->items();

        success |= !items.isEmpty();

        for ( item = items.first(); item; item = items.next() ) {
            if ( item->url().fileName() == "." || item->url().fileName() == ".." )
                continue;
            if ( item->isFile() )
                files[item->url().fileName()] = item->url();
            if ( item->isDir() )
                dirs << item->url();
        }

        // Add files to URL list
        const FileMap::ConstIterator end1 = files.end();
        for ( FileMap::ConstIterator it = files.begin(); it != end1; ++it )
            m_URLs.append( it.data() );

        // Recurse folders
        const KURL::List::Iterator end2 = dirs.end();
        for ( KURL::List::Iterator it = dirs.begin(); it != end2; ++it )
            recurse( *it, true );

        return success;
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
    ThreadWeaver::instance()->queueJob( new PlaylistLoader( parent(), url, m_after ) );

    deleteLater();
}

#include "playlistloader.moc"
