//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#include "amarokfilelist.h"
#include "engine/enginebase.h"
#include "metabundle.h"
#include "playerapp.h"
#include "playlistitem.h"
#include "playlistloader.h"

#include <qapplication.h>  //postEvent()
#include <qtextstream.h>   //loadM3U(),loadPLS()
#include <qfile.h>         //~PlaylistLoader()

#include <kapplication.h>
#include <kcursor.h>       //TagReader::append()
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

//taglib
#include <taglib/tstring.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

/*
 * For pls and m3u specifications see: http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772
 */

//URGENT
//TODO store threads in a stack that can be emptied on premature program exit, or use one thread and a KURL::List stack
//TODO don't delete m_first, it may already have been removed! either make it unremovable or do something more intelligent

//LESS IMPORTANT
//TODO add non-local directories as items with a [+] next to, you open them by clicking the plus!! --maybe not
//TODO display dialog that lists unloadable media after thread is exited
//TODO undo/redo suckz0r as you can push both simultaneously and you get a list which is a mixture!
//     perhaps a static method that accepts a ListView pointer and loads playlists only would help speed
//     up undo/redo
//TODO stop blocking on netaccess::download()
//TODO recursion limits
//TODO either remove the option or always read metatags (also remove extra columns if you keep the option)
//TODO extract and bundle extra info from playlists (especially important for streams) (bundle it with
//     the URLs somehow and then allow replacement if metatags exist (?) )
//TODO consider loading the TagLib::AudioProperties on demand only as they are slow to load
//TODO rethink recursion options <mxcl> IMHO they suck big chunks, always do it recursively, why drop/add a
//     directory if you don't want its contents?
//     you can always open the dir and select the files. Stupid option and PITA to implement.
//     <markey> non-recursive adding should get replaced by "add all media files in current directory"
//TODO reimplement ask recursive in PlaylistWidget::insertMedia()
//TODO make translate work like process(), ie process isn't called afterwards AND it posts its own events whenever a valid file is found
//     <-- you'd have to do it one dir at a time


class PlayerApp;
extern PlayerApp *pApp;


PlaylistLoader::PlaylistLoader( const KURL::List &ul, QListView *lv, QListViewItem *lvi )
   : m_list( ul )
   , m_after( lvi )
   , m_first( 0 )
   , m_listView( lv )
{}


//this is a private ctor used by ::makePlaylistItem()
//it can only be used with placeholders
PlaylistLoader::PlaylistLoader( const KURL::List &ul, QListViewItem *lvi )
   : m_list( ul )
   , m_after( lvi )
   , m_first( lvi )
   , m_listView( lvi->listView() )
{}


PlaylistLoader::~PlaylistLoader()
{
    //for GUI access only

    if( NULL != m_first )
    {
        kdDebug() << "Unlinking tmpfile: " << m_list.first().path() << endl;
        QFile::remove( m_list.first().path() );
        delete m_first; //FIXME deleting m_first is dangerous as user may have done it for us!
    }

    kdDebug() << "[loader] Done!\n";
}


void PlaylistLoader::run()
{
       kdDebug() << "[loader] Started..\n";

       m_recursionCount = -1;
       process( m_list );

       QApplication::postEvent( m_listView, new LoaderDoneEvent( this ) );
}


void PlaylistLoader::process( const KURL::List &list, bool validate )
{
   struct STATSTRUCT statbuf;
   ++m_recursionCount;

   for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
   {
      QString path = (*it).path();

      if( validate && (*it).isLocalFile() )
      {
         if( LSTAT( path.local8Bit(), &statbuf ) != 0 ) continue;

         if( S_ISDIR( statbuf.st_mode ) )
         {
            //some options prevent recursion
            //FIXME depth check too
            if( list.count() > 1 && ( !options.recurse || ( !options.symlink && S_ISLNK( statbuf.st_mode ) ) ) ) continue;

            AmarokFileList files( options.sortSpec );
            files.setAutoDelete( true );
            translate( path, files );
            files.sort();

            KURL::List urls;
            for( KFileItemListIterator it( files ); *it; ++it )
            {
                urls << (*it)->url();
            }

            process( urls, false ); //false will prevent stating for dir, etc.
            continue;
         }
      }

      if( int type = isPlaylist( path.lower() ) )
      {
         if ( !m_recursionCount )     //prevent processing playlist files in subdirs
         {
            if( !(*it).isLocalFile() )
            {
                //if the playlist is not local, we need to d/l it, and KIO doesn't work in QThreads. sigh
                //so this will organise the d/l to occur syncronously and then a new thread spawned :)
                postDownloadRequest( *it );
            }
            else
            {
                loadLocalPlaylist( path, type );
            }
         }
      }
      else
      {
         if( validate && !isValidMedia( *it ) ) continue; //TODO retain stat info if done above, which does happen

         postBundle( *it );
      }
   }
}


