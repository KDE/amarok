//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#include "playlistloader.h"
#include "playlistitem.h" //for Tags struct

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


//FIXME including playerapp.h sucks, we MUST MUST MUST make a separate header for options!
//FIXME this is also very bad as you access data the GUI thread may access simultaneously (:o)
//      what we need is an options struct/class and then I can pass a COPY in the thread ctor
#include <playerapp.h>
extern PlayerApp pApp;


///// ctor, dtor and run() are basic and in header

//URGENT
//TODO instead of deleting m_first, change the item texts or at the very least delete it outside the loader thread!
//TODO ensure that thread still lives until last event it sends is processed since we depend on it (stupidly)
//TODO store threads in a stack that can be emptied on premature program exit
//TODO properly delete threads after use, currently you are being naughty!
//TODO get setCurrentTrack to insert an item if it's not found, document that it is done after all thread operations are complete
//     also you can get the customEvent function to check if new items are current if current request is sitting there

//LESS IMPORTANT
//TODO display dialog that lists unloadable media after thread is exited
//TODO undo/redo suckz0r as you can push both simultaneously and you get a list which is a mixture!
//     perhaps a static method that accepts a ListView pointer and loads playlists only would help speed up undo/redo
//TODO stop blocking on netaccess::download()
//TODO delete temporary playlist files
//TODO recursion limits
//TODO either remove the option or always read metatags (also remove extra columns if you keep the option)
//TODO extract and bundle extra info from playlists (especially important for streams) (bundle it with the URLs somehow and then allow replacement if metatags exist (?) )
//TODO consider loading the TagLib::AudioProperties on demand only as they are slow to load
//TODO rethink recursion options <mxcl> IMHO they suck big chunks, always do it recursively, why drop/add a directory if you don't want its contents?
//     you can always open the dir and select the files. Stupid option and PITA to implement.


PlaylistItem *PlaylistLoader::PlaylistEvent::makePlaylistItem( QListView *lv )
{
   //construct a PlaylistItem and update the after pointer
   //hint: never call from inside the loader thread!
   PlaylistItem *newItem = new PlaylistItem( lv, *m_after, m_url, m_tags );

   if( m_kio )
   {
      QString path = m_url.filename();
      int i = path.findRev( '.' );
      //FIXME KTempFile should default to the suffix "tmp", not "", thus allowing you to have no prefix if you so desire. Bad design needs you to fix it!
      KTempFile tmpfile( QString::null, path.right( i ) ); //default prefix
      path = tmpfile.name();

      kdDebug() << "[loader] KIO::download - " << path << endl;

      //FIXME this seems to block the ui

      #if KDE_IS_VERSION(3,1,92)
      if( KIO::NetAccess::download( m_url, path, this ) )
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

   *m_after = newItem;

   return newItem;
}


void PlaylistLoader::run()
{
       kdDebug() << "[loader] Starting thread..\n";

       process( m_list );

       delete m_first; //delete place-holder, if was set //FIXME aaah! you have to do this in the GUI thread

       sleep( 20 ); //flimsy way to try to ensure all events are finished b4 deletion

       delete this;

       //QApplication::postEvent( m_parent, new DeleteMeEvent( this );
       //delete this;
}

void PlaylistLoader::process( KURL::List &list )
{
   for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
   {
      Tags *tags = 0;

      if( (*it).isLocalFile() )
      {
         KFileItem file( KFileItem::Unknown, KFileItem::Unknown, *it, true );

         if( file.isDir() )
         {
            if( pApp->m_optDropMode != "Recursively" || ( !pApp->m_optFollowSymlinks && file.isLink() && list.count() > 1 ) ) continue; //FIXME depth check too

            KURL::List list2;
            translate( file.url(), list2 );
            process( list2 ); //FIXME inefficient as doubles # of stats (KFileItem uses stat)
            continue;
         }
      }

      if( parsePlaylist( *it ) ) //will arrange for playlist to be parsed
      {
         continue;
      }
      else if( (*it).isLocalFile() )
      {
         TagLib::String str( QStringToTString( (*it).path() ) );
         TagLib::FileRef f( str.toCString() );

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
                             QString( (*it).directory().section( '/', -1 ) ),
                             f.audioProperties() );
         }
      }

      if( isValidMedia( *it ) )
      {
         QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, *it, tags ) );
      }
      else
      {
        kdDebug() << "[loader] Not valid file: " << (*it).prettyURL() << endl;
      }
   }
}


#include <arts/soundserver.h>

bool PlaylistLoader::isValidMedia( const KURL &url )
{
    bool b = true;

    if( url.isLocalFile() )
    {
       //FIXME we already gather this, so don't waste cycles
       KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url );
       KMimeType::Ptr mimeTypePtr = fileItem.determineMimeType();

       Arts::TraderQuery query;
       query.supports( "Interface", "Arts::PlayObject" );
       query.supports( "MimeType", mimeTypePtr->name().latin1() );
       std::vector<Arts::TraderOffer> *offers = query.query();

       b = !offers->empty();

       delete offers;
    }

    return b;
}


