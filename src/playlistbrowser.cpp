// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "collectiondb.h"      //smart playlists
#include "metabundle.h"        //paintCell()
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistloader.h"    //PlaylistBrowserItem::load()
#include "tagdialog.h"
#include "threadweaver.h"      //PlaylistFoundEvent

#include <qevent.h>            //customEvent()
#include <qfile.h>                //renamePlaylist()
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qstyle.h>             //paintCell()
#include <qsplitter.h>
#include <qheader.h>        //mousePressed()
#include <qtextstream.h>    //saveM3U(), savePLS()

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kio/job.h>         //deleteSelectedPlaylists()
#include <kdebug.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kiconloader.h>       //smallIcon
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>       //renamePlaylist(), deleteSelectedPlaylist()
#include <kpopupmenu.h>
#include <ktoolbar.h>
#include <klineedit.h>        //rename()
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
    removeButton = new KAction( "Remove", "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac, "Remove" );
    renameButton = new KAction( "Rename", "editclear", 0, this, SLOT( renameSelectedPlaylist() ), m_ac, "Rename" );
    deleteButton = new KAction( "Delete", "editdelete", 0, this, SLOT( deleteSelectedPlaylists() ), m_ac, "Delete" );
    
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    open->plug( m_toolbar );
    
    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    removeButton->plug( m_toolbar );
    renameButton->plug( m_toolbar);
    deleteButton->plug( m_toolbar);
    removeButton->setEnabled( false );
    renameButton->setEnabled( false );
    deleteButton->setEnabled( false );
    
    m_splitter = new QSplitter( Vertical, this );
    
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
   
    // save the path of all playlists loaded in the playlist browser
    QStringList urls;
    QListViewItemIterator it( m_listview );  
    while( it.current() ) {
        if( isPlaylist( *it ) )
            urls += ((PlaylistBrowserItem *)*it)->url().path();
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
    // open a file selector to add playlists to the playlist browser
    QStringList files;
    files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|Playlist Files", this, "Add Playlists" );
   
    const QStringList::ConstIterator end  = files.constEnd();
    for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )   
        addPlaylist( *it );
}


void PlaylistBrowser::loadPlaylist( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    if( isPlaylist( item ) ) {
        // open the playlist
        #define  item static_cast<PlaylistBrowserItem *>(item)
        Playlist *pls = Playlist::instance();
        pls->clear();
        pls->appendMedia( item->tracksURL() );
        #undef item
    }
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
        if( isPlaylist( item ) && path == item->url().path() )
            exists = true; //the playlist is already in the playlist browser
        ++it;
    }
   
    if( !exists ) {
        if( lastPlaylist == 0 ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
            deleteButton->setEnabled( true );
        }
        lastPlaylist = new PlaylistBrowserItem( m_listview, lastPlaylist, KURL( path ) );
    }
}


void PlaylistBrowser::removeSelectedItems() //SLOT
{
    // this function remove selected playlists and tracks
    
    //remove currentItem, no matter if selected or not
    m_listview->setSelected( m_listview->currentItem(), true );
    
    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected);
    for( ; it.current(); ++it ) {
        // if the playlist of the track item is already selected the current item is skipped
        // it will be deleted from the parent
        QListViewItem *parent = it.current()->parent();
        if( parent && parent->isSelected() )
            continue;
            
        selected.append( it.current() );
    }
        
    for( QListViewItem *item = selected.first(); item; item = selected.next() ) {
        if( isPlaylist( item ) ) {
            if( item == lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                lastPlaylist = above ? (PlaylistBrowserItem *)above : 0;
            }
            delete item;
        }
        else {
            PlaylistBrowserItem *playlist = (PlaylistBrowserItem *)item->parent();
            playlist->removeTrack( item );
        }
    }
    
}