inline
void PlaylistLoader::postDownloadRequest( const KURL &u )
{
    QApplication::postEvent( m_listView, new LoaderEvent( this, u ) );
}

inline
void PlaylistLoader::postBundle( const KURL &u, MetaBundle *mb )
{
    QApplication::postEvent( m_listView, new LoaderEvent( this, u, mb ) );
}

inline
int PlaylistLoader::isPlaylist( const QString &path ) const
{
   //TODO investigate faster methods
   //TODO try to achieve retVal optimisation

        if( path.endsWith( ".m3u", false ) ) return 1;
   else if( path.endsWith( ".pls", false ) ) return 2;
   else return 0;
}


void PlaylistLoader::loadLocalPlaylist( const QString &path, int type )
{
   kdDebug() << "[loader] playlist: " << path << endl;

   QFile file( path );

      if ( file.open( IO_ReadOnly ) )
      {
        QTextStream stream( &file );

        switch( type )
        {
        case 1:
           loadM3u( stream, path.left( path.findRev( '/' ) + 1 ) ); //TODO verify that relative playlists work!!
           break;
        case 2:
           loadPls( stream );
           break;
        default:
           break;
        }
      }

   file.close();
}


bool PlaylistLoader::isValidMedia( const KURL &url, mode_t mode, mode_t permissions ) const
{
   //FIXME determine if the thing at the end of this is a stream! Can arts do this?
   //      currently we always return true as we can't check
   //FIXME I don't actually understand what checks can be done, etc.
   if( url.protocol() == "http" ) return true;

   //FIXME KMimetype doesn't seem to like http files, so here we are assuming if
   //      it's extension is not common, it can't be read. Not perfect
   //      listed in order of liklihood of encounter to avoid logic checks
   QString ext = url.path().right( 4 ).lower();
   bool b = ( ext == ".mp3" || ext == ".ogg" || ext == ".m3u" || ext == ".pls" || ext == ".mod" ||  ext == ".wav" );

   if( !b && !(b = pApp->m_pEngine->canDecode( url, mode, permissions )) )
       kdDebug() << "Rejected URL: " << url.prettyURL() << endl;

    return b;
}


void PlaylistLoader::translate( QString &path, KFileItemList &list )
{
   DIR *d = opendir( path.local8Bit() );
   if( !path.endsWith( "/" ) ) path += '/';

   if( d )
   {
      DIRENT *ent;
      struct STATSTRUCT statbuf;

      while( ( ent = READDIR( d ) ) )
      {
         QString file( ent->d_name );

         if( file == "." || file == ".." ) continue;

         QString newpath = path + ent->d_name;

         //get file information
         if( LSTAT( newpath.local8Bit(), &statbuf ) == 0 )
         {
            //check for these first as they are not mutually exclusive WRT dir/files
            if( S_ISCHR(  statbuf.st_mode ) ||
                S_ISBLK(  statbuf.st_mode ) ||
                S_ISFIFO( statbuf.st_mode ) ||
                S_ISSOCK( statbuf.st_mode ) ); //then do nothing

            else if( S_ISDIR( statbuf.st_mode ) && options.recurse )  //directory
            {
               if( !options.symlink && S_ISLNK( statbuf.st_mode ) ) continue;
               translate( newpath, list );
            }

            else if( S_ISREG( statbuf.st_mode ) )  //file
            {
               KURL url; url.setPath( newpath ); //apparently this is the safe way to do it for unix paths

               //we save some time and pass the stat'd information
               if( isValidMedia( url, statbuf.st_mode & S_IFMT, statbuf.st_mode & 07777 ) )
               {
                  //true means don't determine mimetype (waste of cycles for sure!)
                  list.append( new KFileItem( statbuf.st_mode & S_IFMT, statbuf.st_mode & 07777, url, true ) );
               }
            }
         } //if( LSTAT )
      } //while

      closedir( d );
   } //if( d )
}