#include <dirent.h>   //dirent
#include <sys/stat.h> //lstat()

//some GNU systems don't support big files for some reason
#ifndef __USE_LARGEFILE64 //see dirent.h
 #define DIRENT dirent
 #define SCANDIR scandir
 #define STATSTRUCT stat
 #define LSTAT lstat
#else
 #define DIRENT dirent64
 #define SCANDIR scandir64
 #define STATSTRUCT stat64
 #define LSTAT lstat64
#endif


static int selector( struct DIRENT const *ent )
{
  //add more types?
  //could do an explicit arts check, (yuk! performance issues or what!)
  if( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 )
    return 0;

  return 1;
}

void PlaylistLoader::translate( const KURL &dir, KURL::List &list ) //FIXME KURL is pointless, pass a QString
{
/*
   //TODO KDirListerCache isn't thread safe, so I've had to go lower level and use GNU stat
   //     currently this doesn't matter as we only officially support file: and http: however
   //     eventually we'll need the ability to gather other protocol directory listings and
   //     this will need to be solved. Original KDirLister code follows:

   KDirLister dirlister( true );
   dirlister.openURL( dir, false, false ); // URL; keep = true, reload = true
   while ( !dirlister.isFinished() );

   QPtrList<KFileItem> fileList( dirlister.items() );
   KURL::List *urlList = new KURL::List;

   for( KFileItem *item = fileList.first(); item; item = fileList.next() )
      *urlList << item->url();

   return urlList;
*/

  QString path = dir.path();
  kdDebug() << "[loader] Translating: " << path << endl;

  if( !path.endsWith( "/" ) ) path += '/';

  struct DIRENT **eps;
  int n = SCANDIR( path, &eps, selector, /*alphasort*/ NULL );

  if( n > 0 )
  {
    struct STATSTRUCT statbuf;

    //loop over array of dirents
    for( int cnt = 0; cnt < n; ++cnt )
    {
      QString new_path = path + eps[cnt]->d_name;

      //get file information
      if( LSTAT( new_path, &statbuf ) == 0 )
      {
        if( S_ISLNK(  statbuf.st_mode ) || //FIXME, we have an option for links
          S_ISCHR(  statbuf.st_mode ) ||
          S_ISBLK(  statbuf.st_mode ) ||
          S_ISFIFO( statbuf.st_mode ) ||
          S_ISSOCK( statbuf.st_mode ) )
        {
          continue;
        }

        if( S_ISREG( statbuf.st_mode ) )  //file
          list << KURL( new_path );

        if( S_ISDIR( statbuf.st_mode ) )  //directory
        {
          translate( KURL( new_path ), list );
        }
      }
    }
  }
}


bool PlaylistLoader::parsePlaylist( const KURL &url )
{
   //TODO returns bool

   //TODO mime-based checks would be better, as long as we don't have to fuss with the implementation
   int type;
   QString path( url.path().lower() );

   //1. test for playlist-ness
        if( path.endsWith( ".m3u" ) ) type = 0;
   else if( path.endsWith( ".pls" ) ) type = 1;
   else return false;

   kdDebug() << "[loader] Parsing: " << url.prettyURL() << endl;

   KURL::List list;
   path = url.path();

   if ( !url.isLocalFile() )
   {
      //if the playlist is not local, we need to d/l it, and KIO doesn't work in QThreads. sigh
      //so this will organise the d/l to occur syncronously and then a new thread spawned :)
      QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, url ) );
   }
   else
   {
      QFile file( url.path() );

      if ( file.open( IO_ReadOnly ) )
      {
        QTextStream stream( &file );

        switch( type )
        {
        case 0:
           loadM3u( stream, url.directory( false ), list );
           break;
        case 1:
           loadPls( stream, url.directory( false ), list );
           break;
        default:
           break;
        }
      }

      file.close();
   }

   process( list );

   return true;
}


void PlaylistLoader::loadM3u( QTextStream &stream, const QString &dir, KURL::List &list )
{
    QString str, extStr;

    while ( !( str = stream.readLine() ).isNull() )
    {
        if ( str.startsWith( "#EXTINF" ) )
        {
            extStr = str.section( ",", 1 );
        }

        if ( !str.startsWith( "#" ) )
        {
            if ( !( str[0] == '/' || str.startsWith( "http://" ) ) )
                str.prepend( dir );

            list << KURL( str );

            /*
            //FIXME: what is this about?
            if ( !extStr.isEmpty() )
            {
                destItem->setText( 0, extStr );
                extStr = "";
            }
            */
        }
    }
}


void PlaylistLoader::loadPls( QTextStream &stream, const QString&, KURL::List &list )
{
    QString str;

    while ( !( str = stream.readLine() ).isNull() )
    {
        if ( str.startsWith( "File" ) )
        {
            //destItem = addItem( destItem, str.section( "=", -1 ) );
            KURL url( str.section( "=", -1 ) );
            list << url;

            /*
            //FIXME: no regressions ok?

            str = stream.readLine();

            if ( str.startsWith( "Title" ) )
                destItem->setText( 0, str.section( "=", -1 ) );
            */
        }
    }
}