void PlaylistBrowser::renameSelectedPlaylist() //SLOT
{
    QListViewItem *item = m_listview->currentItem();
    if( !item ) return;

    if( isPlaylist( item ) ) {
        item->setRenameEnabled( 0, true );
        m_listview->rename( item, 0 );
    }
}


void PlaylistBrowser::renamePlaylist( QListViewItem* item, const QString& newName, int ) //SLOT
{
    #define item static_cast<PlaylistBrowserItem*>(item)

    QString oldPath = item->url().path();
    QString newPath = fileDirPath( oldPath ) + newName + fileExtension( oldPath );
   
    if ( rename( oldPath.latin1(), newPath.latin1() ) == -1 )
        KMessageBox::error( this, "Error renaming the file." );
    else
        item->setUrl( newPath );
    
    item->setRenameEnabled( 0, false );
    
    #undef item
}


void PlaylistBrowser::deleteSelectedPlaylists() //SLOT
{
    KURL::List urls;
    
    //delete currentItem, no matter if selected or not
    m_listview->setSelected( m_listview->currentItem(), true );     
    
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        if( isPlaylist( *it ) )
            urls.append( ((PlaylistBrowserItem *)*it)->url() );
        else    //we want to delete only playlists
            m_listview->setSelected( it.current(), false );
    }
    
    if ( urls.isEmpty() ) return;
    
    int button = KMessageBox::warningContinueCancel( this, i18n(
                        "<p>You have selected %1 to be <b>irreversibly</b> "
                        "deleted." ).arg( i18n("1 playlist", "<u>%n playlists</u>", urls.count()) ),
                        QString::null,
                        i18n("&Delete Selected Files") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( removeSelectedItems() ) );
    }
}


void PlaylistBrowser::savePlaylist( PlaylistBrowserItem *item )
{
    //save the modified playlist
    const QString ext = fileExtension( item->url().path() );
    if( ext.lower() == ".m3u" )
        saveM3U( item );
    else
        savePLS( item );
    
    item->setModified( false );
}


void PlaylistBrowser::saveM3U( PlaylistBrowserItem *item )
{
    QFile file( item->url().path() );

    if( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << "#EXTM3U\n";
        QPtrList<TrackItemInfo> trackList = item->trackList();
        for( TrackItemInfo *info = trackList.first(); info; info = trackList.next() )
        {
            stream << "#EXTINF:";
            stream << info->length();
            stream << ',';         
            stream << info->title();
            stream << '\n';
            stream << (info->url().protocol() == "file" ? info->url().path() : info->url().url());
            stream << "\n";
        }
        
        file.close();  
    }
}


void PlaylistBrowser::savePLS( PlaylistBrowserItem *item )
{
    QFile file( item->url().path() );

    if( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        QPtrList<TrackItemInfo> trackList = item->trackList();
        for( TrackItemInfo *info = trackList.first(); info; info = trackList.next() )
        {
            stream << "File=";
            stream << (info->url().protocol() == "file" ? info->url().path() : info->url().url());
            stream << "\nTitle=";
            stream << info->title();
            stream << "\nLength=";
            stream << info->length();
            stream << "\n";
        }
        
        file.close();
    }
}


void PlaylistBrowser::currentItemChanged( QListViewItem *item )    //SLOT
{
    // remove, rename and delete buttons are disabled if there are no playlists
    // rename and delete buttons are disabled for track items
    
    #define item static_cast<PlaylistBrowserItem *>(item)
    
    bool enable_remove = false;
    bool enable_rename = false;
    bool enable_delete = false;
    
    if( !item )
        goto enable_buttons;
         
    else if( isPlaylist( item ) ) {
        enable_remove = true;
        enable_rename = true;
        enable_delete = true;
    }
    else
        enable_remove = true;
        
        
    enable_buttons:    
    
    removeButton->setEnabled( enable_remove );
    renameButton->setEnabled( enable_rename );
    deleteButton->setEnabled( enable_delete );
    
    #undef item
}


