// (c) Max Howell 2004
// See COPYING file for licensing information

#include "playlistbrowser.h"
#include "playlist.h"
#include "playlistloader.h" //PlaylistBrowserItem ctor
#include "metabundle.h"    //paintCell()
#include "collectiondb.h"  //smart playlists
#include "threadweaver.h"  //PlaylistFoundEvent

#include <qevent.h>       //customEvent()
#include <qsplitter.h>
#include <qfile.h>
#include <qpainter.h>  //paintCell()
#include <qpixmap.h>    //paintCell()
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>  //openPlaylist()
#include <kiconloader.h>  //smallIcon
#include <klistview.h>
#include <qvbox.h>
#include <ktoolbar.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>  //removePlaylist() and renamePlaylist()
#include <kactioncollection.h>
#include <kaction.h>
#include <klocale.h>
#include <kurldrag.h>     //dragObject()

#include <stdio.h> //remove() and rename()


PlaylistBrowser *PlaylistBrowser::s_instance = 0;


PlaylistBrowser::PlaylistBrowser( const char *name )
    : QVBox( 0, name )
{
    setSpacing( 3 );
    setMargin( 4 );

    s_instance = this;
    
    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
   
    m_toolbar = new KToolBar( this );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );
    
    m_ac = new KActionCollection( this );
    KAction *open = new KAction( "Add playlist", "fileopen", 0, this, SLOT( openPlaylist() ), m_ac, "Open" );
    KAction *remove = new KAction( "Remove", "edittrash", 0, this, SLOT( removePlaylist() ), m_ac, "Remove" );
    KAction *rename = new KAction( "Rename", "editclear", 0, this, SLOT( renamePlaylist() ), m_ac, "Rename" );
    
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right
    open->plug( m_toolbar );
    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    remove->plug( m_toolbar );
    rename->plug( m_toolbar);
    
    m_splitter    = new QSplitter( Vertical, this );
    
    m_listview = new PlaylistListView( m_splitter );
    m_listview->addColumn( "Playlists" );
    m_listview->setSelectionMode( QListView::Single );
    m_listview->setSorting( -1 );  //sort by name
    //m_listview->setFullWidth( true );
    m_listview->setRootIsDecorated( true );
    
    m_smartlistview = new SmartPlaylistView( m_splitter );
    
    QString str = config->readEntry( "Splitter" );
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *m_splitter; //this sets the splitters position

    // signals and slots connections
    connect( m_listview, SIGNAL(rightButtonClicked ( QListViewItem *, const QPoint &, int ) ),
           this, SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( m_listview, SIGNAL( doubleClicked( QListViewItem *) ), 
           this, SLOT( loadPlaylist( QListViewItem * ) ) );
    connect( m_listview, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
           this, SLOT( renamePlaylist( QListViewItem*, const QString&, int ) ) );
    
    // load playlists         
    QStringList files = config->readListEntry( "Playlists" );
    loadPlaylists( files );
}


PlaylistBrowser::~PlaylistBrowser()
{
   KConfig *config = kapp->config();
   config->setGroup( "PlaylistBrowser" );
   
   // save the path of all playlists in playlist browser
   QStringList urls;
   QListViewItemIterator it( m_listview );  
   while( it.current() ) {
       PlaylistBrowserItem *item = (PlaylistBrowserItem *)it.current();
       if( item->isPlaylist() )
           urls += QString( ((PlaylistBrowserItem *)it.current())->url().path() );
       ++it;
   }
   config->writeEntry( "Playlists", urls);

   //save splitter position
   QString str; QTextStream stream( &str, IO_WriteOnly );
   stream << *m_splitter;
   config->writeEntry( "Splitter", str );
}


void PlaylistBrowser::loadPlaylists( QStringList files )
{

   if( files.count() ) {
       const QStringList::ConstIterator end  = files.constEnd();
       for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )
           if( QFile::exists( *it ) )
               new PlaylistBrowserItem( m_listview, m_listview->lastItem(), KURL(*it) );
   } 
   
}


void PlaylistBrowser::openPlaylist() //SLOT
{
   // open a file selector to add playlists to playlist browser
   QStringList files;
   files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|Playlist Files", this, "Add Playlists" );
   
   const QStringList::ConstIterator end  = files.constEnd();
   for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )   
       addPlaylist( *it );
}


