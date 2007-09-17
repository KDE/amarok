// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// .ram file support from Kaffeine 0.5, Copyright (C) 2004 by Jürgen Kofler (GPL 2 or later)
// .asx file support added by Michael Seiwert Copyright (C) 2006
// .asx file support from Kaffeine, Copyright (C) 2004-2005 by Jürgen Kofler (GPL 2 or later)
// .smil file support from Kaffeine 0.7
// .pls parser (C) Copyright 2005 by Michael Buesch <mbuesch@freenet.de>
// .xspf file support added by Mattias Fliesberg <mattias.fliesberg@gmail.com> Copyright (C) 2006
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#define DEBUG_PREFIX "PlaylistLoader"

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "enginecontroller.h"
#include "mountpointmanager.h"
#include "mydirlister.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include "statusbar.h"
#include "contextbrowser.h"
#include "xspfplaylist.h"

#include <qdatetime.h>   //::recurse()
#include <qeventloop.h>  //::recurse()
#include <qfile.h>       //::loadPlaylist()
#include <qlistview.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qtextstream.h> //::loadPlaylist()

#include <dcopref.h>
#include <kapplication.h>
#include <kurl.h>



//TODO playlists within playlists, local or remote are legal entries in m3u and pls
//TODO directories from inside playlists

struct XMLData
{
    MetaBundle bundle;
    int queue;
    bool stopafter;
    bool dynamicdisabled;
    XMLData(): queue(-1), stopafter(false), dynamicdisabled(false) { }
};

class TagsEvent : public QCustomEvent {
public:
    TagsEvent( const QValueList<XMLData> &x ) : QCustomEvent( 1001 ), xml( QDeepCopy<QValueList<XMLData> >( x ) ) { }
    TagsEvent( const BundleList &bees ) : QCustomEvent( 1000 ), bundles( QDeepCopy<BundleList>( bees ) ) {
        for( BundleList::Iterator it = bundles.begin(), end = bundles.end(); it != end; ++it )
        {
            (*it).detach();
            /// @see MetaBundle for explanation of audioproperties < 0
            if( (*it).length() <= 0 || (*it).bitrate() <= 0 )
                (*it).readTags( TagLib::AudioProperties::Fast, 0 );
        }
    }

    QValueList<XMLData> xml;
    BundleList bundles;
};


