// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// License: GPL V2. See COPYING file for information.

#include "collectiondb.h"      //smart playlists
#include "collectionreader.h"
#include "k3bexporter.h"
#include "metabundle.h"        //paintCell()
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistloader.h"    //PlaylistBrowserItem::load()
#include "smartplaylist.h"
#include "tagdialog.h"         //showContextMenu()
#include "threadweaver.h"

#include <qevent.h>            //customEvent()
#include <qfile.h>             //loadPlaylists(), renamePlaylist()
#include <qheader.h>           //mousePressed()
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qsplitter.h>
#include <qtextstream.h>       //loadPlaylists(), saveM3U(), savePLS()
#include <qtimer.h>            //loading animation

#include <kaction.h>
#include <kactionclasses.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kio/job.h>           //deleteSelectedPlaylists()
#include <kdebug.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kiconloader.h>       //smallIcon
#include <klocale.h>
#include <kmessagebox.h>       //renamePlaylist(), deleteSelectedPlaylist()
#include <kpopupmenu.h>
#include <kstandarddirs.h>     //KGlobal::dirs()
#include <ktoolbar.h>
#include <klineedit.h>         //rename()
#include <kurldrag.h>          //dragObject()

#include <stdio.h>             //rename() in renamePlaylist()


PlaylistBrowser *PlaylistBrowser::s_instance = 0;