void PlaylistBrowser::removePlaylist() //SLOT
{
   PlaylistBrowserItem *item = (PlaylistBrowserItem *)m_listview->currentItem();
   if( !item ) return;
   
 /*  QString path = item->url().path();
   int n = KMessageBox::questionYesNoCancel (this, QString("Do you want to delete\n" + path + "?" ), "Delete file?");
   
   switch( n ) {
       case KMessageBox::Yes:
           //delete the playlist file
           if( remove( path.latin1() ) != 0 )
               KMessageBox::error( this, "An error occured deleting the file." );
               
       case KMessageBox::No:
           //remove the playlist from the playlist browser*/
           delete item;
       /*    break;
   }*/
}


void PlaylistBrowser::renamePlaylist() //SLOT
{
   QListViewItem *item = m_listview->currentItem();
   if( !item ) return;
   
   item->setRenameEnabled( 0, true );
   m_listview->rename( item, 0 );
}


void PlaylistBrowser::renamePlaylist( QListViewItem* item, const QString& newName, int ) //SLOT
{
   #define item static_cast<PlaylistBrowserItem*>(item)

   QString oldPath = item->url().path();
   QString newPath = fileDirPath( oldPath ) + newName + "." + fileExtension( oldPath );
   
   if ( rename( oldPath.latin1(), newPath.latin1() ) == -1 )
       KMessageBox::error( this, "Error renaming the file." );
   else
       item->setUrl( newPath );
       
   #undef item
}

void PlaylistBrowser::loadPlaylist( QListViewItem *item ) //SLOT
{
   if(!item) return;
   
   // open the playlist
   Playlist *pls = Playlist::instance();
   pls->clear();
   pls->appendMedia( ((PlaylistBrowserItem *)item)->url() );
}


void PlaylistBrowser::addPlaylist( QString path )
{
    // this function add a playlist to the playlist browser
    
   QFile file( path );
   if( !file.exists() ) return;
   
   bool exists = false;
   QListViewItemIterator it( m_listview );
   while( it.current() ) {
       PlaylistBrowserItem *item = (PlaylistBrowserItem *)it.current();
       if( item->isPlaylist() && path == item->url().path() )
           exists = true; //the playlist is already in the playlist browser
       ++it;
   }
   
   if( !exists )
       new PlaylistBrowserItem( m_listview, m_listview->lastItem(), KURL( path ) );
}


void PlaylistBrowser::customEvent( QCustomEvent *e )
{
    // the CollectionReader sends a PlaylistFoundEvent when a playlist is found 
    // and this function add the playlist to the playlist browser
    CollectionReader::PlaylistFoundEvent* p = (CollectionReader::PlaylistFoundEvent*)e;
    addPlaylist( p->path() );
}


void PlaylistBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{
   #define item static_cast<PlaylistBrowserItem*>(item)
  
   if( !item ) return;
   
   KPopupMenu menu( this );
   
   if( item->isPlaylist() ) {    //the current item is a playlist
   
       enum Id { LOAD, ADD, EXPAND, RENAME, REMOVE, };
       
       menu.insertItem( i18n( "&Load" ), LOAD );
       menu.insertItem( i18n( "&Add to playlist" ), ADD );
       menu.insertSeparator();
       menu.insertItem( i18n("Expand"), EXPAND );
       menu.insertItem( SmallIcon("editclear"), i18n( "&Rename" ), RENAME );
       menu.insertItem( SmallIcon("edittrash"), i18n( "R&emove" ), REMOVE );
   
       switch( menu.exec( p ) ) 
       {
           case LOAD:
               loadPlaylist( item );
               break;        
           case ADD:
               Playlist::instance()->appendMedia( item->url() );
               break;    
           case EXPAND:
               item->setOpen( true );    
               break;       
           case RENAME:
               renamePlaylist();
               break;
           case REMOVE:
               removePlaylist();
               break;
       }
       
   }
   else {    //the current item is a track

        enum Actions { MAKE, APPEND, QUEUE, INFO };

        menu.insertItem( i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( i18n( "&Add to Playlist" ), APPEND ); //TODO say Append to Playlist
        menu.insertItem( i18n( "&Queue After Current Track" ), QUEUE );
        menu.insertSeparator();
        menu.insertItem( i18n( "Track Information" ), INFO );

        switch( menu.exec( p ) ) {
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case APPEND:
                Playlist::instance()->appendMedia( item->url() );
                break;
            case QUEUE:
                Playlist::instance()->queueMedia( item->url() );
                break;
            case INFO:
                Playlist::instance()->showTrackInfo( item->url() );
        }
   }
   
   #undef item
}


QDragObject *PlaylistBrowser::PlaylistListView::dragObject()
{
   PlaylistBrowserItem *item = (PlaylistBrowserItem *)currentItem();
   return new KURLDrag( item->url(), this );
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserItem
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserItem::PlaylistBrowserItem( KListView *parent, QListViewItem *after, const KURL &url )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_trackn( 0 )
    , m_length( 0 )
    , lastChild( 0 )
    , m_isPlaylist( true )
{

   setDragEnabled( true );
   setRenameEnabled( 0, false );
   setExpandable(true);
   
   setPixmap( 0, KGlobal::iconLoader()->loadIcon( "midi", KIcon::NoGroup, 16 ) );
   kdDebug() << fileBaseName( url.path() ) << endl;
   setText(0, fileBaseName( url.path() ) );
   
   //read the playlist file and and create child item for tracks 
   KURL::List list;
   list += m_url;
   PlaylistLoader *m_loader = new PlaylistLoader( list, this );
   m_loader->start();
}


// this ctor is for child tracks of PlaylistBrowserItem
PlaylistBrowserItem::PlaylistBrowserItem( KListViewItem *parent, KListViewItem *after, const KURL &url, const QString &title, int len )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_title( title )
    , m_length( len )
    , m_isPlaylist( false )
{

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setText( 0, m_title.isEmpty() ? fileBaseName( m_url.path() ) : m_title );
}


PlaylistBrowserItem::~PlaylistBrowserItem() 
{
}


void PlaylistBrowserItem::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
        case PlaylistLoader::MakeItem:
            #define e static_cast<PlaylistLoader::MakeItemEvent*>(e)
            //kdDebug() << "insert " << fileBaseName(e->url().path())<<endl;
            lastChild = new PlaylistBrowserItem( this, lastChild, e->url(), e->title(), e->length() ); //insert after last child
            #undef e
            break;
            
        case PlaylistLoader::Done: {
        
            kdDebug() << "DONE "<<endl; 
            m_trackn = childCount();  //number of tracks
            
            //calculate total playlist length
            QListViewItemIterator it( this );
            ++it; //start from the first child
            int i = 0;
            for( ; i < m_trackn; i++ ) {
                PlaylistBrowserItem *item = (PlaylistBrowserItem *)it.current();
                m_length += item->length();  
                ++it;
            }
            kdDebug() << m_trackn << " Tracks "<<i << " added "<<" Total length: "<<m_length<<endl;
            break;
        }
           
        default: break;
    }
    
}


void PlaylistBrowserItem::setup()
{
    QListViewItem::setup();
    if( isPlaylist() ) {
        QFontMetrics fm( listView()->font() ); 
        setHeight( QMAX( fm.lineSpacing()*2+4+2, pixmap(0)->height()+8 ) );
    }
}

void PlaylistBrowserItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align)
{
    if( !isPlaylist() )
    {
        // for track item we don't need to reimplement paintCell
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }
    
    //flicker-free drawing
    static QPixmap buffer;
    buffer.resize( width, height() );

    if( buffer.isNull() )
    {
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    QPainter pBuf( &buffer, true );
    
    // use alternate background
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );
    
    pBuf.drawPixmap( 2, 4, *pixmap(0) );
    
    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );
    
    int text_x = pixmap(0)->width() + 6;
    int text_y = fm.lineSpacing();
    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );
    
    // draw the playlist name in bold
    font.setBold( true );
    pBuf.setFont( font );
    pBuf.drawText( text_x, text_y, text(0) );
    
    // draw the number of tracks and length of the playlist
    text_y += fm.lineSpacing();
    font.setBold( false );
    pBuf.setFont( font );
    QString info;
    if( m_trackn )    //playlist read
        info += QString("%1 Tracks").arg( m_trackn );
    if( m_length )
        info += QString(" - [%2]").arg( MetaBundle::prettyTime( m_length ) );
    
    pBuf.drawText( text_x, text_y, info);

  /*  pBuf.drawLine( 0, 0, 0, height()-2); 
    pBuf.drawLine( 0, height()-2, width, height()-2 );*/
    //pBuf.fillRect( 0, height()-2, width, 2, cg.text() ); 
    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS SmartPlaylistView
