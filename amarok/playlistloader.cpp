//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#include "playlistitem.h" //for Tags struct
#include "playlistloader.h"

#include <qapplication.h>
#include <qtextstream.h>
#include <qfile.h>

#include <kapplication.h>
#include <kfileitem.h>
#include <kurl.h>
#include <kdebug.h> //remove when you can
#include <ktempfile.h>
#include <kio/netaccess.h>

//taglib
#include <fileref.h>
#include <tag.h>





///// ctor and dtor are basic and in header

/*
 * For pls and m3u specifications see: http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772
 *
 */

//URGENT
//TODO store threads in a stack that can be emptied on premature program exit
//TODO get setCurrentTrack to insert an item if it's not found, document that it is done after all thread
//     operations are complete
//     also you can get the customEvent function to check if new items are current if current request is
//     sitting there
//TODO loaders with one directory with no entries seem to not delete themselves
//TODO don't delete m_first, it may already have been removed! either make it unremovable or do something more intelligent

//FIXME CRASH with URL::List iterator in process
/*
20:23 < Markey> |[New Thread 1024 (LWP 4629)]
20:23 < Markey> |[New Thread 2049 (LWP 4630)]
20:23 < Markey> |[New Thread 7170 (LWP 4686)]
20:23 < Markey> |0x420bb449 in wait4 () from /lib/libc.so.6
20:23 < Markey> |#0  0x420bb449 in wait4 () from /lib/libc.so.6
20:23 < Markey> |#1  0x42137fd0 in __DTOR_END__ () from
                /lib/libc.so.6
20:23 < Markey> |#2  0x41ee0a73 in waitpid () from
                /lib/libpthread.so.0
20:23 < Markey> |#3  0x41405f6d in
                KCrash::defaultCrashHandler(int) ()
20:23 < Markey> |   from /opt/kde3/lib/libkdecore.so.4
20:23 < Markey> |#4  0x41ede11b in pthread_sighandler () from
                /lib/libpthread.so.0
20:23 < Markey> |#5  <signal handler called>
20:23 < Markey> |#6  0x080711d3 in
                QValueListIterator<KURL>::operator++(int)
                (this=0x82b8660)
20:23 < Markey> |    at /usr/lib/qt3/include/qvaluelist.h:121
20:23 < Markey> |#7  0x0807110a in QValueListPrivate
                (this=0x8279728, _p=@0x82b8660)
20:23 < Markey> |    at /usr/lib/qt3/include/qvaluelist.h:272
20:23 < Markey> |#8  0x08070e47 in
                QValueList<KURL>::detachInternal()
                (this=0x82c059c)
20:23 < Markey> |    at /usr/lib/qt3/include/qvaluelist.h:629
20:23 < Markey> |#9  0x08070bb3 in QValueList<KURL>::detach()
                (this=0x82c059c)
20:23 < Markey> |    at /usr/lib/qt3/include/qvaluelist.h:561
20:23 < Markey> |#10 0x08089831 in QValueList<KURL>::begin()
                (this=0x82c059c)
20:23 < Markey> |    at /usr/lib/qt3/include/qvaluelist.h:473
20:23 < Markey> |#11 0x080883b0 in
                PlaylistLoader::process(KURL::List&, bool)
                (this=0x82c0590,
20:23 < Markey> |    list=@0x82c059c, bTranslate=true) at
                amarok/playlistloader.cpp:148
20:23 < Markey> |#12 0x0808833a in PlaylistLoader::run()
                (this=0x82c0590)
20:24 < Markey> |    at amarok/playlistloader.cpp:113
20:24 < Markey> |#13 0x417643a5 in
                QThreadInstance::start(void*) ()
20:24 < Markey> |   from /usr/lib/qt3/lib/libqt-mt.so.3
20:24 < Markey> |#14 0x41edb1b0 in pthread_start_thread () from
                /lib/libpthread.so.0

//I got one too:

[New Thread 16384 (LWP 1601)]
[New Thread 32769 (LWP 1602)]
[New Thread 16386 (LWP 1603)]
0x412b8ab6 in waitpid () from /lib/i686/libpthread.so.0
#0  0x412b8ab6 in waitpid () from /lib/i686/libpthread.so.0
#1  0x4291a010 in KCrash::defaultCrashHandler(int) ()
   from /opt/kde3/lib/libkdecore.so.4
#2  0x412b796c in __pthread_sighandler () from /lib/i686/libpthread.so.0
#3  <signal handler called>
#4  0x42605612 in QString::operator=(QString const&) ()
   from /usr/lib/qt3/lib/libqt-mt.so.3
#5  0x4291fab6 in KURL::operator=(KURL const&) ()
   from /opt/kde3/lib/libkdecore.so.4
#6  0x4291c743 in KURL::KURL(KURL const&) () from /opt/kde3/lib/libkdecore.so.4
#7  0x0806e352 in QValueListPrivate<KURL>::insert(QValueListIterator<KURL>, KURL const&) (this=0x80be0a8, it={node = 0x81c9c50}, x=@0x0) at qvaluelist.h:289
#8  0x0806e850 in QValueListPrivate (this=0x80be0a8, _p=@0x405b7cbc)
    at qvaluelist.h:272
#9  0x0806e5b5 in QValueList<KURL>::detachInternal() (this=0x81c9c9c)
    at qvaluelist.h:629kdDebug() << "Shutting down TagReader Thread..\n";
#10 0x080886da in QValueList<KURL>::begin() (this=0x81c9c9c)
    at qvaluelist.h:473
#11 0x08086af1 in PlaylistLoader::process(KURL::List&, bool) (this=0x81c9c90, 
    list=@0x81c9c9c, validate=true) at playlistloader.cpp:150
#12 0x08086a90 in PlaylistLoader::run() (this=0x81c9c90)
    at playlistloader.cpp:122
#13 0x422ce4a5 in QThreadInstance::start(void*) ()
   from /usr/lib/qt3/lib/libqt-mt.so.3
#14 0x412b1f60 in pthread_start_thread () from /lib/i686/libpthread.so.0
#15 0x410f7327 in clone () from /lib/i686/libc.so.6

*/