PlaylistBrowser::PlaylistBrowser( const char *name )
    : QVBox( 0, name )
    , lastPlaylist( 0 )
{
    setMargin( 4 );

    s_instance = this;

    m_splitter = new QSplitter( Vertical, this );

    QVBox *browserBox = new QVBox( m_splitter );
    //<Toolbar>
    m_ac = new KActionCollection( this );
    KAction *open = new KAction( i18n("Add Playlist..."), "fileopen", 0, this, SLOT( openPlaylist() ), m_ac, "Open" );
    renameButton = new KAction( i18n("Rename"), "editclear", 0, this, SLOT( renameSelectedPlaylist() ), m_ac, "Rename" );
    removeButton = new KAction( i18n("Remove"), "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac, "Remove" );
    deleteButton = new KAction( i18n("Delete"), "editdelete", 0, this, SLOT( deleteSelectedPlaylists() ), m_ac, "Delete" );
    viewMenuButton = new KActionMenu( i18n("View"), "configure", m_ac );
    viewMenuButton->setDelayed( false );
    KPopupMenu *viewMenu = viewMenuButton->popupMenu();
    viewMenu->setCheckable( true );
    viewMenu->insertItem( i18n("Detailed View"), DetailedView );
    viewMenu->insertItem( i18n("List View"), ListView );
    viewMenu->insertSeparator();
    viewMenu->insertItem( i18n("Unsorted"), Unsorted );
    viewMenu->insertItem( i18n("Sort Ascending"), SortAscending );
    viewMenu->insertItem( i18n("Sort Descending"), SortDescending );
    viewMenu->setItemChecked( Unsorted, true );
    connect( viewMenu, SIGNAL( activated(int) ), SLOT( slotViewMenu(int) ) );

    m_toolbar = new KToolBar( browserBox );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );

    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    open->plug( m_toolbar );

    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    renameButton->plug( m_toolbar);
    removeButton->plug( m_toolbar );
    deleteButton->plug( m_toolbar);
    m_toolbar->insertLineSeparator();
    viewMenuButton->plug( m_toolbar );

    renameButton->setEnabled( false );
    removeButton->setEnabled( false );
    deleteButton->setEnabled( false );
    //</Toolbar>

    m_listview = new PlaylistBrowserView( browserBox );

    // Create item representing the current playlist
    KURL url;
    url.setPath( i18n( "Current Playlist" ) );
    url.setProtocol( "cur" );
    lastPlaylist = new PlaylistBrowserItem( m_listview, 0, url );
    currentItemChanged( lastPlaylist );

    new SmartPlaylistBox( m_splitter );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
    m_viewMode = (ViewMode)config->readNumEntry( "View", DetailedView );  //restore the view mode
    viewMenu->setItemChecked( m_viewMode, true );
    m_sortMode = config->readNumEntry( "Sorting", SortAscending );
    slotViewMenu( m_sortMode );
    QString str = config->readEntry( "Splitter", "[228,121]" );    //default splitter position
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *m_splitter;     //this sets the splitters position

    // signals and slots connections
    connect( m_listview, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             this,         SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( m_listview, SIGNAL( doubleClicked( QListViewItem *) ),
             this,         SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( m_listview, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,         SLOT( renamePlaylist( QListViewItem*, const QString&, int ) ) );
    connect( m_listview, SIGNAL( currentChanged( QListViewItem * ) ),
             this,         SLOT( currentItemChanged( QListViewItem * ) ) );

    setMinimumWidth( m_toolbar->sizeHint().width() );

    // Load the playlists stats cache
    loadPlaylists();

    // Add default streams playlist as the next item under "Current Playlist"
    lastPlaylist = static_cast<PlaylistBrowserItem*>( m_listview->firstChild() );
    addPlaylist( locate( "data","amarok/data/Cool-Streams.m3u" ) );
    lastPlaylist = static_cast<PlaylistBrowserItem*>( m_listview->lastItem() );
}


PlaylistBrowser::~PlaylistBrowser()
{
    //save the playlists stats cache
    QFile file( playlistCacheFile() );

    if( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        QListViewItemIterator it( m_listview );
        while( it.current() ) {
            if( isPlaylist( *it ) && !isCurrentPlaylist( *it ) ) {
                PlaylistBrowserItem *item = (PlaylistBrowserItem*)*it;
                stream << "File=" + item->url().path();
                stream << "\n";
                stream << item->trackCount();
                stream << ",";
                stream << item->length();
                stream << ",";
                QFileInfo fi( item->url().path() );
                stream << fi.lastModified().toTime_t();
                stream << "\n";
            }
            ++it;
        }

        file.close();
    }

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
    //save view mode
    config->writeEntry( "View", m_viewMode );
    // save sorting mode
    config->writeEntry( "Sorting", m_sortMode );
    //save splitter position
    QString str; QTextStream stream( &str, IO_WriteOnly );
    stream << *m_splitter;
    config->writeEntry( "Splitter", str );
}


QString PlaylistBrowser::playlistCacheFile()
{
    //returns the playlists stats cache file
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "playlistbrowser_save";
}


void PlaylistBrowser::loadPlaylists()
{
    QFile file( playlistCacheFile() );

    //read playlists stats cache containing the number of tracks, the total length in secs and the last modified date
    if( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        QString str, file;
        int tracks=0, length=0;
        QDateTime lastModified;
        KURL auxKURL;

        while ( !( str = stream.readLine() ).isNull() ) {
            if ( str.startsWith( "File=" ) ) {
                file = str.mid( 5 );
            }
            else {
                tracks = str.section( ',', 0, 0 ).toInt();
                length = str.section( ',', 1, 1 ).toInt();
                int time_t = str.section( ',', 2, 2 ).toInt();
                lastModified.setTime_t( time_t );

                QFileInfo fi( file );
                if( fi.exists() ) {
                    if( fi.lastModified() != lastModified )
                        addPlaylist( file ); //load the playlist
                    else {
                        if( lastPlaylist == 0 ) {    //first child
                            removeButton->setEnabled( true );
                            renameButton->setEnabled( true );
                            deleteButton->setEnabled( true );
                        }
                        auxKURL.setPath(file);
                        lastPlaylist = new PlaylistBrowserItem( m_listview, lastPlaylist, auxKURL, tracks, length );
                    }
                }

            }
        }
    }

}


void PlaylistBrowser::openPlaylist() //SLOT
{
    // open a file selector to add playlists to the playlist browser
    QStringList files;
    files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|Playlist Files", this, i18n("Add Playlists") );

    const QStringList::ConstIterator end  = files.constEnd();
    for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )
        addPlaylist( *it );
}


void PlaylistBrowser::slotDoubleClicked( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    if( isPlaylist( item ) ) {
        // open the playlist
        #define  item static_cast<PlaylistBrowserItem *>(item)
        //don't replace, it generally makes people think amaroK behaves like JuK
        //and we don't so they then get really confused about things
        Playlist::instance()->insertMedia( item->tracksURL() );
        #undef item
    } else {
        KURL::List list( static_cast<PlaylistTrackItem *>(item)->url() );
        Playlist::instance()->insertMedia( list, Playlist::DirectPlay );
    }
}


void PlaylistBrowser::addPlaylist( QString path, bool force )
{
    // this function adds a playlist to the playlist browser

    QFile file( path );
    if( !file.exists() ) return;

    bool exists = false;
    QListViewItemIterator it( m_listview );
    while( it.current() ) {
        if( isPlaylist( *it ) && path == ((PlaylistBrowserItem *)*it)->url().path() ) {
            exists = true; //the playlist is already in the playlist browser
            if( force )
                ((PlaylistBrowserItem *)*it)->load();    //reload the playlist
        }
        ++it;
    }

    if( !exists ) {
        if( lastPlaylist == 0 ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
            deleteButton->setEnabled( true );
        }
        KURL auxKURL;
        auxKURL.setPath(path);
        lastPlaylist = new PlaylistBrowserItem( m_listview, lastPlaylist, auxKURL );
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
        // if the playlist containing this item is already selected the current item will be skipped
        // it will be deleted from the parent
        QListViewItem *parent = it.current()->parent();
        if( parent && parent->isSelected() )
            continue;

        selected.append( it.current() );
    }

    for( QListViewItem *item = selected.first(); item; item = selected.next() ) {

        if ( isCurrentPlaylist( item ) ) continue;

        if( isPlaylist( item ) ) {
            //remove the playlist
            if( item == lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                lastPlaylist = above ? (PlaylistBrowserItem *)above : 0;
            }
            delete item;
        }
        else {
            //remove the track
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

    // Current playlist saving
    if ( isCurrentPlaylist( item ) ) {
        // TODO Remove this hack for 1.2. It's needed because playlists was a file once.
        QString folder = KGlobal::dirs()->saveLocation( "data", "amarok/playlists", false );
        QFileInfo info( folder );
        if ( !info.isDir() ) QFile::remove( folder );

        QString path = KGlobal::dirs()->saveLocation( "data", "amarok/playlists/", true ) + newName + ".m3u";
        kdDebug() << "[PlaylistBrowser] Saving Current-Playlist to: " << path << endl;
        Playlist::instance()->saveM3U( path );
        item->setText( 0, i18n( "Current Playlist" ) );
        addPlaylist( path );
        return;
    }

    QString oldPath = item->url().path();
    QString newPath = fileDirPath( oldPath ) + newName + fileExtension( oldPath );

    if ( rename( QFile::encodeName( oldPath ), QFile::encodeName( newPath ) ) == -1 )
        KMessageBox::error( this, i18n("Error renaming the file.") );
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
                        i18n("&Delete") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( removeSelectedItems() ) );
    }
}


void PlaylistBrowser::savePlaylist( PlaylistBrowserItem *item )
{
    bool append = false;

    if( item->trackList().count() == 0 ) //the playlist hasn't been loaded so we append the dropped tracks
        append = true;

    //save the modified playlist in m3u or pls format
    const QString ext = fileExtension( item->url().path() );
    if( ext.lower() == ".m3u" )
        saveM3U( item, append );
    else
        savePLS( item, append );

    item->setModified( false );    //don't show the save icon
}


void PlaylistBrowser::saveM3U( PlaylistBrowserItem *item, bool append )
{
    QFile file( item->url().path() );

    if( append ? file.open( IO_WriteOnly | IO_Append ) : file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        if( !append )
            stream << "#EXTM3U\n";
        QPtrList<TrackItemInfo> trackList = append ? item->droppedTracks() : item->trackList();
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


void PlaylistBrowser::savePLS( PlaylistBrowserItem *item, bool append )
{
    QFile file( item->url().path() );

    if( append ? file.open( IO_WriteOnly | IO_Append ) : file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        QPtrList<TrackItemInfo> trackList = append ? item->droppedTracks() : item->trackList();
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
    // rename remove and delete buttons are disabled if there are no playlists
    // rename and delete buttons are disabled for track items

    bool enable_remove = false;
    bool enable_rename = false;
    bool enable_delete = false;

    if( !item )
        goto enable_buttons;

    else if( isPlaylist( item ) )
    {
        if ( isCurrentPlaylist( item ) )
            enable_rename = true;
        else {
            enable_remove = true;
            enable_rename = true;
            enable_delete = true;
        }
    }
    else
        enable_remove = true;


    enable_buttons:

    removeButton->setEnabled( enable_remove );
    renameButton->setEnabled( enable_rename );
    deleteButton->setEnabled( enable_delete );
}


void PlaylistBrowser::customEvent( QCustomEvent *e )
{
    //if a playlist is found in collection folders it will be automatically added to the playlist browser

    // the CollectionReader sends a PlaylistFoundEvent when a playlist is found
    CollectionReader::PlaylistFoundEvent* p = (CollectionReader::PlaylistFoundEvent*)e;
    addPlaylist( p->path() );
}


void PlaylistBrowser::slotViewMenu( int id )  //SL0T
{
    if( m_viewMode == (ViewMode) id )
        return;

    switch ( id ) {
        case Unsorted:
            m_sortMode = id;
            m_listview->setSorting( -1 );
            viewMenuButton->popupMenu()->setItemChecked( Unsorted, true );
            viewMenuButton->popupMenu()->setItemChecked( SortAscending, false );
            viewMenuButton->popupMenu()->setItemChecked( SortDescending, false );
            return;
        case SortAscending:
            m_sortMode = id;
            m_listview->setSorting( 0, true );
            viewMenuButton->popupMenu()->setItemChecked( Unsorted, false );
            viewMenuButton->popupMenu()->setItemChecked( SortAscending, true );
            viewMenuButton->popupMenu()->setItemChecked( SortDescending, false );
            return;
        case SortDescending:
            m_sortMode = id;
            m_listview->setSorting( 0, false );
            viewMenuButton->popupMenu()->setItemChecked( Unsorted, false );
            viewMenuButton->popupMenu()->setItemChecked( SortAscending, false );
            viewMenuButton->popupMenu()->setItemChecked( SortDescending, true );
            return;
        default:
            break;
    }

    viewMenuButton->popupMenu()->setItemChecked( m_viewMode, false );
    viewMenuButton->popupMenu()->setItemChecked( id, true );
    m_viewMode = (ViewMode) id;

    QListViewItemIterator it( m_listview );
    for( ; it.current(); ++it )
        it.current()->setup();
}


void PlaylistBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{
    if( !item ) return;

    KPopupMenu menu( this );

    if( isPlaylist( item ) ) {
        if ( isCurrentPlaylist( item ) ) {
        //************* Current Playlist menu ***********
            enum Id { SAVE, CLEAR, RMDUP, BURN_DATACD, BURN_AUDIOCD };

            menu.insertItem( SmallIconSet( "filesave" ), i18n( "&Save" ), SAVE );
            menu.insertItem( SmallIconSet( "view_remove" ), i18n( "&Clear" ), CLEAR );
            menu.insertItem( i18n( "Remove Duplicates / Missing" ), RMDUP );

            menu.insertSeparator();

            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );

            switch( menu.exec( p ) )
            {
                case SAVE:
                    renameSelectedPlaylist();
                    break;
                case CLEAR:
                    Playlist::instance()->clear();
                case RMDUP:
                    Playlist::instance()->removeDuplicates();
                    break;
                case BURN_DATACD:
                    K3bExporter::instance()->exportCurrentPlaylist( K3bExporter::DataCD );
                    break;
                case BURN_AUDIOCD:
                    K3bExporter::instance()->exportCurrentPlaylist( K3bExporter::AudioCD );
                    break;
            }
        }
        //************* Playlist menu ***********
        else {
            #define item static_cast<PlaylistBrowserItem*>(item)
            enum Id { LOAD, ADD, SAVE, RESTORE, RENAME, REMOVE, DELETE };

            menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
            menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
            menu.insertSeparator();
            if( item->isModified() ) {
                menu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
                menu.insertItem( i18n( "Res&tore" ), RESTORE );
                menu.insertSeparator();
            }
            menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
            menu.insertItem( SmallIconSet("editdelete"), i18n( "&Delete" ), DELETE );
            menu.setAccel( Key_Space, LOAD );
            menu.setAccel( Key_F2, RENAME );
            menu.setAccel( Key_Delete, REMOVE );
            menu.setAccel( SHIFT+Key_Delete, DELETE );

            switch( menu.exec( p ) )
            {
                case LOAD:
                    slotDoubleClicked( item );
                    break;
                case ADD:
                    Playlist::instance()->insertMedia( item->tracksURL() );
                    break;
                case SAVE:
                    savePlaylist( item );
                    break;
                case RESTORE:
                    item->restore();
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
    }
    else {
    //******** track menu ***********
        #define item static_cast<PlaylistTrackItem*>(item)

        enum Actions { MAKE, APPEND, QUEUE, BURN_DATACD, BURN_AUDIOCD, REMOVE, INFO };

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), QUEUE );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() && item->url().isLocalFile() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() && item->url().isLocalFile() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIconSet("info"), i18n( "&Track Information" ), INFO );

        switch( menu.exec( p ) ) {
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case APPEND:
                Playlist::instance()->insertMedia( item->url() );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( item->url(), Playlist::Queue );
                break;
            case BURN_DATACD:
                 K3bExporter::instance()->exportTracks( item->url(), K3bExporter::DataCD );
                 break;
            case BURN_AUDIOCD:
                 K3bExporter::instance()->exportTracks( item->url(), K3bExporter::AudioCD );
                 break;
            case REMOVE:
                removeSelectedItems();
                break;
            case INFO:
                if( QFile::exists( item->url().path() ) ) {
                    TagDialog* dialog = new TagDialog( item->url() );
                    dialog->show();
                }
                else KMessageBox::sorry( this, i18n( "This file does not exist: %1" ).arg( item->url().path() ) );
        }
        #undef item
   }


}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserView
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserView::PlaylistBrowserView( QWidget *parent, const char *name )
    : KListView( parent, name )
    , m_marker( 0 )
{
    addColumn( i18n("Playlists") );
    setSelectionMode( QListView::Extended );
    setShowSortIndicator( true );

    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    setFullWidth( true );
    setTreeStepSize( 20 );

    //<loading animation>
    m_loading1 = new QPixmap( locate("data", "amarok/images/loading1.png" ) );
    m_loading2 = new QPixmap( locate("data", "amarok/images/loading2.png" ) );
    m_animationTimer = new QTimer();
    connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );
    //</loading animation>

    connect( this, SIGNAL( mouseButtonPressed ( int, QListViewItem *, const QPoint &, int ) ),
            this, SLOT( mousePressed( int, QListViewItem *, const QPoint &, int ) ) );

    //TODO moving tracks
    //connect( this, SIGNAL( moved(QListViewItem *, QListViewItem *, QListViewItem * )),
    //        this, SLOT( itemMoved(QListViewItem *, QListViewItem *, QListViewItem * )));
}


PlaylistBrowserView::~PlaylistBrowserView()
{
    delete m_animationTimer;
    delete m_loading1;
    delete m_loading2;
}


void PlaylistBrowserView::startAnimation( PlaylistBrowserItem *item )
{
    //starts the loading animation for item
    m_loadingItems.append( item );
    if( !m_animationTimer->isActive() )
        m_animationTimer->start( 100 );
}


void PlaylistBrowserView::stopAnimation( PlaylistBrowserItem *item )
{
    //stops the loading animation for item
    m_loadingItems.remove( item );
    if( !m_loadingItems.count() )
        m_animationTimer->stop();
}


void PlaylistBrowserView::slotAnimation() //SLOT
{
    static uint iconCounter=1;

    for( QListViewItem *item = m_loadingItems.first(); item; item = m_loadingItems.next() )
        ((PlaylistBrowserItem *)item)->setLoadingPix( iconCounter==1 ? m_loading1 : m_loading2 );

    iconCounter++;
    if( iconCounter > 2 )
        iconCounter = 1;
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
        eraseMarker();
        return;
    }

    //only for track items (for playlist items we draw the highlighter)
    if( !isPlaylist( item ) && p.y() - itemRect( item ).top() < (item->height()/2) )
        item = item->itemAbove();

    if( item != m_marker )
    {
        eraseMarker();
        m_marker = item;
        viewportPaintEvent( 0 );
    }
}


