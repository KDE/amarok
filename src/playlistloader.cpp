
//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef FAST_TRANSLATE 
//#include "amarokfilelist.h"    //for sorting directories
#endif
#include "engine/enginebase.h" //isValidMedia()
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"      //we're tied to this class
#include "playlistitem.h"
#include "playlistloader.h"
#include "statusbar.h"     //for progressbar updating

#include <qapplication.h>  //postEvent()
#include <qtextstream.h>   //loadM3U(),loadPLS()
#include <qfile.h>         //~PlaylistLoader()
#include <qtextcodec.h>    //loadXML()

#include <kapplication.h>
#include <kurl.h>
#include <kdebug.h>
#include <ktempfile.h>     //makePlaylistItem()
#include <kio/netaccess.h> //makePlaylistItem()
#include <kfileitem.h>     //process()
#include <kcursor.h>       //makePlaylistItem()
#include <kmessagebox.h>   //makePlaylistItem()
#include <klocale.h>       //makePlaylistItem()

//file stat
#include <dirent.h>        //process()
#include <sys/stat.h>      //process()

//some GNU systems don't support big files for some reason
#ifndef __USE_LARGEFILE64 //see dirent.h
 #define DIRENT dirent
 #define SCANDIR scandir
 #define READDIR readdir
 #define STATSTRUCT stat
 #define LSTAT stat
#else
 #define DIRENT dirent64
 #define SCANDIR scandir64
 #define READDIR readdir64
 #define STATSTRUCT stat64
 #define LSTAT stat64
#endif


/*
 * For pls and m3u specifications see: http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772
 */

//URGENT
//TODO store threads in a stack that can be emptied on premature program exit, or use one thread and a KURL::List stack
//TODO don't delete m_first, it may already have been removed! either make it unremovable or do something more intelligent

//LESS IMPORTANT
//TODO add non-local directories as items with a [+] next to, you open them by clicking the plus!! --maybe not
//TODO display dialog/something else that lists unloadable media after thread is exited
//TODO stop blocking on netaccess::download()
//TODO recursion limits
//TODO rethink recursion options <mxcl> IMHO they suck big chunks, always do it recursively, why drop/add a
//     directory if you don't want its contents?
//     you can always open the dir and select the files. Stupid option and PITA to implement.
//     <markey> non-recursive adding should get replaced by "add all media files in current directory"
//TODO reimplement ask recursive in Playlist::insertMedia()


bool PlaylistLoader::m_stop;

PlaylistLoader::PlaylistLoader( const KURL::List &ul, QObject *receiver, PlaylistItem *pi )
   : m_list( ul )
   , m_after( pi )
   , m_first( 0 )
   , m_listView( (Playlist*)receiver )
   , m_receiver( receiver )
{
}

// this ctor is called by the PlaylistBrowserItem to load playlist info
PlaylistLoader::PlaylistLoader( const KURL::List &ul, QObject *receiver )
   : m_list( ul )
   , m_after( 0 )
   , m_first( 0 )
   , m_listView( 0 )
   , m_receiver( receiver )
{
}

//this is a private ctor used by ::makePlaylistItem()
//it can only be used with placeholders
PlaylistLoader::PlaylistLoader( const KURL::List &ul, PlaylistItem *item )
        : m_list( ul )
        , m_after( item )
        , m_first( item )
        , m_listView( item->listView() )
        , m_receiver( item->listView() )
{}


PlaylistLoader::~PlaylistLoader()
{
    //for GUI access only

    if ( NULL != m_first ) {
        kdDebug() << "Unlinking tmpfile: " << m_list.first().path() << endl;
        QFile::remove( m_list.first().path() );
        delete m_first; //FIXME deleting m_first is dangerous as user may have done it for us!
    }

    kdDebug() << "[PLSloader] Done!\n";
}


void PlaylistLoader::run()
{
    kdDebug() << "[PLSloader] Started..\n";
    
    m_stop = false;
    QApplication::postEvent( amaroK::StatusBar::instance(), new ProgressEvent( ProgressEvent::Start ) );
    QApplication::postEvent( amaroK::StatusBar::instance(), new ProgressEvent( ProgressEvent::Total, m_list.count() ) );

    m_recursionCount = -1;
    process( m_list );

    QApplication::postEvent( amaroK::StatusBar::instance(), new ProgressEvent( ProgressEvent::Stop ) );
    QApplication::postEvent( m_receiver, new PlaylistLoader::DoneEvent( this ) );
}