void PlaylistLoader::loadM3u( QTextStream &stream, const QString &dir )
{
    QString str, title;
    int length = 0;

    while ( !( str = stream.readLine() ).isNull() )
    {
        if ( str.startsWith( "#EXTINF" ) )
        {
            length = str.find( ':' );
            length = str.mid( length, str.find( ',' ) - length ).toInt();
            title  = str.section( ",", 1 );
        }

        else if ( !str.startsWith( "#" ) && !str.isEmpty() )
        {
            if ( !( str[0] == '/' || str.startsWith( "http://" ) ) )
                str.prepend( dir );

            postBundle( KURL::fromPathOrURL( str ), ( length != 0 ) ? new MetaBundle( title, length ) : 0 );

            length = 0;
        }
    }
}


void PlaylistLoader::loadPls( QTextStream &stream )
{
    //FIXME algorithm works, but is rather pants!

    for( QString line = stream.readLine(); !line.isNull(); line = stream.readLine() )
    {
        if( line.startsWith( "File" ) )
        {
            KURL url = KURL::fromPathOrURL( line.section( "=", -1 ) );
            QString title;
            int length = 0;
            MetaBundle *tags = 0;

            line = stream.readLine();

            if( line.startsWith( "Title" ) )
            {
                title = line.section( "=", -1 );
                line = stream.readLine();
            }

            if( line.startsWith( "Length" ) )
            {
                length = line.section( "=", -1 ).toInt();
            }

            if( title != "" || length > 0 )
            {
                tags = new MetaBundle( title, length );
            }

            postBundle( url, tags );
        }
    }
}




PlaylistLoader::LoaderEvent::~LoaderEvent()
{
   delete m_tags;
}


PlaylistItem *PlaylistLoader::LoaderEvent::makePlaylistItem( QListView *lv )
{
   //This function must be called from the GUI!

   //Construct a PlaylistItem and update the after pointer
   //Providing access is limited to the GUI thread, writes to m_after are serialised

   PlaylistItem *newItem = new PlaylistItem( lv, m_thread->m_after, m_url, m_tags );

   if( m_kio )
   {
       //it is safe to dereference m_thread currently as LoaderThreads are deleted in the main Event Loop
       //and we are blocking the event loop right now!
       //however KIO::NetAccess processes the event loop, so we need to dereference now in case the thread is deleted

      //KIO::NetAccess can make it's own tempfile
      //but we need to add .pls/.m3u extension or the Loader will fail
      QString path = m_url.filename();
      int i = path.findRev( '.' );
      //FIXME KTempFile should default to the suffix "tmp", not "", thus allowing you to have no prefix
      //      if you so desire. Bad design needs you to fix it!
      KTempFile tmpfile( QString::null, path.right( i ) ); //default prefix
      path = tmpfile.name();

      kdDebug() << "[loader] KIO::download - " << path << endl;

      //FIXME this will block user input to the interface and process the event queue
      QApplication::setOverrideCursor( KCursor::waitCursor() );
         bool succeeded = KIO::NetAccess::download( m_url, path, lv );
      QApplication::restoreOverrideCursor();

      if( succeeded )
      {
         //the playlist was successfully downloaded
         //KIO::NetAccess created a tempfile, it will be deleted in the new thread's dtor
         KURL url; url.setPath( path ); //required way to set unix paths
         const KURL::List list( url );

         //FIXME set options?
         PlaylistLoader *loader = new PlaylistLoader( list, newItem ); //the item is treated as a placeholder with this ctor
         loader->start();

         //FIXME may dereference what has already been deleted!!!! (NOT SAFE!)
         //TODO hide it instead of deleting it and set m_after before hand
         //m_thread->m_after = newItem;
      }
      else
      {
         KMessageBox::sorry( lv, i18n( "The playlist, '%1', could not be downloaded." ).arg( m_url.prettyURL() ) );
         delete newItem; //we created this in this function, thus it's safe to delete!
         tmpfile.unlink();
      }

      //we return 0 because we don't want the Playlist to register this item or try to read its tags
      return 0;
   }
   else
   {
      m_thread->m_after = newItem;
      return newItem;
   }
}