void PlaylistBrowserView::contentsDragLeaveEvent( QDragLeaveEvent* )
{
     eraseMarker();
}


void PlaylistBrowserView::contentsDropEvent( QDropEvent *e )
{
    QListViewItem *parent = 0;
    QListViewItem *after;

    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item ) {
        eraseMarker();
        return;
    }

    if( !isPlaylist( item ) )
        findDrop( e->pos(), parent, after );

    eraseMarker();

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


void PlaylistBrowserView::eraseMarker() //SLOT
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

        QPoint p = mapFromGlobal( pnt );
        p.setY( p.y() - header()->height() );

        QRect itemrect = itemRect( item );

        QRect expandRect = QRect( 4, itemrect.y() + (item->height()/2) - 5, 15, 15 );
        if( expandRect.contains( p ) ) {    //expand symbol clicked
            setOpen( item, !item->isOpen() );
            return;
        }

        if( static_cast<PlaylistBrowserItem*>(item)->isModified() ) {
            QRect saveRect = QRect( 23, itemrect.y() + 3, 16, 16 );
            if( saveRect.contains( p ) ) {

                enum Id { SAVE, RESTORE };

                KPopupMenu saveMenu( this );
                saveMenu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
                saveMenu.insertItem( i18n( "&Restore" ), RESTORE );

                switch( saveMenu.exec( pnt ) ) {
                    case SAVE:
                        PlaylistBrowser::instance()->savePlaylist( static_cast<PlaylistBrowserItem*>(item) );
                        break;

                    case RESTORE:
                        static_cast<PlaylistBrowserItem*>(item)->restore();
                        break;
                }
            }

        }

    }


}