//LESS IMPORTANT
//TODO add non-local directories as items with a [+] next to, you open them by clicking the plus!!
//TODO display dialog that lists unloadable media after thread is exited
//TODO undo/redo suckz0r as you can push both simultaneously and you get a list which is a mixture!
//     perhaps a static method that accepts a ListView pointer and loads playlists only would help speed
//     up undo/redo
//TODO stop blocking on netaccess::download()
//TODO delete temporary playlist files
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


void PlaylistLoader::run()
{
       kdDebug() << "[loader] Starting thread..\n";

       process( m_list );

       QApplication::postEvent( m_parent, new LoaderDoneEvent( this ) );
}


#include <dirent.h>   //dirent
#include <sys/stat.h> //lstat()

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

void PlaylistLoader::process( KURL::List &list, bool validate )
{
   struct STATSTRUCT statbuf;

   for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
   {
      QString path = (*it).path();

      if( validate && (*it).isLocalFile() )
      {
         if( LSTAT( path, &statbuf ) != 0 ) continue;

         if( S_ISDIR( statbuf.st_mode ) )
         {
            //some options prevent recursion
            if( list.count() > 1 && ( !options.recurse || ( !options.symlink && S_ISLNK( statbuf.st_mode ) ) ) ) continue; //FIXME depth check too

            KURL::List list2;
            translate( path, list2 );
            process( list2, false ); //false will prevent checking for dir, etc.
            continue;
         }
      }

      if( int type = isPlaylist( path.lower() ) )
      {
         if( !(*it).isLocalFile() )
         {
           //if the playlist is not local, we need to d/l it, and KIO doesn't work in QThreads. sigh
           //so this will organise the d/l to occur syncronously and then a new thread spawned :)
           QApplication::postEvent( m_parent, new LoaderEvent( this, *it ) );
         }
         else
         {
            loadLocalPlaylist( path, type );
         }
      }
      else
      {
         if( validate && !isValidMedia( *it ) ) continue; //TODO retain stat info if done above, which does happen

         Tags *meta = 0;
/*
         if( options.meta )
         {
            //TODO can we use filerefs instead of stating above? may shave a few ms

            TagLib::FileRef f( path, false ); //false = don't read audioproperties

            if ( !f.isNull() && f.tag() )
            {
               TagLib::Tag * tag = f.tag();

               meta = new Tags( TStringToQString( tag->title() ).stripWhiteSpace(),
                                TStringToQString( tag->artist() ).stripWhiteSpace(),
                                TStringToQString( tag->album() ).stripWhiteSpace(),
                                TStringToQString( tag->genre() ).stripWhiteSpace(),
                                TStringToQString( tag->comment() ).stripWhiteSpace(),
                                QString::number( tag->year() ),
                                QString::number( tag->track() ),
                                QString( (*it).directory().section( '/', -1 ) ),
                                f.audioProperties() );
            }
         }
*/
         //don't use the 2 parameter ctor of LoaderEvent
         QApplication::postEvent( m_parent, new LoaderEvent( this, *it, meta ) );
      }
   }
}


