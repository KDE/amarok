//
// Author: Max Howell (C) Copyright 2003
//
// Copyright: See COPYING file that comes with this distribution
//

#include "playlistitem.h" //for Tags struct
#include "playlistloader.h"
#include "playlistwidget.h"

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
extern PlayerApp *pApp;


///// ctor and dtor are basic and in header

//URGENT
//TODO instead of deleting m_first, change the item texts or at the very least delete it outside the loader thread!
//TODO ensure that thread still lives until last event it sends is processed since we depend on it (stupidly)
//TODO store threads in a stack that can be emptied on premature program exit
//TODO properly delete threads after use, currently you are being naughty!
//TODO get setCurrentTrack to insert an item if it's not found, document that it is done after all thread operations are complete
//     also you can get the customEvent function to check if new items are current if current request is sitting there

//LESS IMPORTANT
//TODO add non-local directories as items with a [+] next to, you open them by clicking the plus!!
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
//TODO reimplement ask recursive in PlaylistWidget::insertMedia()

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
      if( KIO::NetAccess::download( m_url, path, pApp->m_pBrowserWin ) )
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

void PlaylistLoader::process( KURL::List &list, bool bTranslate )
{
   struct STATSTRUCT statbuf;

   for( KURL::List::ConstIterator it = list.begin(); it != list.end(); ++it )
   {
      QString path = (*it).path();

      if( (*it).isLocalFile() && bTranslate )
      {
         if( LSTAT( path, &statbuf ) != 0 ) continue;

         if( S_ISDIR( statbuf.st_mode ) )
         {
            if( pApp->m_optDropMode != "Recursively" || ( !pApp->m_optFollowSymlinks && S_ISLNK( statbuf.st_mode ) && list.count() > 1 ) ) continue; //FIXME depth check too

            KURL::List list2;
            translate( *it, list2 );
            process( list2, false ); //FIXME inefficient as doubles # of stats (KFileItem uses stat)
            continue;
         }
      }

      if( int type = isPlaylist( path.lower() ) )
      {
         if( !(*it).isLocalFile() )
         {
           //if the playlist is not local, we need to d/l it, and KIO doesn't work in QThreads. sigh
           //so this will organise the d/l to occur syncronously and then a new thread spawned :)
           QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, *it ) );
         }
         else
         {
            loadLocalPlaylist( path, type );
         }
      }
      else
      {
         Tags *meta = 0;

         if( pApp->m_optReadMetaInfo )
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

         QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, *it, meta ) );
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
           loadM3u( stream, path.left( path.findRev( '/' ) ) ); //TODO verify that relative playlists work!!
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


void PlaylistLoader::loadM3u( QTextStream &stream, const QString &dir )
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

            QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, KURL( str ), 0 ) );

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


void PlaylistLoader::loadPls( QTextStream &stream )
{
    QString str;

    while ( !( str = stream.readLine() ).isNull() )
    {
        if ( str.startsWith( "File" ) )
        {
            //destItem = addItem( destItem, str.section( "=", -1 ) );
            KURL url( str.section( "=", -1 ) );

            QApplication::postEvent( m_parent, new PlaylistEvent( &m_after, url, 0 ) );

            /*
            //FIXME: no regressions ok?

            str = stream.readLine();

            if ( str.startsWith( "Title" ) )
                destItem->setText( 0, str.section( "=", -1 ) );
            */
        }
    }
}