void PlaylistBrowserView::rename( QListViewItem *item, int c )
{
    KListView::rename( item, c );

    QRect rect( itemRect( item ) );
    int fieldX = rect.x() + treeStepSize() + 2;
    int fieldW = rect.width() - treeStepSize() - 2;

    KLineEdit *renameEdit = renameLineEdit();
    renameEdit->setGeometry( fieldX, rect.y(), fieldW, rect.height() );
    renameEdit->show();
}


void PlaylistBrowserView::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {
         case Key_Space:    //load
            if ( isCurrentPlaylist( currentItem() ) )
                rename( currentItem(), 0 );
            else
                PlaylistBrowser::instance()->slotDoubleClicked( currentItem() );
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


void PlaylistBrowserView::startDrag()
{
    KURL::List urls;

    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        if( isPlaylist( *it ) )
            urls += ((PlaylistBrowserItem*)*it)->tracksURL();
        else
            urls += ((PlaylistTrackItem*)*it)->url();
    }

    KURLDrag *d = new KURLDrag( urls, viewport() );
    d->dragCopy();

}



/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserItem
////////////////////////////////////////////////////////////////////////////

class PlaylistReader : public PlaylistLoader
{
public:
    PlaylistReader( QObject *recipient, const QString &path )
        : PlaylistLoader( KURL::List(), 0, 0 )
        , m_recipient( recipient )
        , m_path( path )
    {}

