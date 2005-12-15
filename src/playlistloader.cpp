// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// .ram file support from Kaffeine 0.5, Copyright (C) 2004 by Jürgen Kofler (GPL 2 or later)
// .pls parser (C) Copyright 2005 by Michael Buesch <mbuesch@freenet.de>
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

#include <qdatetime.h>   //::recurse()
#include <qeventloop.h>  //::recurse()
#include <qfile.h>       //::loadPlaylist()
#include <qlistview.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qtextstream.h> //::loadPlaylist()

#include <dcopref.h>
#include <kapplication.h>
#include <kdirlister.h>
#include <kurl.h>



typedef QValueList<QDomNode> NodeList;


//TODO playlists within playlists, local or remote are legal entries in m3u and pls
//TODO directories from inside playlists


UrlLoader::UrlLoader( const KURL::List &urls, QListViewItem *after, bool playFirstUrl )
        : ThreadWeaver::DependentJob( Playlist::instance(), "UrlLoader" )
        , m_markerListViewItem( new PlaylistItem( Playlist::instance(), after ) )
        , m_playFirstUrl( playFirstUrl )
        , m_block( "UrlLoader" )
        , m_oldQueue( Playlist::instance()->m_nextTracks )
{
    DEBUG_BLOCK

    connect( this,                 SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ),
             Playlist::instance(), SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ) );

    Playlist::instance()->lock(); // prevent user removing items as this could be bad

    amaroK::OverrideCursor cursor;

    setDescription( i18n("Populating playlist") );

    amaroK::StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_description )
            .setStatus( i18n("Preparing") )
            .setAbortSlot( this, SLOT(abort()) )
            .setTotalSteps( 100 );

    m_markerListViewItem->setText( 0, "MARKERITEM" );

    foreachType( KURL::List, urls ) {
        const KURL &url = *it;
        const QString protocol = url.protocol();

        if( protocol == "file" ) {
            if( QFileInfo( url.path() ).isDir() )
                m_URLs += recurse( url );
            else
                m_URLs += url;
        }

       else if( protocol == "media" ) {
            // url looks like media:/device/path
            DCOPRef mediamanager( "kded", "mediamanager" );
            QString device = url.path( -1 ).mid( 1 ); // remove first slash
            const int slash = device.find( '/' );
            const QString filePath = device.mid( slash ); // extract relative path
            device = device.left( slash ); // extract device
            DCOPReply reply = mediamanager.call( "properties(QString)", device );

            if( reply.isValid() ) {
                const QStringList properties = reply;
                // properties[6] is the mount point
                KURL localUrl = KURL( properties[6] + filePath );

                // add urls
                if( QFileInfo( localUrl.path() ).isDir() )
                    m_URLs += recurse( localUrl );
                else
                    m_URLs += localUrl;
            }
        }

        else if( PlaylistFile::isPlaylistFile( url ) ) {
            new RemotePlaylistFetcher( url, after, m_playFirstUrl );
            m_playFirstUrl = false;
        }

        else if( protocol == "fetchcover" ) {
            // url looks like:   fetchcover:<artist_name> @@@ <album_name>
           QString myUrl = url.path();
           if ( myUrl.endsWith( " @@@" ) )
               myUrl += ' ';
           const QStringList list = QStringList::split( " @@@ ", myUrl, true );
           Q_ASSERT( !list.isEmpty() );
           QString artist_name = list.front();
           QString album_name  = list.back();

           QStringList trackUrls = CollectionDB::instance()->albumTracks( artist_name, album_name, true );
           KURL url;
           foreach( trackUrls ) {
                url.setPath( *it );
                m_URLs += url;
           }
        }
        else if( protocol == "seek" )
            continue;

        else if( protocol == "album" ) {
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
        else if( protocol == "compilation" ) {
            QueryBuilder qb;
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, url.path() );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.setOptions( QueryBuilder::optOnlyCompilations );
            QStringList values = qb.run();

            KURL::List urls;
            KURL url;

            for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
                url.setPath( *it );
                m_URLs += url;
            }
        }
        else {
            // this is the best way I found for recursing if required
            // and not recusring if not required
            const KURL::List urls = recurse( url );

            // recurse only works on directories, else it swallows the URL
            if( urls.isEmpty() )
                m_URLs += url;
            else
                m_URLs += urls;
        }
    }
}

UrlLoader::~UrlLoader()
{
    Playlist::instance()->unlock();

    delete m_markerListViewItem;
}


