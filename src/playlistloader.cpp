// Author: Max Howell (C) Copyright 2003
// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#include "collectiondb.h"
#include "enginecontroller.h"
#include "playlist.h"
#include "playlistloader.h"
#include "statusbar.h"

#include <qapplication.h>
#include <qfile.h>       //::loadPlaylist()
#include <qfileinfo.h>
#include <qlistview.h>
#include <qmap.h>        //::recurse()
#include <qtextstream.h> //::loadPlaylist()

#include <kapplication.h>
#include <kdirlister.h>


bool PlaylistLoader::s_stop = false;

PlaylistLoader::PlaylistLoader( const KURL::List &urls, QListView *parent, QListViewItem *after, bool playFirstUrl )
    : QThread()
    , m_URLs( urls )
    , m_afterItem( after )
    , m_playFirstUrl( playFirstUrl )
    , m_db( new CollectionDB )
    , m_dirLister( new KDirLister() )
{
    m_dirLister->setAutoUpdate( false );
    m_dirLister->setAutoErrorHandlingEnabled( false, 0 );
}


PlaylistLoader::~PlaylistLoader()
{
    s_stop = false;
    delete m_db;
    delete m_dirLister;
}


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

    amaroK::StatusBar::instance()->message( i18n("Retrieving playlist...") );
    QApplication::setOverrideCursor( KCursor::waitCursor() );
        const bool succeeded = KIO::NetAccess::download( url, path, listView );
    QApplication::restoreOverrideCursor();
    amaroK::StatusBar::instance()->clear();

    if( succeeded )
    {
        //TODO delete the tempfile
        KURL url;
        url.setPath( path );

        (new PlaylistLoader( KURL::List( url ), listView, item, directPlay ))->start( QThread::IdlePriority );

    } else {

        KMessageBox::sorry( listView, i18n( "<p>The playlist, <i>'%1'</i>, could not be downloaded." ).arg( url.prettyURL() ) );
        tmpfile.unlink();
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
PlaylistLoader::run()
{
    amaroK::StatusBar::startProgress();
    QApplication::postEvent( Playlist::instance(), new StartedEvent( m_afterItem, m_playFirstUrl ) );

    KURL::List::ConstIterator end = m_fileURLs.end();
    KURL::List::ConstIterator it;

    // BEGIN Read folders recursively
    end = m_URLs.end();
    for( it = m_URLs.begin(); it != end && !s_stop; ++it )
        if ( !recurse( *it ) )
            m_fileURLs.append( *it );
    // END


    // BEGIN: Read tags, post bundles to Playlist
    float increment = 100.0 / m_fileURLs.count();
    float progress = 0;

    end = m_fileURLs.end();
    for ( it = m_fileURLs.begin(); it != end && !s_stop; ++it )
    {
        const KURL &url = *it;

        if( url.isLocalFile() && loadPlaylist( url.path() ) )
            continue;

        if ( EngineController::canDecode( url ) )
            postItem( url );

        progress += increment;
        amaroK::StatusBar::showProgress( uint(progress) );

        // Allow GUI thread some time to breathe
        msleep( 5 );
   }
   // END

    amaroK::StatusBar::stopProgress();
    QApplication::postEvent( Playlist::instance(), new DoneEvent( this ) );
}


void
PlaylistLoader::postItem( const KURL &url, const QString &title, const uint length )
{
    MetaBundle bundle( url, true, m_db );

    bundle.setTitle( title );
    bundle.setLength( length );

    QApplication::postEvent( Playlist::instance(), new ItemEvent( bundle ) );
}


#include <kdebug.h>
bool
PlaylistLoader::loadPlaylist( const QString &path, Format type )
{
    QFile file( path );
    if ( !file.open( IO_ReadOnly ) )
    {
        kdDebug() << "[PLSLoader] Couldn't open file: " << path << endl;
        return false;
    }
    QTextStream stream( &file );

    switch( type )
    {
    case M3U:
    {
        const QString dir = path.left( path.findRev( '/' ) + 1 );
        QString str, title;
        int length = MetaBundle::Undetermined; // = -2

        while( !( str = stream.readLine() ).isNull() && !s_stop )
        {
            if ( str.startsWith( "#EXTINF" ) )
            {
                QString extinf = str.section( ':', 1, 1 );
                length = extinf.section( ',', 0, 0 ).toInt();
                title = extinf.section( ',', 1 );

                if ( length == 0 ) length = MetaBundle::Undetermined;
            }
            else if ( !str.startsWith( "#" ) && !str.isEmpty() )
            {

                if ( !( str[ 0 ] == '/' || str.contains( ':' ) ) )
                    str.prepend( dir );

                postItem( KURL::fromPathOrURL( str ), title, length );

                length = MetaBundle::Undetermined;
                title = QString();
            }
        }
        break;
    }
    case PLS:

        for( QString line = stream.readLine(); !line.isNull() && !s_stop; line = stream.readLine() )
        {
            if( line.startsWith( "File" ) )
            {
                const KURL url = KURL::fromPathOrURL( line.section( "=", -1 ) );
                QString title;
                int length = 0;

                line = stream.readLine();

                if ( line.startsWith( "Title" ) )
                {
                    title = line.section( "=", -1 );
                    line  = stream.readLine();
                }

                if ( line.startsWith( "Length" ) )
                    length = line.section( "=", -1 ).toInt();

                postItem( url, title, length );
            }
        }
        break;

    case XML:
    {
        stream.setEncoding( QTextStream::UnicodeUTF8 );

        QDomDocument d;
        if( !d.setContent( stream.read() ) ) return false;

        const QString ITEM( "item" ); //so we don't construct these QStrings all the time
        const QString URL( "url" );

        for( QDomNode n = d.namedItem( "playlist" ).firstChild();
             !n.isNull() && n.nodeName() == ITEM && !s_stop;
             n = n.nextSibling() )
        {
            const QDomElement e = n.toElement();

            if ( !e.isNull() )
                QApplication::postEvent( Playlist::instance(), new DomItemEvent( KURL(e.attribute( URL ) ), n ) );
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
            msleep( 20 );

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
    MetaBundle bundle( url, true, m_db );
    QApplication::postEvent( Playlist::instance(), new ItemEvent( bundle ) );
}