void PlaylistBrowser::customEvent( QCustomEvent *e )
{
    //if a playlist is found in collection folders it will be automatically added to the playlist browser
    
    // the CollectionReader sends a PlaylistFoundEvent when a playlist is found 
    CollectionReader::PlaylistFoundEvent* p = (CollectionReader::PlaylistFoundEvent*)e;
    addPlaylist( p->path() );
}


void PlaylistBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{   
    if( !item ) return;
   
    KPopupMenu menu( this );
   
    if( isPlaylist( item ) ) {    
    //************* Playlist menu ***********
        #define item static_cast<PlaylistBrowserItem*>(item)
        
        enum Id { LOAD, ADD, SAVE, RESTORE, RENAME, REMOVE, DELETE };
        
        menu.insertItem( i18n( "&Load" ), LOAD );
        menu.insertItem( i18n( "&Add to playlist" ), ADD );
        menu.insertSeparator();
        if( item->isModified() ) {
            menu.insertItem( SmallIcon("filesave"), i18n( "&Save" ), SAVE );
            menu.insertItem( i18n( "Res&tore" ), RESTORE );
            menu.insertSeparator();
        }
        menu.insertItem( SmallIcon("editclear"), i18n( "&Rename" ), RENAME );
        menu.insertItem( SmallIcon("edittrash"), i18n( "R&emove" ), REMOVE );
        menu.insertItem( SmallIcon("editdelete"), i18n( "&Delete" ), DELETE );
        menu.setAccel( Key_Space, LOAD );
        menu.setAccel( Key_F2, RENAME );
        menu.setAccel( Key_Delete, REMOVE );
        menu.setAccel( SHIFT+Key_Delete, DELETE );
        
        switch( menu.exec( p ) ) 
        {
            case LOAD:
                loadPlaylist( item );
                break;        
            case ADD:
                Playlist::instance()->appendMedia( item->tracksURL() );
                break;
            case SAVE:
                PlaylistBrowser::instance()->savePlaylist( item );
                break;
            case RESTORE:
                item->load( true );    //restore the playlist
                break;
            case RENAME:
                renameSelectedPlaylist();
                break;
            case REMOVE:
                removeSelectedItems();
                break;
            case DELETE:
                deleteSelectedPlaylists();
                break;
        }
        #undef item
    }
    else {   
    //******** track menu ***********
        #define item static_cast<PlaylistTrackItem*>(item)
        
        enum Actions { MAKE, APPEND, QUEUE, REMOVE, INFO };

        menu.insertItem( i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( i18n( "&Add to Playlist" ), APPEND ); //TODO say Append to Playlist
        menu.insertItem( i18n( "&Queue After Current Track" ), QUEUE );
        menu.insertSeparator();
        menu.insertItem( SmallIcon("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIcon("info"), i18n( "&Track Information" ), INFO );
        
        switch( menu.exec( p ) ) {
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case APPEND:
                Playlist::instance()->appendMedia( item->url() );
                break;
            case QUEUE:
                Playlist::instance()->queueMedia( item->url() );
                break;
            case REMOVE:
                PlaylistBrowser::instance()->removeSelectedItems();
                break;
            case INFO:
                TagDialog* dialog = new TagDialog( item->url() );
                dialog->show();
        }
        #undef item
   }
   
    
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserView
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserView::PlaylistBrowserView( QWidget *parent, const char *name )
    : KListView( parent, name )
{
    addColumn( "Playlists" );
    setSelectionMode( QListView::Extended );
    setSorting( -1 );  //no sort
    
    setDropVisualizer( true );
    setDropHighlighter( true );
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );
    
    setFullWidth( true );
    setTreeStepSize( 20 );

    connect( this, SIGNAL( mouseButtonPressed ( int, QListViewItem *, const QPoint &, int ) ),
            this, SLOT( mousePressed( int, QListViewItem *, const QPoint &, int ) ) );
    //TODO moving tracks
    //connect( this, SIGNAL( moved(QListViewItem *, QListViewItem *, QListViewItem * )),
    //        this, SLOT( itemMoved(QListViewItem *, QListViewItem *, QListViewItem * )));
}


PlaylistBrowserView::~PlaylistBrowserView()
{
}

void PlaylistBrowserView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() != viewport() && KURLDrag::canDecode( e ) );
}

void PlaylistBrowserView::contentsDragMoveEvent( QDragMoveEvent* e )
{
    //Get the closest item _before_ the cursor
    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item ) {
        slotEraseMarker();
        return;
    }
   
    //only for track items (for playlist items we draw the highlighter)
    if( !isPlaylist( item ) && p.y() - itemRect( item ).top() < (item->height()/2) ) 
        item = item->itemAbove();

    if( item != m_marker )
    {
        slotEraseMarker();
        m_marker = item;
        viewportPaintEvent( 0 );
    }
}


