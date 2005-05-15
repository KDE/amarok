// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// License: GPL V2. See COPYING file for information.

#include "amarok.h"            //actionCollection()
#include "browserToolBar.h"
#include "playlist.h"
#include "collectiondb.h"      //smart playlists
#include "collectionreader.h"
#include "k3bexporter.h"
#include "metabundle.h"        //paintCell()
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "smartplaylist.h"
#include "statusbar.h"         //For notifications, TODO:Remove me once all completed.
#include "tagdialog.h"         //showContextMenu()
#include "threadweaver.h"

#include <qevent.h>            //customEvent()
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
#include <kdebug.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kio/job.h>           //deleteSelectedPlaylists()
#include <kiconloader.h>       //smallIcon
#include <klineedit.h>         //rename()
#include <klocale.h>
#include <kmessagebox.h>       //renamePlaylist(), deleteSelectedPlaylist()
#include <kpopupmenu.h>
#include <kstandarddirs.h>     //KGlobal::dirs()
#include <kurldrag.h>          //dragObject()

#include <stdio.h>             //rename() in renamePlaylist()


PlaylistBrowser *PlaylistBrowser::s_instance = 0;


PlaylistBrowser::PlaylistBrowser( const char *name )
        : QVBox( 0, name )
        , m_lastPlaylist( 0 )
{
    s_instance = this;

    m_splitter = new QSplitter( Vertical, this );

    QVBox *browserBox = new QVBox( m_splitter );
    browserBox->setSpacing( 3 );

    //<Toolbar>
    m_ac = new KActionCollection( this );

    addMenuButton  = new KActionMenu( i18n("Add Source"), "fileopen", m_ac );
    addMenuButton->setDelayed( false );

    KPopupMenu *addMenu  = addMenuButton->popupMenu();
    addMenu->insertItem( i18n("Playlist"), PLAYLIST );
    addMenu->insertItem( i18n("Stream"), STREAM );
    addMenu->insertItem( i18n("Smart Playlist"), SMARTPLAYLIST );
    connect( addMenu, SIGNAL( activated(int) ), SLOT( slotAddMenu(int) ) );

    saveCurrentButton = new KAction( i18n("Save Current"), "filesave", 0, this, SLOT( saveCurrentPlaylist() ), m_ac, "SaveCurrent" );

    renameButton   = new KAction( i18n("Rename"), "editclear", 0, this, SLOT( renameSelectedPlaylist() ), m_ac, "Rename" );
    removeButton   = new KAction( i18n("Remove"), "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac, "Remove" );
    deleteButton   = new KAction( i18n("Delete"), "editdelete", 0, this, SLOT( deleteSelectedPlaylists() ), m_ac, "Delete" );

    viewMenuButton = new KActionMenu( i18n("View"), "configure", m_ac );
    viewMenuButton->setDelayed( false );

    KPopupMenu *viewMenu = viewMenuButton->popupMenu();

    viewMenu->setCheckable( true );
    viewMenu->insertItem( i18n("Detailed View"), DETAILEDVIEW );
    viewMenu->insertItem( i18n("List View"), LISTVIEW );
    viewMenu->insertSeparator();
    viewMenu->insertItem( i18n("Unsorted"), UNSORTED );
    viewMenu->insertItem( i18n("Sort Ascending"), ASCENDING );
    viewMenu->insertItem( i18n("Sort Descending"), DESCENDING );
    viewMenu->setItemChecked( UNSORTED, true );
    connect( viewMenu, SIGNAL( activated(int) ), SLOT( slotViewMenu(int) ) );

    m_toolbar = new Browser::ToolBar( browserBox );
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    addMenuButton->plug( m_toolbar );

    m_toolbar->insertLineSeparator();
    saveCurrentButton->plug( m_toolbar );

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
    new SmartPlaylistBox( m_splitter );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
    m_viewMode = (ViewMode)config->readNumEntry( "View", DETAILEDVIEW );  //restore the view mode
    viewMenu->setItemChecked( m_viewMode, true );
    m_sortMode = config->readNumEntry( "Sorting", ASCENDING );
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

    m_playlistCategory = new PlaylistCategory( m_listview, 0, i18n( "Playlists" ) );
    m_streamsCategory  = new PlaylistCategory( m_listview, m_playlistCategory, i18n( "Streams" ) );
    m_smartCategory    = new PlaylistCategory( m_listview, m_streamsCategory,  i18n( "Smart Playlists" ) );
    m_partyCategory    = new PlaylistCategory( m_listview, m_smartCategory,    i18n( "Parties" ) );

    loadPlaylists();
    loadStreams();
    loadSmartPlaylists();
}


PlaylistBrowser::~PlaylistBrowser()
{
    //save the playlists stats cache
    QFile playlistFile( playlistCacheFile() );
    QFile streamFile( streamCacheFile() );

    // Either file needs to be opened to be written to.
    bool streamWrite   = streamFile.open( IO_WriteOnly );
    bool playlistWrite = playlistFile.open( IO_WriteOnly );

    if( streamWrite || playlistWrite )
    {
        QTextStream playlistStream( &playlistFile );
        QTextStream streamStream( &streamFile );

        QListViewItemIterator it( m_listview );

        while( it.current() ) {
            if( isStream( *it ) && streamWrite )
            {
                StreamEntry *item = (StreamEntry*)*it;
                streamStream << "Name=" + item->title();
                streamStream << "\n";
                streamStream << "Url="  + item->url().prettyURL();
                streamStream << "\n";
            }
            else if( isPlaylist( *it ) && playlistWrite )
            {
                PlaylistEntry *item = (PlaylistEntry*)*it;
                playlistStream << "File=" + item->url().path();
                playlistStream << "\n";
                playlistStream << item->trackCount();
                playlistStream << ",";
                playlistStream << item->length();
                playlistStream << ",";

                QFileInfo fi( item->url().path() );
                playlistStream << fi.lastModified().toTime_t();
                playlistStream << "\n";
            }
            ++it;
        }

        playlistFile.close();
        streamFile.close();
    }

    KConfig *config = kapp->config();

    config->setGroup( "PlaylistBrowser" );
    config->writeEntry( "View", m_viewMode );
    config->writeEntry( "Sorting", m_sortMode );

    QString str; QTextStream stream( &str, IO_WriteOnly );
    stream << *m_splitter;
    config->writeEntry( "Splitter", str );
}

/**
 *************************************************************************
 *  STREAMS
 *************************************************************************
 **/

QString PlaylistBrowser::streamCacheFile()
{
    return amaroK::saveLocation() + "streambrowser_save";
}

void PlaylistBrowser::loadStreams()
{
    QFile file( streamCacheFile() );

    m_lastPlaylist = 0;

    //read playlists stats cache containing the number of tracks, the total length in secs and the last modified date
    if( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        QString str, file, name = QString::null;
        QString protocol;
        KURL auxKURL;

        while ( !( str = stream.readLine() ).isNull() ) {
            if ( str.startsWith( "Name=" ) ) {
                name = str.mid( 5 );
            }
            else if ( str.startsWith( "Url=" ) ) {
                file = str.mid( 4 );

                auxKURL = KURL::KURL(file);
                m_lastPlaylist = new StreamEntry( m_streamsCategory, m_lastPlaylist, auxKURL, name );
            }
        }
    }

    m_streamsCategory->setOpen( true );
}

void PlaylistBrowser::addStream()
{
    StreamEditor dialog( i18n("Stream"), this );

    if( dialog.exec() == QDialog::Accepted )
        new StreamEntry( m_streamsCategory, 0, dialog.url(), dialog.name() );

}

void PlaylistBrowser::editStreamURL( StreamEntry *item )
{
    StreamEditor dialog( this, item->title(), item->url().prettyURL() );

    if( dialog.exec() == QDialog::Accepted )
    {
        item->setTitle( dialog.name() );
        item->setURL( dialog.url() );
        item->setText(0, dialog.name() );
    }
}
/**
 *************************************************************************
 *  SMART-PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::smartCacheFile()
{
    //returns the file used to store custom smart playlists
    return amaroK::saveLocation() + "smartplaylists";
}

void PlaylistBrowser::addSmartPlaylist() //SLOT
{
    //open a dialog to create a custom smart playlist

}

void PlaylistBrowser::loadSmartPlaylists()
{
    // Logic for loading smart playlists should go here
}

void PlaylistBrowser::editSmartPlaylist()
{
    // Logic for editing smart playlists should go here
}

/**
 *************************************************************************
 *  PARTIES
 *************************************************************************
 **/

QString PlaylistBrowser::partyCacheFile()
{
    return amaroK::saveLocation() + "partybrowser_save";
}

void PlaylistBrowser::addPartyConfig()
{
    // Save the current party information for later use
}

void PlaylistBrowser::loadPartyConfigs()
{
    // Load saved and default (random/suggested) party list
}

void PlaylistBrowser::editPartyConfig()
{
    // Edit the chosen party
}


/**
 *************************************************************************
 *  PLAYLISTS
 *************************************************************************
 **/


QString PlaylistBrowser::playlistCacheFile()
{
    //returns the playlists stats cache file
    return amaroK::saveLocation() + "playlistbrowser_save";
}


void PlaylistBrowser::loadPlaylists()
{
    QFile file( playlistCacheFile() );

    m_lastPlaylist = 0;

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
                        if( m_lastPlaylist == 0 ) {    //first child
                            removeButton->setEnabled( true );
                            renameButton->setEnabled( true );
                            deleteButton->setEnabled( true );
                        }
                        auxKURL.setPath(file);
                        m_lastPlaylist = new PlaylistEntry( m_playlistCategory, m_lastPlaylist, auxKURL, tracks, length );
                    }
                }

            }
        }
    }

    m_playlistCategory->setOpen( true );
}