    virtual bool doJob()
    {
        loadPlaylist( m_path );
        QApplication::postEvent( m_recipient, new DoneEvent( this ) );

        return true;
    }

    virtual void postItem( const KURL &url, const QString &title, const uint length )
    {
        QApplication::postEvent( m_recipient, new ItemEvent( url, title, length ) );
    }

    class ItemEvent : public QCustomEvent
    {
    public:
        ItemEvent( const KURL &url, const QString &title, const uint length )
            : QCustomEvent( PlaylistLoader::Item )
            , url( url )
            , title( title )
            , length( length )
        {}

        const KURL url;
        const QString title;
        const uint length;
    };

private:
    QObject* const m_recipient;
    const QString m_path;
};


PlaylistBrowserItem::PlaylistBrowserItem( KListView *parent, QListViewItem *after, const KURL &url, int tracks, int length )
    : KListViewItem( parent, after )
    , m_url( url )
    , m_length( length )
    , m_trackCount( tracks )
    , m_loading( false )
    , m_loaded( false )
    , m_modified( false )
    , m_savePix( 0 )
    , m_loadingPix( 0 )
    , m_lastTrack( 0 )
{
    m_trackList.setAutoDelete( true );
    tmp_droppedTracks.setAutoDelete( false );

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    //kdDebug() << fileBaseName( url.path() ) << endl;
    setText(0, fileBaseName( url.path() ) );
    if( m_url.protocol() != "cur" )
        setPixmap( 0, SmallIcon("player_playlist_2") );

    if( !m_trackCount )
        load();   //load the playlist file
}


