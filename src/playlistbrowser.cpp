// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "collectiondb.h"      //smart playlists
#include "metabundle.h"        //paintCell()
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistloader.h"    //PlaylistBrowserItem ctor
#include "threadweaver.h"      //PlaylistFoundEvent

#include <qevent.h>            //customEvent()
#include <qfile.h>
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qstyle.h>             //paintCell()
#include <qsplitter.h>
#include <qheader.h>        //mousePressed()

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kiconloader.h>       //smallIcon
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>       //removePlaylist() and renamePlaylist()
#include <kpopupmenu.h>
#include <ktoolbar.h>
#include <kurldrag.h>          //dragObject()

#include <stdio.h>             //remove() and rename()


PlaylistBrowser *PlaylistBrowser::s_instance = 0;


PlaylistBrowser::PlaylistBrowser( const char *name )
    : QVBox( 0, name )
    , lastPlaylist( 0 )
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
    removeButton = new KAction( "Remove", "edittrash", 0, this, SLOT( removePlaylist() ), m_ac, "Remove" );
    renameButton = new KAction( "Rename", "editclear", 0, this, SLOT( renamePlaylist() ), m_ac, "Rename" );
    
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right
    open->plug( m_toolbar );
    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    removeButton->plug( m_toolbar );
    renameButton->plug( m_toolbar);
    removeButton->setEnabled( false );
    renameButton->setEnabled( false );
    
    m_splitter    = new QSplitter( Vertical, this );
    
    m_listview = new PlaylistBrowserView( m_splitter );
    
    m_smartlistview = new SmartPlaylistView( m_splitter );
    
    QString str = config->readEntry( "Splitter" );
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *m_splitter; //this sets the splitters position

    // signals and slots connections
    connect( m_listview, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
           this, SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( m_listview, SIGNAL( doubleClicked( QListViewItem *) ), 
           this, SLOT( loadPlaylist( QListViewItem * ) ) );
    connect( m_listview, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
           this, SLOT( renamePlaylist( QListViewItem*, const QString&, int ) ) );
    connect( m_listview, SIGNAL( currentChanged( QListViewItem * ) ),
            this, SLOT( currentItemChanged( QListViewItem * ) ) );
   
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
            addPlaylist( *it );
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


void PlaylistBrowser::loadPlaylist( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    #define  item static_cast<PlaylistBrowserItem *>(item)
    if( item->isPlaylist() ) {
        // open the playlist
        Playlist *pls = Playlist::instance();
        pls->clear();
        pls->appendMedia( item->tracks() );
    }
    #undef item
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
   
    if( !exists ) {
        if( lastPlaylist == 0 ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
        }
        lastPlaylist = new PlaylistBrowserItem( m_listview, lastPlaylist, KURL( path ) );
    }
}


void PlaylistBrowser::removePlaylist() //SLOT
{
    PlaylistBrowserItem *item = (PlaylistBrowserItem *)m_listview->currentItem();
    if( !item ) return;
    
    if( item->isPlaylist() ) {
        if( item == lastPlaylist ) {
            QListViewItem *above = item->itemAbove();
            if( above )
                lastPlaylist = (PlaylistBrowserItem *)above;
            else 
                lastPlaylist = 0;
        }
        delete item;
    }
}


void PlaylistBrowser::renamePlaylist() //SLOT
{
    PlaylistBrowserItem *item = (PlaylistBrowserItem *)m_listview->currentItem();
    if( !item ) return;

    if( item->isPlaylist() ) {
        item->setRenameEnabled( 0, true );
        m_listview->rename( item, 0 );
    }
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
    
    item->setRenameEnabled( 0, false );
    #undef item
}


void PlaylistBrowser::currentItemChanged( QListViewItem *item )    //SLOT
{
    #define item static_cast<PlaylistBrowserItem *>(item)
    // remove and delete buttons are disabled if there are no playlists or the current item is a track
    bool enable = false;
    
    if( !item )
        goto enable_buttons;
    
    if( item->isPlaylist() )
        enable = true;
    
    enable_buttons:
    
    removeButton->setEnabled( enable );
    renameButton->setEnabled( enable );
    
    #undef item
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
   
        enum Id { LOAD, ADD, RENAME, REMOVE, };
       
        menu.insertItem( i18n( "&Load" ), LOAD );
        menu.insertItem( i18n( "&Add to playlist" ), ADD );
        menu.insertSeparator();
        menu.insertItem( SmallIcon("editclear"), i18n( "&Rename" ), RENAME );
        menu.insertItem( SmallIcon("edittrash"), i18n( "R&emove" ), REMOVE );
        menu.setAccel( Key_Space, LOAD );
        menu.setAccel( Key_F2, RENAME );
        menu.setAccel( Key_Delete, REMOVE );
        
        switch( menu.exec( p ) ) 
        {
            case LOAD:
                loadPlaylist( item );
                break;        
            case ADD:
                Playlist::instance()->appendMedia( item->tracks() );
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



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserView
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserView::PlaylistBrowserView( QWidget *parent, const char *name )
    : KListView( parent, name )
{
    addColumn( "Playlists" );
    setSelectionMode( QListView::Single );
    setSorting( -1 );  //no sort
    setFullWidth( true );
    setTreeStepSize( 20 );

    connect( this, SIGNAL( pressed ( QListViewItem *, const QPoint &, int ) ),
            this, SLOT( mousePressed( QListViewItem *, const QPoint &, int ) ) );
}


PlaylistBrowserView::~PlaylistBrowserView()
{
}


void PlaylistBrowserView::mousePressed( QListViewItem *item, const QPoint &pnt, int )    //SLOT
{ 
    // this function check if the +/- symbol has been pressed and expande/collapse the playlist
    
    #define item static_cast<PlaylistBrowserItem *>(item)
    
    if( !item ) return;
   
    if( item->isPlaylist() ) {
    
        QPoint p = mapFromGlobal( pnt );
        p.setY( p.y() - header()->height() );
        
        QRect r = itemRect( item );
 
        r = QRect( 4, r.y() + (item->height()/2) - 5, 15, 15 );
        if( r.contains( p ) )    //expand symbol clicked
            setOpen( item, !item->isOpen() );
        
    }
    
    #undef item
}


void PlaylistBrowserView::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {
         case Key_Space:    //load
            PlaylistBrowser::instance()->loadPlaylist( currentItem() );
            break;
            
        case Key_Delete:    //remove
            PlaylistBrowser::instance()->removePlaylist();    //remove current playlist
            break;
        
        case Key_F2:    //rename
            PlaylistBrowser::instance()->renamePlaylist();
            break;
            
        default:
            KListView::keyPressEvent( e );
            break;
    }
}


QDragObject *PlaylistBrowserView::dragObject()
{
    PlaylistBrowserItem *item = (PlaylistBrowserItem *)currentItem();
    return new KURLDrag( item->isPlaylist() ? item->tracks() : item->url(), this );
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserItem
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserItem::PlaylistBrowserItem( KListView *parent, KListViewItem *after, const KURL &url )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_trackn( 0 )
    , m_length( 0 )
    , lastChild( 0 )
    , m_isPlaylist( true )
    , m_done( false )
{

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);
   
    setPixmap( 0, KGlobal::iconLoader()->loadIcon( "midi", KIcon::NoGroup, 16 ) );
    kdDebug() << fileBaseName( url.path() ) << endl;
    setText(0, fileBaseName( url.path() ) );
   
    //read the playlist file in a thread
    KURL::List list;
    list += m_url;
    PlaylistLoader *m_loader = new PlaylistLoader( list, this );
    m_loader->start();
}


// this ctor is for child tracks of PlaylistBrowserItem
PlaylistBrowserItem::PlaylistBrowserItem( KListViewItem *parent, KListViewItem *after, const KURL &url, const QString &title )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_title( title )
    , m_isPlaylist( false )
{

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setText( 0, m_title.isNull() ? fileBaseName( m_url.path() ) : m_title );
}


void PlaylistBrowserItem::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
        case PlaylistLoader::MakeItem:
            #define e static_cast<PlaylistLoader::MakeItemEvent*>(e)
            m_list += e->url();    //add the track to the tracks list
            m_length += e->length();    //add the track length to playlist total length
            m_titleList += e->title();
            m_trackn++;    //increment number of tracks in playlist
            #undef e
            break;
            
        case PlaylistLoader::Done: {       
            kdDebug() << text(0) << " DONE" << endl; 
            m_done = true;
            // repaint the item to display playlist info
            repaint();
            break;
        }
           
        default: break;
    }
    
}