void PlaylistBrowser::addPlaylist( QString path, bool force )
{
    // this function adds a playlist to the playlist browser

    QFile file( path );
    if( !file.exists() ) return;

    bool exists = false;
    for( QListViewItemIterator it( m_listview ); *it; ++it )
        if( isPlaylist( *it ) && path == ((PlaylistEntry *)*it)->url().path() ) {
            exists = true; //the playlist is already in the playlist browser
            if( force )
                ((PlaylistEntry *)*it)->load(); //reload the playlist
        }

    if( !exists ) {
        if( m_lastPlaylist == 0 ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
            deleteButton->setEnabled( true );
        }
        KURL auxKURL;
        auxKURL.setPath(path);
        m_lastPlaylist = new PlaylistEntry( m_playlistCategory, m_lastPlaylist, auxKURL );
        m_playlistCategory->setOpen( true );
    }
}

void PlaylistBrowser::openPlaylist() //SLOT
{
    // open a file selector to add playlists to the playlist browser
    QStringList files;
    files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|" + i18n("Playlist Files"), this, i18n("Add Playlists") );

    const QStringList::ConstIterator end  = files.constEnd();
    for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )
        addPlaylist( *it );
}

/**
 *************************************************************************
 *  General Methods
 *************************************************************************
 **/

void PlaylistBrowser::saveCurrentPlaylist()
{
    PlaylistSaver dialog( i18n("Current Playlist"), this );

    if( dialog.exec() == QDialog::Accepted )
    {
        QString name = dialog.title();

        // TODO Remove this hack for 1.2. It's needed because playlists was a file once.
        QString folder = KGlobal::dirs()->saveLocation( "data", "amarok/playlists", false );
        QFileInfo info( folder );
        if ( !info.isDir() ) QFile::remove( folder );

        QString path = KGlobal::dirs()->saveLocation( "data", "amarok/playlists/", true ) + name + ".m3u";
        kdDebug() << "[PlaylistBrowser] Saving Current-Playlist to: " << path << endl;
        if ( !Playlist::instance()->saveM3U( path ) ) {
            KMessageBox::sorry( this, i18n( "Cannot write playlist (%1).").arg(path) );
            return;
        }
        addPlaylist( path );
    }

}