void PlaylistBrowserView::contentsDragLeaveEvent( QDragLeaveEvent* )
{
     slotEraseMarker();
}


void PlaylistBrowserView::contentsDropEvent( QDropEvent *e )
{
    QListViewItem *parent = 0;
    QListViewItem *after;

    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item ) {
        slotEraseMarker();
        return;
    }
    
    if( !isPlaylist( item ) )
        findDrop( e->pos(), parent, after ); 
    
    slotEraseMarker();
    
    if( e->source() == viewport() )   
        e->ignore();    //TODO add support to move tracks
    else {
        KURL::List list;
        QMap<QString, QString> map;
        if( KURLDrag::decode( e, list, map ) ) {
            if( parent ) {
                //insert the dropped tracks
                PlaylistBrowserItem *playlist = (PlaylistBrowserItem *)parent;
                playlist->insertTracks( after, list, map );
            } 
            else //dropped on a playlist item
            { 
                PlaylistBrowserItem *playlist = (PlaylistBrowserItem *)item;
                //append the dropped tracks
                playlist->insertTracks( 0, list, map );
            }
        }
        else
            e->ignore();
    }
    
}

void PlaylistBrowserView::slotEraseMarker() //SLOT
{
    if( m_marker )
    {
        QRect spot;
        if( isPlaylist( m_marker ) )
            spot = drawItemHighlighter( 0, m_marker );
        else
            spot = drawDropVisualizer( 0, 0, m_marker );
        
        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}

void PlaylistBrowserView::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e ); //we call with 0 in contentsDropEvent()

    if( m_marker )
    {
        QPainter painter( viewport() );
        if( isPlaylist( m_marker ) )    //when dragging on a playlist we draw a focus rect
            drawItemHighlighter( &painter, m_marker );
        else //when dragging on a track we draw a line marker
            painter.fillRect( drawDropVisualizer( 0, 0, m_marker ), 
                                   QBrush( colorGroup().highlight(), QBrush::Dense4Pattern ) );
    }
}


void PlaylistBrowserView::mousePressed( int button, QListViewItem *item, const QPoint &pnt, int )    //SLOT
{ 
    // this function expande/collapse the playlist if the +/- symbol has been pressed
    // and show the save menu if the save icon has been pressed

    if( !item || button != LeftButton ) return;
   
    if( isPlaylist( item ) ) {
        #define item static_cast<PlaylistBrowserItem *>(item)
        
        QPoint p = mapFromGlobal( pnt );
        p.setY( p.y() - header()->height() );
        
        QRect itemrect = itemRect( item );
        
        QRect expandRect = QRect( 4, itemrect.y() + (item->height()/2) - 5, 15, 15 );
        if( expandRect.contains( p ) ) {    //expand symbol clicked
            setOpen( item, !item->isOpen() );
            return;
        }

        if( item->isModified() ) {    
            QRect saveRect = QRect( 23, itemrect.y() + 3, 16, 16 );        
            if( saveRect.contains( p ) ) {
        
                enum Id { SAVE, RESTORE };
            
                KPopupMenu saveMenu( this );
                saveMenu.insertItem( SmallIcon("filesave"), i18n( "&Save" ), SAVE );
                saveMenu.insertItem( i18n( "&Restore" ), RESTORE );
            
                switch( saveMenu.exec( pnt ) ) {
                    case SAVE:
                        PlaylistBrowser::instance()->savePlaylist( item );
                        break;
                        
                    case RESTORE:
                        item->load( true );    //restore the playlist
                        break;
                }        
            }
         
        }
        #undef item
    }

    
}


