// Author: Max Howell (C) Copyright 2003
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#include <dirent.h>      //::recurse()
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include <qapplication.h>
#include <qdom.h>
#include <qfile.h>       //::loadPlaylist()
#include <qfileinfo.h>
#include <qlistview.h>
#include <qtextstream.h> //::loadPlaylist()
#include "statusbar.h"
#include <sys/stat.h>

#include "playlist.h"

PlaylistLoader* PlaylistLoader::s_instance = 0;

PlaylistLoader::PlaylistLoader( const KURL::List &urls, QListView *parent, QListViewItem *after )
    : QThread()
    , m_markey( parent ? new PlaylistItem( KURL(), parent, after ) : 0 )
    , m_URLs( urls )
    , m_stop( false )
{
    s_instance = this;
}

PlaylistLoader::~PlaylistLoader()
{
    s_instance = 0;
}

void
PlaylistLoader::run()
{
    m_markey->setVisible( false );

    amaroK::StatusBar::startProgress();
    QApplication::postEvent( Playlist::instance(), new QCustomEvent( Started ) );

    const KURL::List::ConstIterator end = m_URLs.end();

    for( KURL::List::ConstIterator it = m_URLs.begin(); it != end && !m_stop; ++it )
    {
        const KURL &url = *it;

        if( url.isLocalFile() )
        {
            const QString path = url.path();

            if( QFileInfo( path ).isDir() ) { recurse( path ); continue; }

            if( loadPlaylist( path ) ) continue;

            if( EngineController::canDecode( url ) ) createPlaylistItem( url );

            else addBadURL( url );
        }
        else if( isPlaylist( url ) )
        {
            //TODO
        }
        else if( EngineController::canDecode( url ) ) createPlaylistItem( url );
        else addBadURL( url );
    }

    delete m_markey;


    //TODO dialog for failed entries

    const float increment = 100.0 / m_pairs.count();
    const List::ConstIterator end2 = m_pairs.end();
    float progress = 0;
    for( List::ConstIterator it = m_pairs.begin(); it != end2 && !m_stop; ++it )
    {
        if ( (*it).first.isLocalFile() )
            QApplication::postEvent( Playlist::instance(), new TagsEvent( (*it).first, (*it).second ) );

        progress += increment;
        amaroK::StatusBar::showProgress( uint(progress) );
    }

    amaroK::StatusBar::stopProgress();
    QApplication::postEvent( Playlist::instance(), new DoneEvent( this ) );
}

inline void
PlaylistLoader::createPlaylistItem( const KURL &url )
{
    m_pairs.append( Pair(url, new PlaylistItem( url, m_markey )) );
}

void
PlaylistLoader::createPlaylistItem( const KURL &url, const QString &title, const uint length )
{
    PlaylistItem *item = new PlaylistItem( url, m_markey );

    item->KListViewItem::setText( PlaylistItem::Title, title );
    item->KListViewItem::setText( PlaylistItem::Length, MetaBundle::prettyLength( length ) );

    m_pairs.append( Pair(url, item) );
}

void
PlaylistLoader::recurse( QString path )
{
    KURL url;
    DIR *d = opendir( QFile::encodeName( path ) );

    if( d )
    {
        QStringList dirs;
        QStringList files;
        struct stat statbuf;

        if( !path.endsWith( "/" ) ) path += '/';

        for( dirent *ent; ( ent = readdir( d ) ) && !m_stop; )
        {
            if ( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 ) continue;

            const QString file = QFile::decodeName( ent->d_name );
            const QString newPath = path + file;
            const QCString localePath = QFile::encodeName( newPath );

            //get file information
            if( lstat( localePath, &statbuf ) == 0 )
            {
                //check for these first as they are not mutually exclusive WRT dir/files
                if( S_ISCHR( statbuf.st_mode ) ||
                    S_ISBLK( statbuf.st_mode ) ||
                    S_ISFIFO( statbuf.st_mode ) ||
                    S_ISSOCK( statbuf.st_mode ) ) continue;

                if( S_ISDIR( statbuf.st_mode )/*FIXME && options.recurse */)  //is directory
                {
                    //FIXME if( !options.symlink && S_ISLNK( statbuf.st_mode ) ) continue;

                    dirs += newPath;
                }

                else if( S_ISREG( statbuf.st_mode ) )  //is file
                {
                    url.setPath( newPath ); //safe way to do it for unix paths

                    if ( EngineController::canDecode( url ) )
                        files += newPath;
                    else
                        addBadURL( url );
                }
            } //if( LSTAT )
        } //for

        closedir( d );

        //alpha-sort the files we found, and then post them to the playlist
        files.sort();
        const QStringList::ConstIterator end1 = files.end();
        for ( QStringList::ConstIterator it = files.begin(); it != end1; ++it ) {
            url.setPath( *it );
            createPlaylistItem( url );
        }

        const QStringList::Iterator end2 = dirs.end();
        for ( QStringList::Iterator it = dirs.begin(); it != end2; ++it )
            recurse( *it );
    } //if( d )
    else { url.setPath( path ); addBadURL( url ); }
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

        while( !( str = stream.readLine() ).isNull() && !m_stop )
        {
            if ( str.startsWith( "#EXTINF" ) )
            {
                QString extinf = str.section( ':', 1, 1 );
                length = extinf.section( ',', 0, 0 ).toInt();
                title = extinf.section( ',', 1, 1 );

                if ( length == 0 ) length = MetaBundle::Undetermined;
            }
            else if ( !str.startsWith( "#" ) && !str.isEmpty() )
            {

                if ( !( str[ 0 ] == '/' || str.startsWith( "http://" ) ) ) //FIXME how about ftp etc?
                    str.prepend( dir );

                createPlaylistItem( KURL::fromPathOrURL( str ), title, length );

                length = MetaBundle::Undetermined;
                title = QString();
            }
        }
        break;
    }
    case PLS:

        for( QString line = stream.readLine(); !line.isNull() && !m_stop; line = stream.readLine() )
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

                createPlaylistItem( url, title, length );
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
             !n.isNull() && n.nodeName() == ITEM && !m_stop;
             n = n.nextSibling() )
        {
            const QDomElement e = n.toElement();

            if ( !e.isNull() )
                new PlaylistItem( KURL(e.attribute( URL )), m_markey, n );
        }
    }
    default:
        ;
    } //switch

    return true;
}

#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <klocale.h>
void
PlaylistLoader::downloadPlaylist( const KURL &url, QListView *listView, QListViewItem *item )
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

        (new PlaylistLoader( KURL::List( url ), listView, item ))->start();

    } else {

        KMessageBox::sorry( listView, i18n( "<p>The playlist, <i>'%1'</i>, could not be downloaded." ).arg( url.prettyURL() ) );
        tmpfile.unlink();
    }
}