void PlaylistBrowser::slotDoubleClicked( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    if( isPlaylist( item ) ) {
        // open the playlist
        #define item static_cast<PlaylistEntry *>(item)
        //don't replace, it generally makes people think amaroK behaves like JuK
        //and we don't so they then get really confused about things
        Playlist::instance()->insertMedia( item->tracksURL(), Playlist::Replace );
        #undef item
    }
    else if( isStream( item ) )
    {
        Playlist::instance()->insertMedia( static_cast<StreamEntry *>(item)->url(), Playlist::Replace );
    }
    else if( isCategory( item ) )
    {
        item->setOpen( !item->isOpen() );
    }
    else if( isPlaylistTrackItem( item ) )
    {
        KURL::List list( static_cast<PlaylistTrackItem *>(item)->url() );
        Playlist::instance()->insertMedia( list, Playlist::DirectPlay );
    }
    else
        //If you remove this, please also remove #include "statusbar.h", thanks.
        amaroK::StatusBar::instance()->shortMessage( i18n("No functionality for item double click implemented") );
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

        if( isPlaylist( item ) ) {
            //remove the playlist
            if( item == m_lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                m_lastPlaylist = above ? (PlaylistEntry *)above : 0;
            }
            delete item;
        }
        if( isStream( item ) ) {
            if( item == m_lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                m_lastPlaylist = above ? (StreamEntry *)above : 0;
            }
            delete item;
        }
        else if( isPlaylistTrackItem( item ) ) {
            //remove the track
            PlaylistEntry *playlist = (PlaylistEntry *)item->parent();
            playlist->removeTrack( item );
        }
    }
}