inline
int PlaylistLoader::isPlaylist( const QString &path )
{
   //TODO case insensitive endsWith exists in Qt3.2
   //TODO investigate faster methods
   //TODO try to achieve retVal optimisation

        if( path.endsWith( ".m3u" ) ) return 1;
   else if( path.endsWith( ".pls" ) ) return 2;
   else return 0;
}


void PlaylistLoader::loadLocalPlaylist( const QString &path, int type )
{
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




#include <arts/soundserver.h>

bool PlaylistLoader::isValidMedia( const KURL &url, mode_t mode, mode_t permissions )
{
   QString ext = url.path().right( 4 ).lower(); //FIXME 4 tmps

   //listed in order of liklihood of encounter to avoid logic checks
   bool b = ( ext == ".mp3" || ext == ".ogg" || ext == ".m3u" || ext == ".pls" || ext == ".mod" ||  ext == ".wav" );

   if( !b && url.protocol() != "http" )
   {
       //FIXME KMimetype doesn't seem to like http files, so here we are assuming if
       //      it's extension is not common, it can't be read. Not perfect

       KFileItem fileItem( mode, permissions, url, false ); //false = determineMimeType straight away
       KMimeType::Ptr mimeTypePtr = fileItem.determineMimeType();

       kdDebug() << fileItem.mimetype() << endl;

       Arts::TraderQuery query;
       query.supports( "Interface", "Arts::PlayObject" );
       query.supports( "MimeType", mimeTypePtr->name().latin1() );
       std::vector<Arts::TraderOffer> *offers = query.query();

       b = !offers->empty();

       delete offers;
    }

    return b;
}


void PlaylistLoader::translate( QString &path, KURL::List &list ) //FIXME KURL is pointless, pass a QString
{
   DIR *d = opendir( path );
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
         if( LSTAT( newpath, &statbuf ) == 0 )
         {
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
               KURL url( newpath );

               //we save some time and pass the stat'd information
               if( isValidMedia( url, statbuf.st_mode & S_IFMT, statbuf.st_mode & 07777 ) )
               {
                  list << url;
               }
            }
         } //if( LSTAT )
      } //while
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

        else if ( !str.startsWith( "#" ) )
        {
            if ( !( str[0] == '/' || str.startsWith( "http://" ) ) )
                str.prepend( dir );

            QApplication::postEvent( m_parent, new LoaderEvent( this, KURL( str ), ( length != 0 ) ? new Tags( title, length ) : 0 ) );

            length = 0;
        }
    }
}


void PlaylistLoader::loadPls( QTextStream &stream )
{
    QString line, title;
    KURL url;
    uint length = 0;
    bool posted = true;

    //FIXME algorithm works, but is rather pants!

    while ( !( line = stream.readLine() ).isNull() )
    {
        if( line.startsWith( "File" ) )
        {
           if( !posted )
           {
              Tags *tags = 0;

              if( length > 0 || title != "" )
              {
                 tags = new Tags( title, length );
              }

              QApplication::postEvent( m_parent, new LoaderEvent( this, url, tags ) );
           }

           url = line.section( "=", -1 );
           posted = false;
           title  = "";
           length = 0;
        }

        else if( line.startsWith( "Title" ) )
        {
           title = line.section( "=", -1 );
        }

        else if( line.startsWith( "Length" ) )
        {
           length = line.section( "=", -1 ).toInt();
        }
    }
}


PlaylistItem *PlaylistLoader::LoaderEvent::makePlaylistItem( QListView *lv )
{
   //construct a PlaylistItem and update the after pointer
   //hint: never call from inside the loader thread!
   //access from the GUI thread must be serialised
   PlaylistItem *newItem = new PlaylistItem( lv, m_thread->m_after, m_url, m_tags );

   if( m_kio )
   {
      QString path = m_url.filename();
      int i = path.findRev( '.' );
      //FIXME KTempFile should default to the suffix "tmp", not "", thus allowing you to have no prefix
      //      if you so desire. Bad design needs you to fix it!
      KTempFile tmpfile( QString::null, path.right( i ) ); //default prefix
      path = tmpfile.name();

      kdDebug() << "[loader] KIO::download - " << path << endl;

      //FIXME this seems to block the ui

      #if KDE_IS_VERSION(3,1,92)
      if( KIO::NetAccess::download( m_url, path, m_thread->m_parent ) ) //should be thread-safe as we are only reading it no?
      #else
      if( KIO::NetAccess::download( m_url, path ) )
      #endif
      {
         //we set true to ensure the place-holder (newItem) is deleted after processing
         PlaylistLoader *loader = new PlaylistLoader( KURL::List( KURL( path ) ), lv, newItem, true );
         loader->start();
         //KIO::NetAccess::removeTempFile( path );

      }
      else
      {
        delete newItem;
        return 0;
      }
   }

   m_thread->m_after = newItem;

   return newItem;
}