void PlaylistBrowserView::rename( QListViewItem *item, int c )
{
    // we reimplement this function to show the 
    KListView::rename( item, c );
    
    QRect rect( itemRect( item ) );
    int fieldX = rect.x() + treeStepSize() + 2;
    int fieldW = rect.width() - treeStepSize() - 2;
    if( item->pixmap(0) ) {
        int pixw = item->pixmap(0)->width() + 3;
        fieldX += pixw;
        fieldW -= pixw;
    }
    
    KLineEdit *renameEdit = renameLineEdit();
    renameEdit->setGeometry( fieldX, rect.y(), fieldW, rect.height() );
    renameEdit->show();
}


void PlaylistBrowserView::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {
         case Key_Space:    //load
            PlaylistBrowser::instance()->loadPlaylist( currentItem() );
            break;
            
        case Key_Delete:    //remove
            PlaylistBrowser::instance()->removeSelectedItems();
            break;
        
        case Key_F2:    //rename
            PlaylistBrowser::instance()->renameSelectedPlaylist();
            break;
            
        case SHIFT+Key_Delete:    //delete
            PlaylistBrowser::instance()->deleteSelectedPlaylists();
            break;
            
        default:
            KListView::keyPressEvent( e );
            break;
    }
}


QDragObject *PlaylistBrowserView::dragObject()
{
    KURL::List urls;
    
    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        if( isPlaylist( *it ) )
            urls += ((PlaylistBrowserItem*)*it)->tracksURL();
        else 
            urls += ((PlaylistTrackItem*)*it)->url();
    }

    return new KURLDrag( urls, viewport() );
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserItem
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserItem::PlaylistBrowserItem( KListView *parent, QListViewItem *after, const KURL &url )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_length( 0 )
    , m_done( false )
    , m_modified( false )
    , m_savePix( 0 )
    , m_lastTrack( 0 )
{
    m_trackList.setAutoDelete( true );
    
    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);
   
    kdDebug() << fileBaseName( url.path() ) << endl;
    setText(0, fileBaseName( url.path() ) );
   
    //load the playlist file
    load();
}


PlaylistBrowserItem::~PlaylistBrowserItem()
{
    m_trackList.clear();
}


void PlaylistBrowserItem::load( bool clear )
{
    //if clear is true the playlist is restored
    
    if( clear ) {
        setOpen( false );
        m_trackList.clear();
        m_length = 0;
        m_done = false;
    }
    
     //read the playlist file in a thread
    KURL::List list;
    list += m_url;
    PlaylistLoader *m_loader = new PlaylistLoader( list, this );
    m_loader->start();
    
    setModified( false );
}


void PlaylistBrowserItem::insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map )
{
    int pos = 0;
    if( after ) {
        pos = m_trackList.find( ((PlaylistTrackItem*)after)->trackInfo() ) + 1;
        if( pos == -1 )
            return;
    }
    
    uint k = 0;
    const KURL::List::ConstIterator end = list.end();
    for ( KURL::List::ConstIterator it = list.begin(); it != end; ++it,k++ ) {
        QString key = (*it).isLocalFile() ? (*it).path() : (*it).url();
        QString str = map[ key ];
        QString title = str.section(';',0,0);
        int length = str.section(';',1,1).toUInt();
        
        TrackItemInfo *newInfo = new TrackItemInfo( *it, title, length );
        m_length += newInfo->length();
        
        if( after ) {
            m_trackList.insert( pos+k, newInfo );
            if( isOpen() )
                after = new PlaylistTrackItem( this, after, newInfo );
        }
        else {
            m_trackList.append( newInfo );
            if( isOpen() )  //append the track item to the playlist
                m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, newInfo );
        }
        
    }
    
    setModified( true );
}