PlaylistBrowserItem::~PlaylistBrowserItem()
{
    m_trackList.clear();
    tmp_droppedTracks.setAutoDelete( true );
    tmp_droppedTracks.clear();
}

void PlaylistBrowserItem::load()
{
    m_trackList.clear();
    m_length = 0;
    m_loaded = false;
    m_loading = true;
    //starts loading animation
    ((PlaylistBrowserView *)listView())->startAnimation( this );

     //read the playlist file in a thread
    ThreadWeaver::instance()->queueJob( new PlaylistReader( this, m_url.path() ) );
}


void PlaylistBrowserItem::restore()
{
    setOpen( false );

    if( !m_loaded ) {
        TrackItemInfo *info = tmp_droppedTracks.first();
        while( info ) {
            m_length -= info->length();
            m_trackCount--;
            tmp_droppedTracks.remove();    //remove current item
            delete info;
            info = tmp_droppedTracks.current();    //the new current item
        }
    }
    else
        load();    //reload the playlist

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

        TrackItemInfo *newInfo = new TrackItemInfo( *it, title.isEmpty() ? key : title, length );
        m_length += newInfo->length();
        m_trackCount++;

        if( after ) {
            m_trackList.insert( pos+k, newInfo );
            if( isOpen() )
                after = new PlaylistTrackItem( this, after, newInfo );
        }
        else {
            if( m_loaded ) {
                m_trackList.append( newInfo );
                if( isOpen() )  //append the track item to the playlist
                    m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, newInfo );
            }
            else
                tmp_droppedTracks.append( newInfo );
        }

    }

    setModified( true );    //show a save icon to save changes
}