void PlaylistLoader::process( const KURL::List &list, const bool validate )
{
    struct STATSTRUCT statbuf;
    ++m_recursionCount;
    
    int count = 0;
    const KURL::List::ConstIterator end = list.end();
    
    for ( KURL::List::ConstIterator it = list.begin(); it != end && !m_stop; ++it, count++ ) {
        
        if ( !( count % 10 ) ) // Don't post events too often since this blocks amaroK
            QApplication::postEvent( amaroK::StatusBar::instance(), new ProgressEvent( ProgressEvent::Progress, count ) );
            
        QString path = ( *it ).path();

        if ( validate && ( *it ).isLocalFile() ) {
            QCString localePath = QFile::encodeName( path );

            if ( LSTAT( localePath, &statbuf ) != 0 ) continue;

            if ( S_ISDIR( statbuf.st_mode ) ) {
                //some options prevent recursion
                //FIXME depth check too
                if ( list.count() > 1 && ( !options.recurse || ( !options.symlink && S_ISLNK( statbuf.st_mode ) ) ) ) continue;
#ifdef FAST_TRANSLATE
                translate( path, localePath );
#else
                AmarokFileList files( options.sortSpec );
                files.setAutoDelete( true );
                translate( path, files );
                files.sort();

                KURL::List urls;
                for ( KFileItemListIterator it( files ); *it; ++it ) {
                    urls << ( *it ) ->url();
                }
                process( urls, false ); //false will prevent stating for dir, etc.
#endif
                continue;
            }
        }

        if ( int type = isPlaylist( path ) ) {
            if ( !m_recursionCount )      //prevent processing playlist files in subdirs
            {
                if ( !( *it ).isLocalFile() )
                {
                    //if the playlist is not local, we need to d/l it, and KIO doesn't work in QThreads. sigh
                    //so this will organise the d/l to occur syncronously and then a new thread spawned :)
                    postDownloadRequest( *it );
                } else
                {
                    loadLocalPlaylist( path, type );
                }
            }
        } else {
            if ( validate && !isValidMedia( *it ) ) continue; //TODO retain stat info if done above, which does happen

            postBundle( *it );
        }
    }
}


inline
void PlaylistLoader::postDownloadRequest( const KURL &u )
{
    QApplication::postEvent( m_listView, new PlaylistLoader::DownloadPlaylistEvent( this, u ) );
}

inline
void PlaylistLoader::postBundle( const KURL &u )
{
    QApplication::postEvent( m_receiver, new PlaylistLoader::MakeItemEvent( this, u, QString::null, MetaBundle::Undetermined ) );

    //    PlaylistItem *item = new PlaylistItem( m_listView, m_after, u, QString::null, MetaBundle::Undetermined ); m_after = item;
}

inline
void PlaylistLoader::postBundle( const KURL &u, const QString &s, const int i )
{
    QApplication::postEvent( m_receiver, new PlaylistLoader::MakeItemEvent( this, u, s , i ) );

    //    PlaylistItem *item = new PlaylistItem( m_listView, m_after, u, s, i ); m_after = item;
}


int PlaylistLoader::isPlaylist( const QString &path )  //static
{
    const QString ext = path.right( 4 ).lower();

    if ( ext == ".m3u" ) return 1;
    else if ( ext == ".pls" ) return 2;
    else if ( ext == ".xml" ) return 3;
    else return 0;
}


void PlaylistLoader::loadLocalPlaylist( const QString &path, int type )
{
    QFile file( path );

    if ( file.open( IO_ReadOnly ) ) {
        QTextStream stream( &file );

        switch ( type )

        {
                case 1: {
                    loadM3U( stream, path.left( path.findRev( '/' ) + 1 ) ); //TODO verify that relative playlists work!!
                    break;
                }
                case 2:
                loadPLS( stream );
                break;
                case 3:
                loadXML( stream );
                break;
                default:
                break;
        }
    }

    file.close();
}


bool PlaylistLoader::isValidMedia( const KURL &url, mode_t mode, mode_t permissions )  //static
{
    //FIXME determine if the thing at the end of this is a stream! Can arts do this?
    //      currently we always return true as we can't check
    //FIXME I don't actually understand what checks can be done, etc.
    
    // Accept non-local files, since we can't test them for validity at this point
    if ( !url.isLocalFile() ) return true;

    //TODO  if the playback fails, do some tests and tell the user why - we want to be the best!

    QString ext;
    int index = url.fileName().findRev( "." );

    if ( index > 0 ) {
        ext = url.fileName().mid( index + 1 ).lower();
        if ( Playlist::s_extensionCache.contains( ext ) ) {
            // Look up extension in cache. Not perfect, but it's currently the best compromise.
            return Playlist::s_extensionCache[ ext ];
        }
    }
    bool valid = EngineController::engine() ->canDecode( url );
    // Cache this result for the next lookup
    if ( !ext.isEmpty() )
        Playlist::s_extensionCache.insert( ext, valid );

    return valid;
}