void PlaylistBrowserItem::removeTrack( QListViewItem *item )
{
    #define item static_cast<PlaylistTrackItem*>(item)
    
    TrackItemInfo *info = item->trackInfo();
    m_length -= info->length();
    m_trackList.remove( info );
    if( item == m_lastTrack ) {
        QListViewItem *above = item->itemAbove();
        m_lastTrack = above ? (PlaylistTrackItem *)above : 0;
    }
    delete item;
    
    #undef item
    
    setModified( true );
}


void PlaylistBrowserItem::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
        case PlaylistLoader::MakeItem:  {
            #define e static_cast<PlaylistLoader::MakeItemEvent*>(e)
            TrackItemInfo *info = new TrackItemInfo( e->url(), e->title(), e->length() );
            m_trackList.append( info );
            m_length += info->length();
            if( isOpen() )
                m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
            #undef e
            break;
        }
            
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
    if( open == isOpen())
        return;

    if( open ) {    //expande
        //create track items
        for ( TrackItemInfo *info = m_trackList.first(); info; info = m_trackList.next() ) 
            m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
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
        m_lastTrack = 0;
        
    }
    
    QListViewItem::setOpen( open );
}


KURL::List PlaylistBrowserItem::tracksURL()
{
    KURL::List list;
    
    if( m_done )  { //playlist loaded
        for( TrackItemInfo *info = m_trackList.first(); info; info = m_trackList.next() )
            list += info->url();
    } 
    else 
        list = m_url;    //playlist url
        
    return list;
}


void PlaylistBrowserItem::setModified( bool chg )
{
    if( chg != m_modified ) {
        if( chg )
            m_savePix = new QPixmap( KGlobal::iconLoader()->loadIcon( "filesave", KIcon::NoGroup, 16 ) );
        else {
            delete m_savePix;
            m_savePix = 0;
        }
    
        m_modified = chg;
    }
    //this function is also called every time a track is inserted or removed
    //we repaint the item to update playlist info
    repaint();
}


void PlaylistBrowserItem::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = m_savePix ? QMAX( m_savePix->height(), fm.lineSpacing() ) : fm.lineSpacing();
    setHeight( h + fm.lineSpacing() + margin + 4 );
}


void PlaylistBrowserItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align)
{ 
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
    
    QRect rect( ((lv->treeStepSize()-9) / 2) + 1, (height()-9) / 2, 9, 9 );
    
    if( !m_trackList.isEmpty() ) {
        //draw +/- symbol to expande/collapse the playlist
        
        pBuf.setPen( cg.mid() );
        pBuf.drawRect( rect );
        //fill the rect with base color if the item has alternate color and viceversa
        QColor color = backgroundColor() == lv->alternateBackground() ? cg.base() : lv->alternateBackground();
        pBuf.fillRect( rect.x()+1, rect.y()+1, rect.width()-2, rect.height()-2, color );    
        // +/- drawing
        pBuf.setPen( cg.text() );
        pBuf.drawLine( rect.x()+2, rect.y()+4, rect.x()+6, rect.y()+4 );
        if( !isOpen() )
            pBuf.drawLine( rect.x()+4, rect.y()+2, rect.x()+4, rect.y()+6 );  
    }
    
    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );
    
    int text_x = lv->treeStepSize() + 3;
    int text_y = fm.lineSpacing();
    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );
    
    //if the playlist has been modified a save icon is shown
    if( m_modified && m_savePix ) {       
        pBuf.drawPixmap( text_x, 3, *m_savePix );
        text_x += m_savePix->width()+4;
    }
    
    // draw the playlist name in bold
    font.setBold( true );
    pBuf.setFont( font );
    QFontMetrics fmName( font );
    
    QString name = text(0);
    if( fmName.width( name ) + text_x + lv->itemMargin()*2 > width ) {
        int ellWidth = fmName.width( "..." );
        QString text = QString::fromLatin1("");
        int i = 0;
        int len = name.length();
        while ( i < len && fmName.width( text + name[ i ] ) + ellWidth < width - text_x - lv->itemMargin()*2  ) {
            text += name[ i ];
            i++;
        }
        name = text + "...";
    }
    
    pBuf.drawText( text_x, text_y, name );
    
    if( m_done ) { //playlist loaded
        // draw the number of tracks and the total length of the playlist
        text_y += m_savePix ? QMAX( fm.lineSpacing(), m_savePix->height() ) : fm.lineSpacing();
        font.setBold( false );
        pBuf.setFont( font );
    
        QString info;
        info += QString("%1 Tracks").arg( m_trackList.count() );
        if( m_length )
            info += QString(" - [%2]").arg( MetaBundle::prettyTime( m_length ) );
    
        if( m_modified )
            text_x = lv->treeStepSize() + 3;
        pBuf.drawText( text_x, text_y, info);
    }
    
    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}