void PlaylistBrowser::renameSelectedPlaylist() //SLOT
{
    QListViewItem *item = m_listview->currentItem();
    if( !item ) return;

    if( isPlaylist( item ) || isStream( item ) ) {
        item->setRenameEnabled( 0, true );
        m_listview->rename( item, 0 );
    }
}


void PlaylistBrowser::renamePlaylist( QListViewItem* item, const QString& newName, int ) //SLOT
{
    #define item static_cast<PlaylistEntry*>(item)

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
            urls.append( ((PlaylistEntry *)*it)->url() );
        else    //we want to delete only playlists
            m_listview->setSelected( it.current(), false );
    }

    if ( urls.isEmpty() ) return;

    int button = KMessageBox::warningContinueCancel( this, i18n( "<p>You have selected 1 playlist to be <b>irreversibly</b> deleted.",
                                                                 "<p>You have selected %n playlists to be <b>irreversibly</b> deleted.",
                                                                 urls.count() ),
                                                     QString::null,
                                                     KGuiItem(i18n("&Delete"),"editdelete") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( removeSelectedItems() ) );
    }
}


void PlaylistBrowser::savePlaylist( PlaylistEntry *item )
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


void PlaylistBrowser::saveM3U( PlaylistEntry *item, bool append )
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


void PlaylistBrowser::savePLS( PlaylistEntry *item, bool append )
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
        enable_remove = true;
        enable_rename = true;
        enable_delete = true;
    }
    else if( isStream( item ) )
    {
        enable_remove = true;
        enable_rename = true;
        enable_delete = false;
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

void PlaylistBrowser::slotAddMenu( int id ) //SLOT
{
    switch( id )
    {
        case PLAYLIST:
            openPlaylist();
            return;

        case STREAM:
            addStream();
            return;

        case SMARTPLAYLIST:
            addSmartPlaylist();

        default:
            break;
    }
}

void PlaylistBrowser::slotViewMenu( int id )  //SL0T
{
    if( m_viewMode == (ViewMode) id )
        return;

    switch ( id ) {
        case UNSORTED:
            m_sortMode = id;
            m_listview->setSorting( -1 );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, true );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, false );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, false );
            return;
        case ASCENDING:
            m_sortMode = id;
            m_listview->setSorting( 0, true );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, false );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, true );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, false );
            return;
        case DESCENDING:
            m_sortMode = id;
            m_listview->setSorting( 0, false );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, false );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, false );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, true );
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