void PlaylistBrowserItem::removeTrack( QListViewItem *item )
{
    #define item static_cast<PlaylistTrackItem*>(item)
    //remove a track and update playlist stats
    TrackItemInfo *info = item->trackInfo();
    m_length -= info->length();
    m_trackCount--;
    m_trackList.remove( info );
    if( item == m_lastTrack ) {
        QListViewItem *above = item->itemAbove();
        m_lastTrack = above ? (PlaylistTrackItem *)above : 0;
    }
    delete item;

    #undef item

    setModified( true );    //show a save icon to save changes
}


void PlaylistBrowserItem::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
        case PlaylistLoader::Item:  {
            #define e static_cast<PlaylistReader::ItemEvent*>(e)
            TrackItemInfo *info = new TrackItemInfo( e->url, e->title, e->length );
            m_trackList.append( info );
            m_length += info->length();
            if( isOpen() )
                m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
            #undef e
            break;
       }

        case PlaylistLoader::Done: {

            //the tracks dropped on the playlist while it wasn't loaded are added to the track list
            if( tmp_droppedTracks.count() ) {
                for ( TrackItemInfo *info = tmp_droppedTracks.first(); info; info = tmp_droppedTracks.next() ) {
                    m_trackList.append( info );
                    m_length += info->length();
                }
                tmp_droppedTracks.clear();
            }

            m_loading = false;
            m_loaded = true;
            ((PlaylistBrowserView *)listView())->stopAnimation( this );  //stops the loading animation

            if( m_trackCount ) setOpen( true );
            else repaint();

            m_trackCount = m_trackList.count();

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

        if( m_loaded ) {
            //create track items
            for ( TrackItemInfo *info = m_trackList.first(); info; info = m_trackList.next() )
                m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
        }
        else {
            load();
            return;
        }

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


int PlaylistBrowserItem::compare( QListViewItem* i, int /*col*/, bool ascending ) const
{
    PlaylistBrowserItem* item = static_cast<PlaylistBrowserItem*>(i);

    // Special case for "Current Playlist", to keep it always at first position

    if ( url().protocol() == "cur" )
        return ascending ? -1 : 1;
    else if ( item->url().protocol() == "cur" )
        return ascending ? 1 : -1;

    // Compare case-insensitive
    return QString::localeAwareCompare( text( 0 ).lower(), item->text( 0 ).lower() );
}


KURL::List PlaylistBrowserItem::tracksURL()
{
    KURL::List list;

    if( m_loaded )  { //playlist loaded
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
    if ( h % 2 > 0 )
        h++;
    if( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DetailedView )
        setHeight( h + fm.lineSpacing() + margin );
    else
        setHeight( h + margin );
}


void PlaylistBrowserItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align)
{
    bool detailedView = PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DetailedView;

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

    if( detailedView ) {
        // draw a line at the top
        pBuf.setPen( cg.text() );
        pBuf.drawLine( 0, 0, width, 0 );
    }

    KListView *lv = (KListView *)listView();

    QRect rect( ((lv->treeStepSize()-9) / 2) + 1, (height()-9) / 2, 9, 9 );

    if( m_loading && m_loadingPix ) {
        pBuf.drawPixmap( (lv->treeStepSize() - m_loadingPix->width())/2,
                         (height() - m_loadingPix->height())/2,
                         *m_loadingPix );
    }
    else if( m_trackCount ) {
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

    // Use underlined font for "Current Playlist"
    if ( m_url.protocol() == "cur" )
        font.setUnderline( true );

    // Use italic font for "Cool-Streams"
    if ( text( 0 ) == "Cool-Streams" )
        font.setItalic( true );

    int text_x = lv->treeStepSize() + 3;
    int textHeight = detailedView ? fm.lineSpacing() + lv->itemMargin() + 1 : height();
    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );

    //if the playlist has been modified a save icon is shown
    if( m_modified && m_savePix ) {
        pBuf.drawPixmap( text_x, (textHeight - m_savePix->height())/2, *m_savePix );
        text_x += m_savePix->width()+4;
    } else if( pixmap(0) ) {
        int y = (textHeight - pixmap(0)->height())/2;
        if( detailedView ) y++;
        pBuf.drawPixmap( text_x, y, *pixmap(0) );
        text_x += pixmap(0)->width()+4;
    }

    // draw the playlist name in bold
    font.setBold( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DetailedView );
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

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( detailedView ) {
        QString info;

        text_x = lv->treeStepSize() + 3;
        font.setBold( false );
        pBuf.setFont( font );

        if ( m_url.protocol() != "cur" )
        {
            if( m_loading )
                info = i18n( "Loading..." );
            else
            {     //playlist loaded
                // draw the number of tracks and the total length of the playlist
                info += i18n("1 Track", "%n Tracks", m_trackCount);
                if( m_length )
                    info += QString(" - [%2]").arg( MetaBundle::prettyTime( m_length ) );
            }

            pBuf.drawText( text_x, textHeight, width, fm.lineSpacing(), AlignVCenter, info);
        }
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

#include "playlistbrowser.moc"
