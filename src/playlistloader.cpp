// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#define DEBUG_PREFIX "PlaylistLoader"

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


PlaylistLoader::PlaylistLoader( QObject *recipient, const KURL::List &urls, QListViewItem *after, bool playFirstUrl )
    : ThreadWeaver::DependentJob( recipient, "PlaylistLoader" )
    , m_URLs( urls )
    , m_dirLister( new KDirLister() )
    , m_markerListViewItem( new PlaylistItem( Playlist::instance(), after ) )
    , m_playFirstUrl( playFirstUrl )
{
    setDescription( i18n("Loading media") ); //TODO better wording

    amaroK::StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_description )
            .setTotalSteps( 100 );

    m_markerListViewItem->setText( 0, "IF YOU CAN SEE THIS THERE IS A BUG" );

    m_dirLister->setAutoUpdate( false );
    m_dirLister->setAutoErrorHandlingEnabled( false, 0 );

    // BEGIN Read folders recursively
    KURL::List::ConstIterator it;
    KURL::List::ConstIterator end = m_URLs.end();

    for( it = m_URLs.begin(); it != end && !isAborted(); ++it )
    {
        const KURL url = *it;

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
           QStringList trackValues = CollectionDB::instance()->albumTracks( artist_id, album_id );
           if ( !trackValues.isEmpty() )
               for ( uint j = 0; j < trackValues.count(); j++ ) {
                   KURL url;
                   url.setPath(trackValues[j]);
                   url.setProtocol("file");
                   m_fileURLs.append( url );
               }
        } else
            if ( url.isLocalFile() ) {
                if ( QFileInfo( url.path() ).isDir() )
                    recurse( url );
                else
                    m_fileURLs.append( url );
            }
            else if ( !recurse( url ) )
                m_fileURLs.append( url );
    }
    // END

    delete m_dirLister;
}


PlaylistLoader::~PlaylistLoader()
{}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
/////////////////////////////////////////////////////////////////////////////////////

#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <klocale.h>
void
PlaylistLoader::downloadPlaylist( const KURL &url, QListView *listView, QListViewItem *item, bool directPlay )
{
    //KIO::NetAccess can make it's own tempfile
    //but we need to add .pls/.m3u extension or the Loader will fail
    QString path = url.filename();
    KTempFile tmpfile( QString::null, path.mid( path.findRev( '.' ) ) ); //use default prefix
    path = tmpfile.name();

//TODO    setStatus( i18n("Retrieving playlist...") );

    QApplication::setOverrideCursor( KCursor::waitCursor() );
        const bool succeeded = KIO::NetAccess::download( url, path, listView );
    QApplication::restoreOverrideCursor();

    if( succeeded )
    {
        //TODO delete the tempfile
        KURL url;
        url.setPath( path );

        ThreadWeaver::instance()->queueJob( new PlaylistLoader( listView, KURL::List( url ), item, directPlay ) );

    } else {

        amaroK::StatusBar::instance()->longMessage(
                i18n("<p>The playlist, <i>'%1'</i>, could not be downloaded.").arg( url.prettyURL() ),
                KDE::StatusBar::Sorry );
        tmpfile.unlink();
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

bool
PlaylistLoader::doJob()
{
    setProgressTotalSteps( m_fileURLs.count() );

    KURL::List::ConstIterator it;
    KURL::List::ConstIterator end = m_fileURLs.end();

    for ( it = m_fileURLs.begin(); it != end && !isAborted(); ++it )
    {
        const KURL &url = *it;

        if( url.isLocalFile() && isPlaylist( url ) && loadPlaylist( url.path() ) )
            continue;

        if ( EngineController::canDecode( url ) )
            postItem( url );
        else
            m_badURLs += url;

        incrementProgress();

        // Allow GUI thread some time to breathe
        msleep( 2 );
   }

    return true;
}

void
PlaylistLoader::completeJob()
{
    delete m_markerListViewItem; //TODO doesn't get deleted if this isn't called

    KURL::List &list = m_badURLs;

    if ( !list.isEmpty() ) {
        amaroK::StatusBar::instance()->shortLongMessage(
                i18n("Some URLs could not be loaded."),
        i18n("These URLs could not be loaded into the playlist: " ) ); //TODO

        for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
            kdDebug() << *it << endl;
    }

    QApplication::sendEvent( dependent(), this );
}

void
PlaylistLoader::postItem( const KURL &url, const QString &title, const uint length )
{
    MetaBundle bundle( url, true, CollectionDB::instance() );

    if ( !title.isEmpty())
        bundle.setTitle( title );

    if ( length != MetaBundle::Undetermined )
        bundle.setLength( length );

    QApplication::postEvent( Playlist::instance(), new ItemEvent( bundle, m_markerListViewItem, m_playFirstUrl ) );

    m_playFirstUrl = false;
}

bool
PlaylistLoader::loadPlaylist( const QString &path, Format type )
{
    QFile file( path );
    if ( !file.open( IO_ReadOnly ) ) {
        amaroK::StatusBar::instance()->longMessageThreadSafe(
                i18n( "The playlist file '%1', could not be opened" ),
                KDE::StatusBar::Sorry );
        return false;
    }
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
                QApplication::postEvent(
                        Playlist::instance(),
                        new DomItemEvent(
                                KURL(e.attribute( URL ) ),
                                n,
                                m_markerListViewItem,
                                m_playFirstUrl ) );

            m_playFirstUrl = false;
        }
    }
    default:
        ;
    } //switch

    return true;
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
            m_fileURLs.append( it.data() );

        // Recurse folders
        const KURL::List::Iterator end2 = dirs.end();
        for ( KURL::List::Iterator it = dirs.begin(); it != end2; ++it )
            recurse( *it, true );

        return success;
}


void
PlaylistLoader::postItem( const KURL &url )
{
    MetaBundle bundle( url, true, CollectionDB::instance() );
    QApplication::postEvent( Playlist::instance(), new ItemEvent( bundle, m_markerListViewItem, m_playFirstUrl ) );

    m_playFirstUrl = false;
}