#ifdef FAST_TRANSLATE
void PlaylistLoader::translate( QString &path, const QCString &encodedPath )
#else
void PlaylistLoader::translate( QString &path, KFileItemList &list )
#endif
{
#ifdef FAST_TRANSLATE
    QStringList directories;
    QStringList files;
    DIR *d = opendir( encodedPath );
#else
    DIR *d = opendir( QFile::encodeName( path ) );
#endif


    if ( !path.endsWith( "/" ) ) path += '/';

    if ( d ) {
        DIRENT * ent;
        struct STATSTRUCT statbuf;

        while ( ( ent = READDIR( d ) ) ) {
            const QString file = QFile::decodeName( ent->d_name );
            if ( file == "." || file == ".." ) continue;

            const QString newPath( path + file );
            const QCString localePath = QFile::encodeName( newPath );

            //get file information
            if ( LSTAT( localePath, &statbuf ) == 0 ) {
                //check for these first as they are not mutually exclusive WRT dir/files
                if ( S_ISCHR( statbuf.st_mode ) ||
                        S_ISBLK( statbuf.st_mode ) ||
                        S_ISFIFO( statbuf.st_mode ) ||
                        S_ISSOCK( statbuf.st_mode ) ); //then do nothing

                else if ( S_ISDIR( statbuf.st_mode ) && options.recurse )  //is directory
                {
                    if ( !options.symlink && S_ISLNK( statbuf.st_mode ) ) continue;
#ifdef FAST_TRANSLATE
                    directories += newPath;
#else
                    translate( newPath, list );
#endif
                }
                else if ( S_ISREG( statbuf.st_mode ) )  //is file
                {
                    KURL url; url.setPath( newPath ); //safe way to do it for unix paths

                    if ( isPlaylist( newPath ) )
                        //QApplication::postEvent( m_listView, new PlaylistFoundEvent( url ) );
                        ;
                    else
                    {
                        //we save some time and pass the stat'd information
                        if ( isValidMedia( url, statbuf.st_mode & S_IFMT, statbuf.st_mode & 07777 ) ) {
#ifdef FAST_TRANSLATE
                            files += file;
#else
                            //true means don't determine mimetype (waste of cycles for sure!)
                            list.append( new KFileItem( statbuf.st_mode & S_IFMT, statbuf.st_mode & 07777, url, true ) );
#endif

                        }
                    }
                }
            } //if( LSTAT )
        } //while

        closedir( d );

#ifdef FAST_TRANSLATE
        //alpha-sort the files we found, and then post them to the playlist
        files.sort();
        const QStringList::ConstIterator end = files.end();
        for ( QStringList::ConstIterator it = files.begin(); it != end; ++it ) {
            QString file = path; file += *it;
            KURL url; url.setPath( file );
            postBundle( url );
        }

        {   //translate all sub-directories
            const QStringList::Iterator end = directories.end();
            for ( QStringList::Iterator it = directories.begin(); it != end; ++it ) {
                translate( *it, QFile::encodeName( ( *it ) ) ); //FIXME cache QCStrings from above too
            }
        }
#endif

    } //if( d )
}


void PlaylistLoader::loadM3U( QTextStream &stream, const QString &dir )
{
    QString str, title;
    int length = MetaBundle::Undetermined; // = -2

    while ( !( str = stream.readLine() ).isNull() ) {
        if ( str.startsWith( "#EXTINF" ) ) {
            QString extinf = str.section( ':', 1, 1 );
            length = extinf.section( ',', 0, 0 ).toInt();
            title = extinf.section( ',', 1, 1 );

            if ( length == 0 ) length = MetaBundle::Undetermined;
        }
        else if ( !str.startsWith( "#" ) && !str.isEmpty() ) {
            if ( !( str[ 0 ] == '/' || str.startsWith( "http://" ) ) )
                str.prepend( dir );

            KURL url = KURL::fromPathOrURL( str );
            postBundle( url, title, length );

            length = MetaBundle::Undetermined;
            title = QString();
        }
    }
}