class TagsEvent : public QCustomEvent {
public:
    TagsEvent() : QCustomEvent( 1001 ) {}
    TagsEvent( const BundleList &bees ) : QCustomEvent( 1000 ), bundles( bees ) {
        for( BundleList::Iterator it = bundles.begin(), end = bundles.end(); it != end; ++it )
            /// @see MetaBundle for explanation of audioproperties < 0
            if( (*it).length() <= 0 || (*it).bitrate() <= 0 )
                (*it).readTags( TagLib::AudioProperties::Fast );
    }

    NodeList nodes;
    BundleList bundles;
};

bool
UrlLoader::doJob()
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
UrlLoader::customEvent( QCustomEvent *e )
{
    #define e static_cast<TagsEvent*>(e)
    switch( e->type() ) {
    case 1000:
        foreachType( BundleList, e->bundles )
        {
            //Only add files that exist to the playlist
            if( !(*it).exists() )
                continue;

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
        const QString STOP_AFTER = "stop_after";
        const QString DISABLED = "disabled";
        foreachType( NodeList, e->nodes )
        {
            if( (*it).isNull() ) //safety
                continue;

            const QDomElement element = QDomNode( *it ).toElement(); //we check it is an element in loadPlaylist
            PlaylistItem* const item = new PlaylistItem( *it, m_markerListViewItem );

            //TODO scrollbar position
            //TODO previous tracks queue
            //TODO current track position, even if user doesn't have resume playback turned on

            if( element.hasAttribute( QUEUE_INDEX ) ) {
                const int index = element.attribute( QUEUE_INDEX ).toInt();

                if( index == 0 )
                    Playlist::instance()->setCurrentTrack( item );

                else if( index > 0 ) {
                    PLItemList &m_nextTracks = Playlist::instance()->m_nextTracks;
                    int count = m_nextTracks.count();

                    for( int c = count; c < index; c++ )
                        // Append foo values and replace with correct values later.
                        m_nextTracks.append( item );

                    m_nextTracks.replace( index - 1, item );
                }
            }
            if( element.hasAttribute( STOP_AFTER ) )
                Playlist::instance()->m_stopAfterTrack = item;

            if( element.hasAttribute( DISABLED ) )
                item->setEnabled( false );
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
UrlLoader::completeJob()
{
    const PLItemList &newQueue = Playlist::instance()->m_nextTracks;
    QPtrListIterator<PlaylistItem> it( newQueue );
    PLItemList added;
    for( it.toFirst(); *it; ++it )
        if( !m_oldQueue.containsRef( *it ) )
            added << (*it);

    if( !added.isEmpty() )
        emit queueChanged( added, PLItemList() );

    if ( !m_badURLs.isEmpty() ) {
        amaroK::StatusBar::instance()->shortLongMessage(
                i18n("Some media could not be loaded (not playable)."),
                i18n("These media could not be loaded into the playlist: " ) );

        debug() << "The following urls were not suitable for the playlist:" << endl;
        for ( uint it = 0; it < m_badURLs.count(); it++  )
            debug() << "\t" << m_badURLs[it] << endl;
    }

    //syncronous, ie not using eventLoop
    QApplication::sendEvent( dependent(), this );
}

KURL::List
UrlLoader::recurse( const KURL &url )
{
    typedef QMap<QString, KURL> FileMap;

    KDirLister lister( false );
    lister.setAutoUpdate( false );
    lister.setAutoErrorHandlingEnabled( false, 0 );
    if ( !lister.openURL( url ) )
        return KURL::List();

    // Fucking KDirLister sometimes hangs on remote media, so we add a timeout
    const int timeout = 3000; // ms
    QTime watchdog;
    watchdog.start();

    while( !lister.isFinished() && !isAborted() && watchdog.elapsed() < timeout )
        kapp->eventLoop()->processEvents( QEventLoop::ExcludeUserInput );

    KFileItemList items = lister.items(); //returns QPtrList, so we MUST only do it once!
    KURL::List urls;
    FileMap files;
    for( KFileItem *item = items.first(); item; item = items.next() ) {
        if( item->isFile() ) { files[item->name()] = item->url(); continue; }
        if( item->isDir() ) urls += recurse( item->url() );
    }

    foreachType( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        if( !PlaylistFile::isPlaylistFile( (*it).fileName() ) )
            urls += *it;

    return urls;
}

void
UrlLoader::loadXml( const KURL &url )
{
    QFile file( url.path() );
    if( !file.open( IO_ReadOnly ) ) {
        m_badURLs += url;
        return;
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QString er;
    int l, c;
    if( !d.setContent( stream.read(), &er, &l, &c ) ) { // return error values
        amaroK::StatusBar::instance()->longMessageThreadSafe( i18n(
                //TODO add a link to the path to the playlist
                "The XML in the playlist was invalid. Please report this as a bug to the amaroK "
                "developers. Thank you." ), KDE::StatusBar::Error );
        error() << "[PLAYLISTLOADER]: Error loading xml file: " << url.prettyURL() << "(" << er << ")"
                << " at line " << l << ", column " << c << endl;
        return;
    }

    NodeList nodes;
    TagsEvent* e = new TagsEvent();
    const QString ITEM( "item" ); //so we don't construct this QString all the time
    for( QDomNode n = d.namedItem( "playlist" ).firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != ITEM ) continue;

        if( !n.toElement().isNull() )
            e->nodes += n;

        if( e->nodes.count() == OPTIMUM_BUNDLE_COUNT ) {
            QApplication::postEvent( this, e );
            e = new TagsEvent();
        }
    }
    QApplication::postEvent( this, e );
}



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
    case RAM: loadRealAudioRam( stream ); break;
    case ASX:
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

    for( QString line; !stream.atEnd(); )
    {
        line = stream.readLine();

        if( line.startsWith( "#EXTINF" ) ) {
            const QString extinf = line.section( ':', 1 );
            const int length = extinf.section( ',', 0, 0 ).toInt();
            b.setTitle( extinf.section( ',', 1 ) );
            b.setLength( length <= 0 ? /*MetaBundle::Undetermined HACK*/ -2 : length );
        }

        else if( !line.startsWith( "#" ) && !line.isEmpty() )
        {
            // KURL::isRelativeURL() expects absolute URLs to start with a protocol, so prepend it if missing
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
    // Counted number of "File#=" lines.
    unsigned int entryCnt = 0;
    // Value of the "NumberOfEntries=#" line.
    unsigned int numberOfEntries = 0;
    // Does the file have a "[playlist]" section? (as it's required by the standard)
    bool havePlaylistSection = false;
    QString tmp;
    QStringList lines;

    const QRegExp regExp_NumberOfEntries("^NumberOfEntries\\s*=\\s*\\d+$");
    const QRegExp regExp_File("^File\\d+\\s*=");
    const QRegExp regExp_Title("^Title\\d+\\s*=");
    const QRegExp regExp_Length("^Length\\d+\\s*=\\s*\\d+$");
    const QRegExp regExp_Version("^Version\\s*=\\s*\\d+$");
    const QString section_playlist("[playlist]");

    /* Preprocess the input data.
     * Read the lines into a buffer; Cleanup the line strings;
     * Count the entries manually and read "NumberOfEntries".
     */
    while (!stream.atEnd()) {
        tmp = stream.readLine();
        tmp = tmp.stripWhiteSpace();
        if (tmp.isEmpty())
            continue;
        lines.append(tmp);

        if (tmp.contains(regExp_File)) {
            entryCnt++;
            continue;
        }
        if (tmp == section_playlist) {
            havePlaylistSection = true;
            continue;
        }
        if (tmp.contains(regExp_NumberOfEntries)) {
            numberOfEntries = tmp.section('=', -1).stripWhiteSpace().toUInt();
            continue;
        }
    }
    if (numberOfEntries != entryCnt) {
        warning() << ".pls playlist: Invalid \"NumberOfEntries\" value.  "
                  << "NumberOfEntries=" << numberOfEntries << "  counted="
                  << entryCnt << endl;
        /* Corrupt file. The "NumberOfEntries" value is
         * not correct. Fix it by setting it to the manually
         * counted number and go on parsing.
         */
        numberOfEntries = entryCnt;
    }
    if (!numberOfEntries)
        return true;

    unsigned int index;
    bool ok = false;
    bool inPlaylistSection = false;

    Q_ASSERT(m_bundles.isEmpty());
    m_bundles.insert(m_bundles.begin(), numberOfEntries, MetaBundle());
    /* Now iterate through all beautified lines in the buffer
     * and parse the playlist data.
     */
    QStringList::const_iterator i = lines.begin(), end = lines.end();
    for ( ; i != end; ++i) {
        if (!inPlaylistSection && havePlaylistSection) {
            /* The playlist begins with the "[playlist]" tag.
             * Skip everything before this.
             */
            if ((*i) == section_playlist)
                inPlaylistSection = true;
            continue;
        }
        if ((*i).contains(regExp_File)) {
            // Have a "File#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).stripWhiteSpace();
            m_bundles[index - 1].setUrl(KURL::fromPathOrURL(tmp));
            continue;
        }
        if ((*i).contains(regExp_Title)) {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).stripWhiteSpace();
            m_bundles[index - 1].setTitle(tmp);
            continue;
        }
        if ((*i).contains(regExp_Length)) {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).stripWhiteSpace();
            m_bundles[index - 1].setLength(tmp.toInt(&ok));
            Q_ASSERT(ok);
            continue;
        }
        if ((*i).contains(regExp_NumberOfEntries)) {
            // Have the "NumberOfEntries=#" line.
            continue;
        }
        if ((*i).contains(regExp_Version)) {
            // Have the "Version=#" line.
            tmp = (*i).section('=', 1).stripWhiteSpace();
            // We only support Version=2
            if (tmp.toUInt(&ok) != 2)
                warning() << ".pls playlist: Unsupported version." << endl;
            Q_ASSERT(ok);
            continue;
        }
        warning() << ".pls playlist: Unrecognized line: \"" << *i << "\"" << endl;
    }
    return true;
}

unsigned int
PlaylistFile::loadPls_extractIndex( const QString &str ) const
{
    /* Extract the index number out of a .pls line.
     * Example:
     *   loadPls_extractIndex("File2=foobar") == 2
     */
    bool ok = false;
    unsigned int ret;
    QString tmp(str.section('=', 0, 0));
    tmp.remove(QRegExp("^\\D*"));
    ret = tmp.stripWhiteSpace().toUInt(&ok);
    Q_ASSERT(ok);
    return ret;
}

bool
PlaylistFile::loadRealAudioRam( QTextStream &stream )
{
    MetaBundle b;
    QString url;
    //while loop adapted from Kaffeine 0.5
    while (!stream.atEnd())
    {
        url = stream.readLine();
        if (url[0] == '#') continue; /* ignore comments */
        if (url == "--stop--") break; /* stop line */
        if ((url.left(7) == "rtsp://") || (url.left(6) == "pnm://") || (url.left(7) == "http://"))
        {
            b.setUrl(KURL(url));
            m_bundles += b;
            b = MetaBundle();
        }
    }

    return true;
}
/// @class RemotePlaylistFetcher

#include <ktempfile.h>
#include <kio/job.h>
#include <klocale.h>

RemotePlaylistFetcher::RemotePlaylistFetcher( const KURL &source, QListViewItem *after, bool playFirstUrl )
        : QObject( (QObject*)Playlist::instance() )
        , m_source( source )
        , m_after( after )
        , m_playFirstUrl( playFirstUrl )
{
    //We keep the extension so the UrlLoader knows what file type it is
    const QString path = source.path();
    m_temp = new KTempFile( QString::null /*use default prefix*/, path.mid( path.findRev( '.' ) ) );
    m_temp->setAutoDelete( true );

    m_destination.setPath( m_temp->name() );

    KIO::Job *job = KIO::file_copy( m_source, m_destination,
            -1,      /* permissions, this means "do what you think" */
            true,    /* overwrite */
            false,   /* resume download */
            false ); /* don't show stupid UIServer dialog */

    amaroK::StatusBar::instance()->newProgressOperation( job )
            .setDescription( i18n("Retrieving Playlist") );

    connect( job, SIGNAL(result( KIO::Job* )), SLOT(result( KIO::Job* )) );

    Playlist::instance()->lock();
}

RemotePlaylistFetcher::~RemotePlaylistFetcher()
{
    Playlist::instance()->unlock();

    delete m_temp;
}

void
RemotePlaylistFetcher::result( KIO::Job *job )
{
    if( job->error() ) {
        error() << "Couldn't download remote playlist\n";
        deleteLater();
    }

    else {
        debug() << "Playlist was downloaded successfully\n";

        UrlLoader *loader = new UrlLoader( m_destination, m_after, m_playFirstUrl );
        ThreadWeaver::instance()->queueJob( loader );

        // we mustn't get deleted until the loader is finished
        // or the playlist we downloaded will be deleted before
        // it can be parsed!
        loader->insertChild( this );
    }
}



/// @class SqlLoader

SqlLoader::SqlLoader( const QString &sql, QListViewItem *after )
        : UrlLoader( KURL::List(), after, false )
        , m_sql( sql )
{}

bool
SqlLoader::doJob()
{
    DEBUG_BLOCK

    const QStringList values = CollectionDB::instance()->query( m_sql );

    setProgressTotalSteps( values.count() );

    BundleList bundles;
    uint x = 0;
    for( for_iterators( QStringList, values ); it != end || isAborted(); ++it ) {
        setProgress( x += 11 );

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