// TagReader ===============

//PRE-COMMIT URGENT!
//TODO read threading notes in Qt assistant to check you haven't made some silly error

//NORMAL
//TODO in cases where some tags are already set, keep them if there is no replacement
//TODO read audio tags on demand only?
//TODO tags can only be read by taglib over file: currently, so you should reject other protocols perhaps
//TODO audioproperties stuff
//TODO consider just using the Taglib::Tag itself? -- how would you copy playlist info?
//TODO it seems taglib returns a load of blank tags for all mp3s (? -- test this), this means we always have a tag object even though some tracks have nothing in them! Try to stop this

void TagReader::append( PlaylistItem *item )
{
   //for GUI access only
   //we're a friend of PlaylistItem

   //FIXME as far as I can tell,          b = !m_Q.empty();taglib requires the files to be local and file:/ as it doesn't accept KURLs
   if( item->url().protocol() == "file" )
   {
      //QDeepCopy<QString> url = item->url().path();
      Bundle bundle( item, item->url(), ( item->m_tags == 0 ) ? 0 : new Tags( *item->m_tags ) );

      mutex.lock();
      m_Q.push_back( bundle );
      mutex.unlock();

      if( !running() ) start( QThread::LowestPriority );
   }
}

void TagReader::run()
{
   Tags *tags;
   
   msleep( 100 ); //this is an attempt to encourage the queue to be filled with more than 1 item before we
                    //start processing, and thus prevent unecessary stopping and starting of the thread

      while( m_bool )
      {
         mutex.lock();
	 if( m_Q.empty() ) { mutex.unlock(); break; }
	 Bundle bundle( m_Q.front() );
         mutex.unlock();

         tags = readTags( bundle.url, bundle.tags );

         //we need to check the item is still there, if the playlistItem was removed, it will no longer be in
         //the queue
         mutex.lock();
         if( ( !m_Q.empty() ) && m_Q.front() == bundle )
         {
            QApplication::postEvent( m_parent, new TagReaderEvent( bundle.item, tags ) );
            m_Q.pop_front();
         }
         mutex.unlock();
      }

      kdDebug() << "[tagReader] done!\n";
}

Tags *TagReader::readTags( const KURL &url, Tags *tags )
{
   //TODO use old tags if available
   delete tags;

   TagLib::FileRef f( url.path(), false ); //false = don't read audioproperties

   if ( !f.isNull() && f.tag() )
   {
      TagLib::Tag * tag = f.tag();

      tags = new Tags( TStringToQString( tag->title() ).stripWhiteSpace(),
                       TStringToQString( tag->artist() ).stripWhiteSpace(),
                       TStringToQString( tag->album() ).stripWhiteSpace(),
                       TStringToQString( tag->genre() ).stripWhiteSpace(),
                       TStringToQString( tag->comment() ).stripWhiteSpace(),
                       QString::number( tag->year() ),
                       QString::number( tag->track() ),
                       QString( url.directory().section( '/', -1 ) ),
                       f.audioProperties() );
   }
   else tags = 0;

   return tags;
}

void TagReader::cancel()
{
   //FIXME if an event was just sent amaroK will crash

   mutex.lock();
   m_Q.clear();
   mutex.unlock();
}

void TagReader::remove( PlaylistItem *pi )
{
   //FIXME if an event was just sent for any of these items then amaroK will crash (when they are deleted)
   //FIXME slow, but works
   //thread safe removal of above item, called when above item no longer needs tags, ie is about to be deleted

   mutex.lock();
   m_Q.erase( std::remove( m_Q.begin(), m_Q.end(), pi ), m_Q.end() );
   mutex.unlock();

}



void TagReader::TagReaderEvent::bindTags()
{
   //for GUI access only
   //we're a friend of PlaylistItem
   //if( NULL != m_item->m_tags ) delete m_item->m_tags;
   m_item->m_tags = this->m_tags;
   m_item->setMetaTitle();
}