void PlaylistLoader::loadPLS( QTextStream &stream )
{
    //FIXME algorithm works, but is rather pants!

    for ( QString line = stream.readLine(); !line.isNull(); line = stream.readLine() ) {
        if ( line.startsWith( "File" ) ) {
            KURL url = KURL::fromPathOrURL( line.section( "=", -1 ) );
            QString title;
            int length = 0;

            line = stream.readLine();

            if ( line.startsWith( "Title" ) ) {
                title = line.section( "=", -1 );
                line = stream.readLine();
            }

            if ( line.startsWith( "Length" ) ) {
                length = line.section( "=", -1 ).toInt();
            }

            postBundle( url, title, length );
        }
    }
}

#include <qdom.h>
void PlaylistLoader::loadXML( QTextStream &stream )
{
    stream.setCodec( QTextCodec::codecForName( "utf8" ) );
    QDomDocument d;
    if ( !d.setContent( stream.read() ) ) { kdDebug() << "Could not load XML\n"; return ; }

    QDomNode n = d.namedItem( "playlist" ).firstChild();

    const QString ITEM( "item" ); //so we don't construct the QStrings all the time
    const QString URL( "url" );
    const QString s = "%1 %2 %3";

    while ( !n.isNull() && n.nodeName() == ITEM ) {
        const QDomElement e = n.toElement();
        if ( e.isNull() ) {
            kdDebug() << "Element '" << n.nodeName() << "' is null, skipping." << endl;
            continue;
        }

        //TODO  check this is safe, is it ok to cause paint Events from this thread?
        //TODO  if this is safe you may want to do it all like this
        //FIXME may be non-crash bugs due to non serialised access to m_after
        m_after = new PlaylistItem( m_listView, m_after, KURL( e.attribute( URL ) ), n );

        n = n.nextSibling();
    }
}


PlaylistItem*
PlaylistLoader::MakeItemEvent::makePlaylistItem( Playlist *pw )
{
    PlaylistItem * item = new PlaylistItem( pw, m_thread->m_after, m_url, m_title, m_length );
    m_thread->m_after = item;
    return item;
}

PlaylistItem*
PlaylistLoader::DownloadPlaylistEvent::makePlaylistItem( Playlist *lv )
{
    PlaylistItem * newItem = MakeItemEvent::makePlaylistItem( lv );

    //it is safe to dereference m_thread currently as LoaderThreads are deleted in the main Event Loop
    //and we are blocking the event loop right now!
    //however KIO::NetAccess processes the event loop, so do any m_thread dereferencing before the KIO call

    bool playFirstItem = m_thread->options.playFirstItem;
    m_thread->options.playFirstItem = false; //FIXME what if KIO fails?

    //KIO::NetAccess can make it's own tempfile
    //but we need to add .pls/.m3u extension or the Loader will fail
    QString path = m_url.filename();
    KTempFile tmpfile( QString::null, path.mid( path.findRev( '.' ) ) ); //use default prefix
    path = tmpfile.name();

    kdDebug() << "[PLSloader] Trying to download: " << m_url.prettyURL() << endl;

    //FIXME this will block user input to the interface AND process the event queue
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    bool succeeded = KIO::NetAccess::download( m_url, path, reinterpret_cast<QWidget*>( lv ) );
    QApplication::restoreOverrideCursor();

    if ( succeeded ) {
        kdDebug() << "[PLSloader] Download was successful!\n";

        //the playlist was successfully downloaded
        //KIO::NetAccess created a tempfile, it will be deleted in the new thread's dtor
        KURL url; url.setPath( path ); //required way to set unix paths
        const KURL::List list( url );

        //the item is treated as a placeholder with this ctor
        PlaylistLoader *loader = new PlaylistLoader( list, newItem );
        //pass on the playFirstItem flag to the new loader
        loader->options.playFirstItem = playFirstItem;

        loader->start();

        //FIXME may dereference what has already been deleted!!!! (NOT SAFE!)
        //TODO hide it instead of deleting it and set m_after before hand
        //m_thread->m_after = newItem;
    } else {
        KMessageBox::sorry( reinterpret_cast<QWidget*>( lv ), i18n( "The playlist, '%1', could not be downloaded." ).arg( m_url.prettyURL() ) );
        newItem->setVisible( false ); //FIXME you set m_thread->m_after to this so we can't delete it!
        tmpfile.unlink();
    }

    //we return 0 because we don't want the Playlist to register this item or try to read its tags
    return 0;
}
