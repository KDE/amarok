// Author: Max Howell (C) Copyright 2003
// Copyright: See COPYING file that comes with this distribution
//

///For pls and m3u specifications see:
///http://forums.winamp.com/showthread.php?s=dbec47f3a05d10a3a77959f17926d39c&threadid=65772

#include "collectiondb.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistloader.h"
#include "statusbar.h"

#include <qapplication.h>
#include <qdom.h>
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
    , m_marker( parent ? new PlaylistItem( KURL(), parent, after ) : 0 )
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

void
PlaylistLoader::run()
{
    m_marker->setVisible( false );

    float increment, progress;
    PlaylistItem* item = 0;
    m_needSecondPass = false;
    amaroK::StatusBar::startProgress();
    QApplication::postEvent( Playlist::instance(), new QCustomEvent( Started ) );

    //BEGIN first pass
    increment = 100.0 / m_URLs.count();
    progress = 0;
    const KURL::List::ConstIterator end = m_URLs.end();

    for( KURL::List::ConstIterator it = m_URLs.begin(); it != end && !s_stop; ++it )
    {
        const KURL &url = *it;

        if( recurse( url ) )
            continue;

        if( url.isLocalFile() && loadPlaylist( url.path() ) )
            continue;

        if( EngineController::canDecode( url ) )
            item = createPlaylistItem( url );
        else
            addBadURL( url );

        if ( item ) m_needSecondPass |= !item->inCollection() || !item->hasAudioproperties();
        progress += increment;
        amaroK::StatusBar::showProgress( uint(progress) );
    }
    //END first pass

    //TODO dialog for failed entries

    delete m_marker;

    //BEGIN second pass
    if ( m_needSecondPass )
    {
        increment = 100.0 / m_pairs.count();
        progress = 0;
        const List::ConstIterator end2 = m_pairs.end();
        for( List::ConstIterator it = m_pairs.begin(); it != end2 && !s_stop; ++it )
        {
            if ( (*it).first.isLocalFile() )
            {
                if ( (*it).second->inCollection() )
                {
                    if ( !(*it).second->hasAudioproperties() )
                    {
                        // Store audioproperties in database if not yet stored
                        TagsEvent* e = new TagsEvent( (*it).first, (*it).second );
                        m_db->addAudioproperties( e->bundle );
                        QApplication::postEvent( Playlist::instance(), e );
                    }
                }
                else
                    QApplication::postEvent( Playlist::instance(), new TagsEvent( (*it).first, (*it).second ) );
            }

            progress += increment;
            amaroK::StatusBar::showProgress( uint(progress) );
        }
    }
    //END second pass

    amaroK::StatusBar::stopProgress();
    QApplication::postEvent( Playlist::instance(), new DoneEvent( this ) );
}

PlaylistItem*
PlaylistLoader::createPlaylistItem( const KURL &url )
{
    MetaBundle bundle;
    PlaylistItem* item;

    // Get MetaBundle from Collection if it's already stored, otherwise read from disk
    if ( m_db->getMetaBundleForUrl( url.path(), &bundle ) )
        item = new PlaylistItem( url, m_marker, bundle );
    else
        item = new PlaylistItem( url, m_marker );

    if ( m_playFirstUrl ) {
        QApplication::postEvent( Playlist::instance(), new QCustomEvent( QEvent::Type(Play), item ) );
        m_playFirstUrl = false;
    }

    m_pairs.append( Pair(url, item) );

    return item;
}

void
PlaylistLoader::createPlaylistItem( const KURL &url, const QString &title, const uint length )
{
    PlaylistItem *item = createPlaylistItem( url );

    item->KListViewItem::setText( PlaylistItem::Title, title );
    item->KListViewItem::setText( PlaylistItem::Length, MetaBundle::prettyLength( length ) );

    m_pairs.append( Pair(url, item) );
}

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
            msleep( 100 );

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

        // Post files to the playlist
        const FileMap::ConstIterator end1 = files.end();
        for ( FileMap::ConstIterator it = files.begin(); it != end1; ++it ) {
            PlaylistItem* item = createPlaylistItem( it.data() );
            m_needSecondPass |= !item->inCollection() || !item->hasAudioproperties();
        }

        // Recurse folders
        const KURL::List::Iterator end2 = dirs.end();
        for ( KURL::List::Iterator it = dirs.begin(); it != end2; ++it )
            recurse( *it, true );

        return success;
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

                createPlaylistItem( KURL::fromPathOrURL( str ), title, length );

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
             !n.isNull() && n.nodeName() == ITEM && !s_stop;
             n = n.nextSibling() )
        {
            const QDomElement e = n.toElement();

            if ( !e.isNull() )
                new PlaylistItem( KURL(e.attribute( URL )), m_marker, n );
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

        (new PlaylistLoader( KURL::List( url ), listView, item, directPlay ))->start();

    } else {

        KMessageBox::sorry( listView, i18n( "<p>The playlist, <i>'%1'</i>, could not be downloaded." ).arg( url.prettyURL() ) );
        tmpfile.unlink();
    }
}