UrlLoader::UrlLoader( const KURL::List &urls, QListViewItem *after, int options )
        : ThreadManager::DependentJob( Playlist::instance(), "UrlLoader" )
        , m_markerListViewItem( new PlaylistItem( Playlist::instance(), after ) )
        , m_playFirstUrl( options & (Playlist::StartPlay | Playlist::DirectPlay) )
        , m_coloring( options & Playlist::Colorize )
        , m_options( options )
        , m_block( "UrlLoader" )
        , m_oldQueue( Playlist::instance()->m_nextTracks )
        , m_xmlSource( 0 )
{

    connect( this,                 SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ),
             Playlist::instance(), SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ) );

    Playlist::instance()->lock(); // prevent user removing items as this could be bad

    Amarok::OverrideCursor cursor;

    setDescription( i18n("Populating playlist") );

    Amarok::StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_description )
            .setStatus( i18n("Preparing") )
            .setAbortSlot( this, SLOT(abort()) )
            .setTotalSteps( 100 );

    foreachType( KURL::List, urls ) {
        const KURL url = Amarok::detachedKURL(Amarok::mostLocalURL( *it ));
        const QString protocol = url.protocol();

        if( protocol == "seek" )
            continue;

        else if( ContextBrowser::hasContextProtocol( url ) )
        {
            DEBUG_BLOCK
            debug() << "context expandurl" << endl;

            m_URLs += ContextBrowser::expandURL(Amarok::detachedKURL(url));
        }

        else if( !MetaBundle::isKioUrl( url ) )
        {
            m_URLs += url;
        }

        else if( protocol == "file" ) {
            if( QFileInfo( url.path() ).isDir() )
                m_URLs += recurse( url );
            else
                m_URLs += url;
        }

        // Note: remove for kde 4 - we don't need to be hacking around KFileDialog,
        // it has been fixed for kde 3.5.3
        else if( protocol == "media" || url.url().startsWith( "system:/media/" ) )
        {
            QString path = url.path( -1 );
            if( url.url().startsWith( "system:/media/" ) )
                path = path.mid( 6 );
            // url looks like media:/device/path
            DCOPRef mediamanager( "kded", "mediamanager" );
            QString device = path.mid( 1 ); // remove first slash
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
            debug() << "remote playlist" << endl;
            new RemotePlaylistFetcher( url, after, m_options );
            m_playFirstUrl = false;
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
    if( Playlist::instance() )
    {
        Playlist::instance()->unlock();
        if( m_markerListViewItem )
           delete m_markerListViewItem;
    }

    delete m_xmlSource;
}

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
                QApplication::postEvent( this, new TagsEvent( playlist.bundles()) );
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
UrlLoader::customEvent( QCustomEvent *e)
{
    //DEBUG_BLOCK
    #define e static_cast<TagsEvent*>(e)
    switch( e->type() ) {
    case 1000:
        foreachType( BundleList, e->bundles )
        {
            int alreadyOnPlaylist = 0;

            PlaylistItem *item = 0;
            if( m_options & (Playlist::Unique | Playlist::Queue) )
            {
                item = Playlist::instance()->m_urlIndex.getFirst( (*it).url() );
            }

            if( item )
                alreadyOnPlaylist++;
            else
                item = new PlaylistItem( *it, m_markerListViewItem, (*it).exists() );

            if( m_options & Playlist::Queue )
                Playlist::instance()->queue( item );

            if( m_playFirstUrl && (*it).exists() )
            {
                Playlist::instance()->activate( item );
                m_playFirstUrl = false;
            }
        }
        break;

    case 1001:
    {
        foreachType( QValueList<XMLData>, e->xml )
        {
            if( (*it).bundle.isEmpty() ) //safety
                continue;

            PlaylistItem* const item = new PlaylistItem( (*it).bundle, m_markerListViewItem );
            item->setIsNew( m_coloring );

            //TODO scrollbar position
            //TODO previous tracks queue
            //TODO current track position, even if user doesn't have resume playback turned on

            if( (*it).queue >= 0 ) {
                if( (*it).queue == 0 )
                    Playlist::instance()->setCurrentTrack( item );

                else if( (*it).queue > 0 ) {
                    PLItemList &m_nextTracks = Playlist::instance()->m_nextTracks;
                    int count = m_nextTracks.count();

                    for( int c = count; c < (*it).queue; c++ )
                        // Append foo values and replace with correct values later.
                        m_nextTracks.append( item );

                    m_nextTracks.replace( (*it).queue - 1, item );
                }
            }
            if( (*it).stopafter )
                Playlist::instance()->m_stopAfterTrack = item;

            item->setFilestatusEnabled( (*it).bundle.exists() );
            item->setDynamicEnabled( !( (*it).dynamicdisabled ) );
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
    DEBUG_BLOCK
    const PLItemList &newQueue = Playlist::instance()->m_nextTracks;
    QPtrListIterator<PlaylistItem> it( newQueue );
    PLItemList added;
    for( it.toFirst(); *it; ++it )
        if( !m_oldQueue.containsRef( *it ) )
            added << (*it);

    if( !added.isEmpty() )
        emit queueChanged( added, PLItemList() );

    if ( !m_badURLs.isEmpty() ) {
        QString text = i18n("These media could not be loaded into the playlist: " );
        debug() << "The following urls were not suitable for the playlist:" << endl;
        for ( uint it = 0; it < m_badURLs.count(); it++  )
        {
            if( it < 5 )
                text += QString("<br>%1").arg( m_badURLs[it].prettyURL() );
            else if( it == 5 )
                text += QString("<br>Plus %1 more").arg( m_badURLs.count() - it );
            debug() << "\t" << m_badURLs[it] << endl;
        }

        Amarok::StatusBar::instance()->shortLongMessage(
                i18n("Some media could not be loaded (not playable)."), text );
    }

    if( !m_dynamicMode.isEmpty() )
        Playlist::instance()->setDynamicMode( PlaylistBrowser::instance()->findDynamicModeByTitle( m_dynamicMode ) );

    //synchronous, ie not using eventLoop
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

namespace Amarok
{

// almost the same as UrlLoader::recurse, but global
KURL::List
recursiveUrlExpand( const KURL &url, int maxURLs )
{
    typedef QMap<QString, KURL> FileMap;

    if( url.protocol() != "file" || !QFileInfo( url.path() ).isDir() )
        return KURL::List( url );

    MyDirLister lister( false );
    lister.setAutoUpdate( false );
    lister.setAutoErrorHandlingEnabled( false, 0 );
    if ( !lister.openURL( url ) )
        return KURL::List();

    // Fucking KDirLister sometimes hangs on remote media, so we add a timeout
    const int timeout = 3000; // ms
    QTime watchdog;
    watchdog.start();

    while( !lister.isFinished() && watchdog.elapsed() < timeout )
        kapp->eventLoop()->processEvents( QEventLoop::ExcludeUserInput );

    KFileItemList items = lister.items(); //returns QPtrList, so we MUST only do it once!
    KURL::List urls;
    FileMap files;
    for( KFileItem *item = items.first(); item; item = items.next() ) {
        if( maxURLs >= 0 && (int)(urls.count() + files.count()) >= maxURLs )
            break;
        if( item->isFile()
                && !PlaylistFile::isPlaylistFile( item->url().fileName() )
                )
        {
            files[item->name()] = item->url();
            continue;
        }
        if( item->isDir() )
            urls += recursiveUrlExpand( item->url(), maxURLs - urls.count() - files.count() );
    }

    foreachType( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        urls += *it;

    return urls;
}

KURL::List
recursiveUrlExpand( const KURL::List &list, int maxURLs )
{
    KURL::List urls;
    foreachType( KURL::List, list )
    {
        if( maxURLs >= 0 && (int)urls.count() >= maxURLs )
            break;
        urls += recursiveUrlExpand( *it, maxURLs - urls.count() );
    }

    return urls;
}

} // Amarok

void
UrlLoader::loadXml( const KURL &url )
{
    QFile file( url.path() );
    if( !file.open( IO_ReadOnly ) ) {
        m_badURLs += url;
        return;
    }
    m_currentURL = url;

    delete m_xmlSource;
    m_xmlSource = new QXmlInputSource( file );
    MyXmlLoader loader;
    connect( &loader, SIGNAL( newBundle( const MetaBundle&, const XmlAttributeList& ) ),
             this, SLOT( slotNewBundle( const MetaBundle&, const XmlAttributeList& ) ) );
    connect( &loader, SIGNAL( playlistInfo( const QString&, const QString&, const QString& ) ),
             this, SLOT( slotPlaylistInfo( const QString&, const QString&, const QString& ) ) );
    loader.load( m_xmlSource );
    if( !m_xml.isEmpty() )
    {
        QApplication::postEvent( this, new TagsEvent( m_xml ) );
        m_xml.clear();
    }
    if( !loader.lastError().isEmpty() )
    {
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n(
                //TODO add a link to the path to the playlist
                "The XML in the playlist was invalid. Please report this as a bug to the Amarok "
                "developers. Thank you." ), KDE::StatusBar::Error );
        ::error() << "[PLAYLISTLOADER]: Error in " << m_currentURL.prettyURL() << ": " << loader.lastError() << endl;
    }
}

void UrlLoader::slotNewBundle( const MetaBundle &bundle, const XmlAttributeList &atts )
{
    XMLData data;
    data.bundle = QDeepCopy<MetaBundle>( bundle );
    for( int i = 0, n = atts.count(); i < n; ++i )
    {
        if( atts[i].first == "queue_index" )
        {
            bool ok = true;
            data.queue = atts[i].second.toInt( &ok );
            if( !ok )
                data.queue = -1;
        }
        else if( atts[i].first == "stop_after" )
            data.stopafter = true;
        else if( atts[i].first == "dynamicdisabled" )
            data.dynamicdisabled = true;
    }
    data.bundle.checkExists();
    m_xml.append( data );
    if( m_xml.count() == OPTIMUM_BUNDLE_COUNT )
    {
        QApplication::postEvent( this, new TagsEvent( m_xml ) );
        m_xml.clear();
    }
}

void UrlLoader::slotPlaylistInfo( const QString &, const QString &version, const QString &dynamicMode )
{
    if( version != Amarok::xmlVersion() )
    {
        Amarok::StatusBar::instance()->longMessageThreadSafe( i18n(
            "Your last playlist was saved with a different version of Amarok than this one, "
            "and this version can no longer read it.\n"
            "You will have to create a new one.\n"
            "Sorry :(" ) );
        static_cast<MyXmlLoader*>( const_cast<QObject*>( sender() ) )->abort(); //HACK?
        return;
    }
    else
        m_dynamicMode = dynamicMode;
}

/// @class PlaylistFile

PlaylistFile::PlaylistFile( const QString &path )
        : m_path( path )
{
    QFile file( path );
    if( !file.open( IO_ReadOnly ) ) {
        m_error = i18n( "Amarok could not open the file." );
        return;
    }

    QTextStream stream( &file );

    switch( format( m_path ) ) {
    case M3U: loadM3u( stream ); break;
    case PLS: loadPls( stream ); break;
    case XML:
        m_error = i18n( "This component of Amarok cannot translate XML playlists." );
        return;
    case RAM: loadRealAudioRam( stream ); break;
    case ASX: loadASX( stream ); break;
    case SMIL: loadSMIL( stream ); break;
    case XSPF: loadXSPF( stream ); break;
    default:
        m_error = i18n( "Amarok does not support this playlist format." );
        return;
    }

    if( m_error.isEmpty() && m_bundles.isEmpty() )
        m_error = i18n( "The playlist did not contain any references to files." );
    debug() << m_error << endl;
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

            if( KURL::isRelativeURL( url ) ) {
                KURL kurl( KURL::fromPathOrURL( directory + line ) );
                kurl.cleanPath();
                b.setPath( kurl.path() );
            }
            else {
                b.setUrl( KURL::fromPathOrURL( line ) );
            }

            // Ensure that we always have a title: use the URL as fallback
            if( b.title().isEmpty() )
                b.setTitle( url );

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
            // Ensure that if the entry has no title, we show at least the URL as title
            m_bundles[index - 1].setTitle(tmp);
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

bool
PlaylistFile::loadXSPF( QTextStream &stream )
{
    XSPFPlaylist* doc = new XSPFPlaylist( stream );

    XSPFtrackList trackList = doc->trackList();

    foreachType( XSPFtrackList, trackList )
    {
        KURL location = (*it).location;
        QString artist = (*it).creator;
        QString title  = (*it).title;
        QString album  = (*it).album;

        if( location.isEmpty() || ( location.isLocalFile() && !QFile::exists( location.url() ) ) )
        {
            QueryBuilder qb;
            qb.addMatch( QueryBuilder::tabArtist, QueryBuilder::valName, artist );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valTitle, title );
            if( !album.isEmpty() )
                qb.addMatch( QueryBuilder::valName, album );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            QStringList values = qb.run();

            if( values.isEmpty() ) continue;

            MetaBundle b( values[0] );

            m_bundles += b;
        }
        else
        {
            debug() << location << ' ' << artist << ' ' << title << ' ' << album << endl;
            MetaBundle b;
            b.setUrl( location );
            b.setArtist( artist );
            b.setTitle( title );
            b.setAlbum( album );
            b.setComment( (*it).annotation );
            b.setLength( (*it).duration / 1000 );
            m_bundles += b;
        }
    }

    m_title = doc->title();

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

bool
PlaylistFile::loadASX( QTextStream &stream )
{
    //adapted from Kaffeine 0.7
    MetaBundle b;
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QString content = stream.read();

    //ASX looks a lot like xml, but doesn't require tags to be case sensitive,
    //meaning we have to accept things like: <Abstract>...</abstract>
    //We use a dirty way to achieve this: we make all tags lower case
    QRegExp ex("(<[/]?[^>]*[A-Z]+[^>]*>)");
    ex.setCaseSensitive(true);
    while ( (ex.search(content)) != -1 )
        content.replace(ex.cap( 1 ), ex.cap( 1 ).lower());


    if (!doc.setContent(content, &errorMsg, &errorLine, &errorColumn))
    {
        debug() << "Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn << endl;
        return false;
    }

    QDomElement root = doc.documentElement();

    QString url;
    QString title;
    QString author;
    QTime length;
    QString duration;

    if (root.nodeName().lower() != "asx") return false;

    QDomNode node = root.firstChild();
    QDomNode subNode;
    QDomElement element;

    while (!node.isNull())
    {
       url = QString::null;
       title = QString::null;
       author = QString::null;
       length = QTime();
       if (node.nodeName().lower() == "entry")
       {
          subNode = node.firstChild();
          while (!subNode.isNull())
          {
             if ((subNode.nodeName().lower() == "ref") && (subNode.isElement()) && (url.isNull()))
             {
                element = subNode.toElement();
                if (element.hasAttribute("href"))
                   url = element.attribute("href");
                if (element.hasAttribute("HREF"))
                   url = element.attribute("HREF");
                if (element.hasAttribute("Href"))
                   url = element.attribute("Href");
                if (element.hasAttribute("HRef"))
                   url = element.attribute("HRef");
              }
		if ((subNode.nodeName().lower() == "duration") && (subNode.isElement()))
                {
                   duration = QString::null;
                   element = subNode.toElement();
                   if (element.hasAttribute("value"))
                      duration = element.attribute("value");
                   if (element.hasAttribute("Value"))
                      duration = element.attribute("Value");
                   if (element.hasAttribute("VALUE"))
                      duration = element.attribute("VALUE");

                   if (!duration.isNull())
                      length = PlaylistFile::stringToTime(duration);
                  }

                  if ((subNode.nodeName().lower() == "title") && (subNode.isElement()))
                  {
                     title = subNode.toElement().text();
                  }
                  if ((subNode.nodeName().lower() == "author") && (subNode.isElement()))
                  {
                     author = subNode.toElement().text();
                  }
		  subNode = subNode.nextSibling();
	  }
	  if (!url.isNull())
          {
	     if (title.isNull())
                title = url;
                b.setUrl(KURL(url));
                m_bundles += b;
                b = MetaBundle();
	   }
        }
	node = node.nextSibling();
     }
     return true;
}

bool
PlaylistFile::loadSMIL( QTextStream &stream )
{
	// adapted from Kaffeine 0.7
	QDomDocument doc;
    if( !doc.setContent( stream.read() ) )
    {
        debug() << "Could now read smil playlist" << endl;
        return false;
    }
	QDomElement root = doc.documentElement();
	stream.setEncoding ( QTextStream::UnicodeUTF8 );

	if( root.nodeName().lower() != "smil" )
       return false;

	KURL kurl;
	QString url;
	QDomNodeList nodeList;
	QDomNode node;
	QDomElement element;

	//audio sources...
	nodeList = doc.elementsByTagName( "audio" );
	for( uint i = 0; i < nodeList.count(); i++ )
	{
        MetaBundle b;
		node = nodeList.item(i);
		url = QString::null;
		if( (node.nodeName().lower() == "audio") && (node.isElement()) )
		{
			element = node.toElement();
			if( element.hasAttribute("src") )
				url = element.attribute("src");

			else if( element.hasAttribute("Src") )
				url = element.attribute("Src");

			else if( element.hasAttribute("SRC") )
				url = element.attribute("SRC");
		}
		if( !url.isNull() )
		{
			b.setUrl( url );
			m_bundles += b;
		}
	}

	return true;
}


/// @class RemotePlaylistFetcher

#include <ktempfile.h>
#include <kio/job.h>
#include <klocale.h>

RemotePlaylistFetcher::RemotePlaylistFetcher( const KURL &source, QListViewItem *after, int options )
        : QObject( Playlist::instance()->qscrollview() )
        , m_source( source )
        , m_after( after )
        , m_playFirstUrl( options & (Playlist::StartPlay | Playlist::DirectPlay) )
        , m_options( options )
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

    Amarok::StatusBar::instance()->newProgressOperation( job )
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

        UrlLoader *loader = new UrlLoader( m_destination, m_after, m_options );
        ThreadManager::instance()->queueJob( loader );

        // we mustn't get deleted until the loader is finished
        // or the playlist we downloaded will be deleted before
        // it can be parsed!
        loader->insertChild( this );
    }
}



/// @class SqlLoader

SqlLoader::SqlLoader( const QString &sql, QListViewItem *after, int options )
        : UrlLoader( KURL::List(), after, options )
        , m_sql( QDeepCopy<QString>( sql ) )
{
    // Ovy: just until we make sure every SQL query from dynamic playlists is handled
    // correctly
    debug() << "Sql loader: query is: " << sql << "\n";
}

bool
SqlLoader::doJob()
{
    DEBUG_BLOCK
    const QStringList values = CollectionDB::instance()->query( m_sql );

    setProgressTotalSteps( values.count() );

    BundleList bundles;
    uint x = 0;
    for( for_iterators( QStringList, values ); it != end && !isAborted(); ++it ) {
        setProgress( x += QueryBuilder::dragFieldCount );

        bundles += CollectionDB::instance()->bundleFromQuery( &it );

        if( bundles.count() == OPTIMUM_BUNDLE_COUNT || it == last ) {
            QApplication::postEvent( this, new TagsEvent( bundles ) );
            bundles.clear();
        }
    }

    setProgress100Percent();

    return true;
}

QTime PlaylistFile::stringToTime(const QString& timeString)
{
   int sec = 0;
   bool ok = false;
   QStringList tokens = QStringList::split(':',timeString);

   sec += tokens[0].toInt(&ok)*3600; //hours
   sec += tokens[1].toInt(&ok)*60; //minutes
   sec += tokens[2].toInt(&ok); //secs

   if (ok)
      return QTime().addSecs(sec);
         else
            return QTime();
}

bool MyXmlLoader::startElement( const QString &a, const QString &name, const QString &b, const QXmlAttributes &atts )
{
    if( name == "playlist" )
    {
        QString product, version, dynamic;
        for( int i = 0, n = atts.count(); i < n; ++i )
        {
            if( atts.localName( i ) == "product" )
                product = atts.value( i );
            else if( atts.localName( i ) == "version" )
                version = atts.value( i );
            else if( atts.localName( i ) == "dynamicMode" )
                dynamic = atts.value( i );
        }
        emit playlistInfo( product, version, dynamic );
        return !m_aborted;
    }
    else
        return XmlLoader::startElement( a, name, b, atts );
}

#include "playlistloader.moc"