// TagReader ===============

void TagReader::append( PlaylistItem *item )
{
   //for GUI access only
   //we're a friend of PlaylistItem

   //TODO taglib isn't network transparent
   if( item->url().protocol() == "file" )
   {
      //QDeepCopy<QString> url = item->url().path();
      Bundle bundle( item, item->url() );

      mutex.lock();
      m_Q.push_back( bundle );
      mutex.unlock();

      if( !running() )
      {
          start( QThread::LowestPriority );
          //m_parent->setCursor( KCursor::workingCursor() );
          QApplication::setOverrideCursor( KCursor::workingCursor() );
      }
   }
}


void TagReader::run()
{
    MetaBundle *tags;

    msleep( 200 ); //this is an attempt to encourage the queue to be filled with more than 1 item before we
                   //start processing, and thus prevent unecessary stopping and starting of the thread

    kdDebug() << "[reader] Started..\n";

    while( m_bool )
    {
        mutex.lock();
        if( m_Q.empty() ) { mutex.unlock(); break; } //point of loop exit is here
        Bundle bundle( m_Q.front() );
        mutex.unlock();

        tags = readTags( bundle.url ); //rate-limiting step

        //we need to check the item is still there
        //if the playlistItem was removed it will no longer be in the queue
        mutex.lock();
        if( !m_Q.empty() && m_Q.front() == bundle )
        {
            QApplication::postEvent( m_parent, new TagReaderEvent( bundle.item, tags ) );
            m_Q.pop_front();
        }
        mutex.unlock();
    }
    kdDebug() << "[reader] Done!\n";


    QApplication::postEvent( m_parent, new QCustomEvent( TagReader::Done ) );
}


MetaBundle *TagReader::readTags( const KURL &url ) //STATIC
{
   MetaBundle *tags = 0;

   //audioproperties are read on demand
   TagLib::FileRef f( url.path().local8Bit(), true );

   if ( !f.isNull() )
   {
      //it is my impression from looking at the source that tag() never returns 0
      const TagLib::Tag *tag = f.tag();

      tags = new MetaBundle( TStringToQString( tag->title() ).stripWhiteSpace(),
                       TStringToQString( tag->artist() ).stripWhiteSpace(),
                       TStringToQString( tag->album() ).stripWhiteSpace(),
                       ( tag->year() == 0 ) ? QString() : QString::number( tag->year() ),
                       TStringToQString( tag->comment() ).stripWhiteSpace(),
                       TStringToQString( tag->genre() ).stripWhiteSpace(),
                       ( tag->track() == 0 ) ? QString() : QString::number( tag->track() ),
                       f.audioProperties() );
   }

   return tags;
}


void TagReader::cancel()
{
   mutex.lock();
   m_Q.clear();
   mutex.unlock();

   //this is because currently, tagreader 98% of the time has sent events for playlistitems to be deleted
   //by processing events you process these playlistItem events and then after this function theory are deleted
   //FIXME delay deletion of the items instead (use an event to do it instead)
   kapp->processEvents();
}


void TagReader::remove( PlaylistItem *pi )
{
   //thread safe removal of above item, called when above item no longer needs tags, ie is about to be deleted

   mutex.lock();
   m_Q.remove( Bundle( pi, KURL() ) );
   mutex.unlock();
}



TagReader::TagReaderEvent::~TagReaderEvent()
{
    delete m_tags;
}

void
TagReader::TagReaderEvent::bindTags()
{
   //for GUI access only
   //we're a friend of PlaylistItem
   if( m_tags )
   {
       m_item->setMeta( *m_tags );
   }
}

void
TagReader::TagReaderEvent::addSearchTokens( QStringList &tokens, QPtrList<QListViewItem> &ptrs )
{
    //for GUI access only
    //we need to add this item to the search-index
    QString
    s  = m_tags->m_artist;
    s += ' ';
    s += m_tags->m_title;
    s += ' ';
    s += m_item->trackName();

    tokens.append( s );
    ptrs.append( m_item );
}