////////////////////////////////////////////////////////////////////////////

SmartPlaylistView::SmartPlaylistView( QWidget *parent, const char *name )
   : KListView( parent, name )
{
   addColumn("Smart Playlists");
   setSelectionMode(QListView::Single);
   setSorting( -1 ); //no sort
   setFullWidth( true );
   
   KListViewItem *item =  new KListViewItem(this, i18n("Never Played") );
   item->setPixmap( 0, SmallIcon("midi") );
   item->setDragEnabled(true);

   item =  new KListViewItem(this, i18n("Recently Played") );
   item->setPixmap( 0, SmallIcon("midi") );
   item->setDragEnabled(true);
   
   item =  new KListViewItem(this, i18n("Newest Tracks") );
   item->setPixmap( 0, SmallIcon("midi") );
   item->setDragEnabled(true);
   
   item = new KListViewItem(this, i18n("Most Played") );
   item->setPixmap( 0, SmallIcon("midi") );
   item->setDragEnabled(true);
   
   item = new KListViewItem(this, i18n("All Collection") );
   item->setPixmap( 0, SmallIcon("kfm") );
   item->setDragEnabled(true);
   
   connect( this, SIGNAL( doubleClicked( QListViewItem *) ), SLOT( loadPlaylistSlot( QListViewItem * ) ) );
   
}


SmartPlaylistView::~SmartPlaylistView()
{
}


KURL::List SmartPlaylistView::loadSmartPlaylist( QueryType queryType )
{
    // this function load the smart playlist querying the database for "queryType"
    // (eg. all collection, most played tracks, recently played, newest tracks, never played)
    // and returns the list of tracks
    //TODO untagged tracks would be useful
    
   QStringList values;
   QStringList names;
   KURL::List list;

   CollectionDB *db = new CollectionDB();
    
   if( queryType == AllCollection ) 
       db->execSql( QString( "SELECT  tags.url "
                            "FROM tags, artist, album "
                            "WHERE artist.id = tags.artist AND album.id = tags.album "
                            "ORDER BY artist.name DESC" ), &values, &names ); 
   
   else if( queryType == MostPlayed ) 
       db->execSql( QString( "SELECT  tags.url "
                            "FROM tags, statistics "
                            "WHERE statistics.url = tags.url "
                            "ORDER BY statistics.playcounter DESC "
                            "LIMIT 0,15;" ), &values, &names );
   
    else if( queryType == NewestTracks )
       db->execSql( QString( "SELECT  tags.url "
                            "FROM tags, artist, album "
                            "WHERE artist.id = tags.artist AND album.id = tags.album "
                            "ORDER BY tags.createdate DESC "
                            "LIMIT 0,15;" ), &values, &names ); 
   
   else if( queryType == RecentlyPlayed )
       db->execSql( QString( "SELECT  tags.url "
                            "FROM tags, statistics "
                            "WHERE statistics.url = tags.url "
                            "ORDER BY statistics.accessdate DESC "
                            "LIMIT 0,15;" ), &values, &names );

   else if( queryType == NeverPlayed )
       db->execSql( QString( "SELECT url "
                                           "FROM tags " 
                                           "WHERE url NOT IN(SELECT url FROM statistics)" ), &values, &names );
   
   
   if ( values.count() )
       for ( uint i = 0; i < values.count(); i = i++ ) 
           list += KURL( values[i].replace( "\"", QCString( "%22" ) ) );
       
   values.clear();
   names.clear();
       
   return list;
}


void SmartPlaylistView::loadPlaylistSlot( QListViewItem *item ) //SLOT
{
    // open the smart playlist
   Playlist::instance()->clear();
   Playlist::instance()->appendMedia( loadSmartPlaylist( (QueryType)itemIndex(item) ) );
}


QDragObject *SmartPlaylistView::dragObject()
{
   return new KURLDrag( loadSmartPlaylist( (QueryType)itemIndex( currentItem() ) ), this );
}


#include "playlistbrowser.moc"