void PlaylistBrowserItem::setOpen( bool open )
{
    if( open == isOpen() )
        return;

    if( open ) {    //expande and create track items
    
        PlaylistBrowserItem *last=0;
        int k=0;
        const KURL::List::ConstIterator end = m_list.end();
        for ( KURL::List::ConstIterator it = m_list.begin(); it != end; ++it, k++ )
            last = new PlaylistBrowserItem( this, last, *it, m_titleList[k] );
    }
    else {    //collapse
        
        QListViewItem* child = firstChild();
        QListViewItem* childTmp;
        //delete all children
        while ( child ) {
            childTmp = child;
            child = child->nextSibling();
            delete childTmp;
        }
        
    }
    
    QListViewItem::setOpen( open );
}


KURL::List PlaylistBrowserItem::tracks()
{
    if( m_done )  //playlist loaded
        return m_list;    //tracks list
    else 
        return KURL::List( m_url );    //returns the playlist url
}


void PlaylistBrowserItem::setup()
{
    if( isPlaylist() ) {
        QFontMetrics fm( listView()->font() );
        int margin = listView()->itemMargin()*2;
        setHeight( fm.lineSpacing()*2 + margin + 4 );
    }
    else
        QListViewItem::setup();
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
    
    // draw a line at the top
    pBuf.setPen( cg.text() );
    pBuf.drawLine( 0, 0, width, 0 );
    
    KListView *lv = (KListView *)listView();
    
    QRect rect( 6, (height()-9)/2, 9, 9 );
    
    //draw +/- symbol to open/close the playlist
    
    pBuf.setPen( cg.mid() );
    pBuf.drawRect( rect );
    //fill the rect with base color if the item has alternate color and viceversa
    QColor color = backgroundColor() == lv->alternateBackground() ? cg.base() : lv->alternateBackground();
    pBuf.fillRect( rect.x()+1, rect.y()+1, rect.width()-2, rect.height()-2, color );    
    //draw the - 
    pBuf.setPen( cg.text() );
    pBuf.drawLine( rect.x()+2, rect.y()+4, rect.x()+6, rect.y()+4 );
    if( !isOpen() )
        pBuf.drawLine( rect.x()+4, rect.y()+2, rect.x()+4, rect.y()+6 );  
    
    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );
    
    int text_x = rect.x() + rect.width() + 8;
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
    if( m_trackn )
        info += QString("%1 Tracks").arg( m_trackn );
    if( m_length )
        info += QString(" - [%2]").arg( MetaBundle::prettyTime( m_length ) );
    
    pBuf.drawText( text_x, text_y, info);

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
   
    KListViewItem *lastItem = new KListViewItem( this, i18n("All Collection") );
    lastItem->setPixmap( 0, SmallIcon("kfm") );
    lastItem->setDragEnabled(true);
    
    //insert items after last item
    lastItem = new KListViewItem(this, lastItem, i18n("Most Played") );
    lastItem->setPixmap( 0, SmallIcon("midi") );
    lastItem->setDragEnabled(true);
    
    lastItem =  new KListViewItem(this, lastItem, i18n("Newest Tracks") );
    lastItem->setPixmap( 0, SmallIcon("midi") );
    lastItem->setDragEnabled(true);
    
    lastItem =  new KListViewItem(this, lastItem, i18n("Recently Played") );
    lastItem->setPixmap( 0, SmallIcon("midi") );
    lastItem->setDragEnabled(true);
    
    lastItem =  new KListViewItem(this, lastItem, i18n("Never Played") );
    lastItem->setPixmap( 0, SmallIcon("midi") );
    lastItem->setDragEnabled(true);

    connect( this, SIGNAL( doubleClicked( QListViewItem *) ), SLOT( loadPlaylistSlot( QListViewItem * ) ) );
   
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
    
   
    if ( !values.isEmpty() )
    {
        const QString escapedQuote = "%22";
    
        for ( uint i = 0; i < values.count(); ++i ) 
            list += KURL( values[i].replace( '\"', escapedQuote ) );
    }
       
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