/**
 ************************
 *  Context Menu Entries
 ************************
 **/

void PlaylistBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{
    if( !item ) return;

    KPopupMenu menu( this );

    if( isPlaylist( item ) ) {
        #define item static_cast<PlaylistEntry*>(item)
        enum Id { LOAD, ADD, SAVE, RESTORE, RENAME, REMOVE, DELETE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertSeparator();
        if( item->isModified() )
        {
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
    else if( isStream( item ) )
    {
        enum Actions { LOAD, ADD, EDIT, REMOVE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet("editclear"), i18n( "E&dit" ), EDIT );
        menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMedia( static_cast<StreamEntry *>(item)->url() );
                break;
            case EDIT:
                editStreamURL( static_cast<StreamEntry *>(item) );
                break;
            case REMOVE:
                removeSelectedItems();
                break;
        }
    }
    else if( isCategory( item ) ) {

        enum Actions { EXPAND, COLLAPSE };

        if( !item->isExpandable() || !item->childCount() )
            return;

        item->isOpen() ?
            menu.insertItem( SmallIconSet( "back" ), i18n("Collapse"), COLLAPSE ) :
            menu.insertItem( SmallIconSet( "forward" ), i18n("Expand"), EXPAND );

        switch( menu.exec( p ) ) {
            case EXPAND:
                item->setOpen( true );
                break;
            case COLLAPSE:
                item->setOpen( false );
                break;
        }
    }
    else if( isPlaylistTrackItem( item ) )
    {
    //******** track menu ***********
        #define item static_cast<PlaylistTrackItem*>(item)

        enum Actions { MAKE, APPEND, QUEUE, BURN_DATACD, BURN_AUDIOCD, REMOVE, INFO };

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), QUEUE );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );


        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() && item->url().isLocalFile() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() && item->url().isLocalFile() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIconSet("info"), i18n( "&View/Edit Meta Information" ), INFO );

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


void PlaylistBrowserView::startAnimation( PlaylistEntry *item )
{
    //starts the loading animation for item
    m_loadingItems.append( item );
    if( !m_animationTimer->isActive() )
        m_animationTimer->start( 100 );
}


void PlaylistBrowserView::stopAnimation( PlaylistEntry *item )
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
        ((PlaylistEntry *)item)->setLoadingPix( iconCounter==1 ? m_loading1 : m_loading2 );

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
                PlaylistEntry *playlist = (PlaylistEntry *)parent;
                playlist->insertTracks( after, list, map );
            }
            else //dropped on a playlist item
            {
                PlaylistEntry *playlist = (PlaylistEntry *)item;
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

        if( static_cast<PlaylistEntry*>(item)->isModified() ) {
            QRect saveRect = QRect( 23, itemrect.y() + 3, 16, 16 );
            if( saveRect.contains( p ) ) {

                enum Id { SAVE, RESTORE };

                KPopupMenu saveMenu( this );
                saveMenu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
                saveMenu.insertItem( i18n( "&Restore" ), RESTORE );

                switch( saveMenu.exec( pnt ) ) {
                    case SAVE:
                        PlaylistBrowser::instance()->savePlaylist( static_cast<PlaylistEntry*>(item) );
                        break;

                    case RESTORE:
                        static_cast<PlaylistEntry*>(item)->restore();
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
            urls += ((PlaylistEntry*)*it)->tracksURL();
        else if ( isStream( *it ) )
            urls += ((StreamEntry*)*it)->url();
        else
            urls += ((PlaylistTrackItem*)*it)->url();
    }

    KURLDrag *d = new KURLDrag( urls, viewport() );
    d->dragCopy();

}

#include "playlistbrowser.moc"