//////////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistTrackItem
////////////////////////////////////////////////////////////////////////////////

PlaylistTrackItem::PlaylistTrackItem( QListViewItem *parent, QListViewItem *after, TrackItemInfo *info )
    : KListViewItem( parent, after )
    , m_trackInfo( info ) 
{
    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setText( 0, info->title() );
}

const KURL &PlaylistTrackItem::url()
{
    return m_trackInfo->url();
}


//////////////////////////////////////////////////////////////////////////////////
//    CLASS TrackItemInfo
////////////////////////////////////////////////////////////////////////////////

TrackItemInfo::TrackItemInfo( const KURL &u, const QString &t, const int l ) 
        : m_url( u ) 
        , m_title( t )
        , m_length( l )
{
    if( m_title.isEmpty() )
        m_title = MetaBundle::prettyTitle( fileBaseName( m_url.path() ) );
    
    if( m_length < 0 )
        m_length = 0;
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
    
    switch( queryType ) {
    
        case AllCollection: 
            db->execSql( "SELECT  tags.url "
                                  "FROM tags, artist, album "
                                  "WHERE artist.id = tags.artist AND album.id = tags.album "
                                  "ORDER BY artist.name DESC", &values, &names ); 
            break;
        
        case MostPlayed:
            db->execSql( "SELECT  tags.url "
                                  "FROM tags, statistics "
                                  "WHERE statistics.url = tags.url "
                                  "ORDER BY statistics.playcounter DESC "
                                  "LIMIT 0,15;", &values, &names );
            break;
            
        case NewestTracks:
            db->execSql( "SELECT  tags.url "
                                  "FROM tags, artist, album "
                                  "WHERE artist.id = tags.artist AND album.id = tags.album "
                                  "ORDER BY tags.createdate DESC "
                                  "LIMIT 0,15;", &values, &names ); 
            break;
            
        case RecentlyPlayed:
            db->execSql( "SELECT  tags.url "
                                  "FROM tags, statistics "
                                  "WHERE statistics.url = tags.url "
                                  "ORDER BY statistics.accessdate DESC "
                                  "LIMIT 0,15;", &values, &names );
            break;
            
        case NeverPlayed:
            db->execSql( "SELECT url "
                                  "FROM tags " 
                                  "WHERE url NOT IN(SELECT url FROM statistics)", &values, &names );
            break;
            
        default:    ;
    }
   
    if ( !values.isEmpty() )
    {
        for ( uint i = 0; i < values.count(); ++i ) 
            list += KURL( values[i] );
    }
       
    values.clear();
    names.clear();
    delete db;
    
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
