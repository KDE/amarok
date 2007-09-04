/* This file is part of the KDE project
   Copyright (C) 2004 Mark Kretschmann <markey@web.de>
   Copyright (C) 2003 Alexander Dymo <cloudtemple@mksat.net>
   Copyright (C) 2003 Roberto Raggi <roberto@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "filebrowser.h"

#include "amarok.h"
#include "browserToolBar.h"
#include "collectiondb.h"
#include "collection/CollectionManager.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "kbookmarkhandler.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "medium.h"
#include "mydirlister.h"
#include "mydiroperator.h"
#include "playlistbrowser.h"

#include "playlistloader.h"
#include "playlist/PlaylistModel.h"
#include "statusbar.h"
#include "tagdialog.h"
#include "TheInstances.h"

#include <k3listview.h>
#include <k3urldrag.h>
#include <KAction>
#include <KActionMenu>
#include <KActionCollection>
#include <KConfigGroup>
#include <KDirOperator>
#include <KHBox>
#include <KIconLoader>
#include <KIO/NetAccess>
#include <KLineEdit>
#include <KLocale>
#include <KMenu>
#include <KPushButton>     ///@see SearchPane
#include <KUrlComboBox>    ///@see ctor
#include <KUrlCompletion>

#include <q3iconview.h>
#include <QAbstractItemView>
#include <QDir>
#include <QLabel>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name, Medium * medium )
        : KVBox( 0 )
{
    KActionCollection *actionCollection;
    SearchPane *searchPane;
    setObjectName( name );

    KUrl location;

    // Try to keep filebrowser working even if not in a medium context
    // so if a medium object not passed in, keep earlier behavior
    if (!medium) {
        m_medium = 0;
        location = KUrl( Amarok::config( "Filebrowser" ).readEntry( "Location", QDir::homePath() ) );
        KFileItem *currentFolder = new KFileItem( KFileItem::Unknown, KFileItem::Unknown, location );
        //KIO sucks, NetAccess::exists puts up a dialog and has annoying error message boxes
        //if there is a problem so there is no point in using it anyways.
        //so... setting the diroperator to ~ is the least sucky option
        if ( !location.isLocalFile() || !currentFolder->isReadable() ) {
            location = KUrl( QDir::homePath() ) ;
        }
    }
    else{
        m_medium = medium;
        location = KUrl( m_medium->mountPoint() );
    }

    KActionCollection* ac = new KActionCollection( this );
    KStandardAction::selectAll( this, SLOT( selectAll() ), ac );

    KToolBar *toolbar = new Browser::ToolBar( this );
    toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );

    { //Filter LineEdit
//        KToolBar* searchToolBar = new Browser::ToolBar( this );
//
        m_filter = new KLineEdit( 0 );
        toolbar->addWidget(m_filter);
        m_filter->setClearButtonShown( true );
        m_filter->setClickMessage( i18n( "Enter search terms here" ) );

        m_filter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);

        m_filter->setToolTip( i18n( "Enter space-separated terms to search in the directory-listing" ) );
    }

    { //Directory Listing
        KVBox *container; KHBox *box;

        container = new KVBox( this );
        container->setFrameStyle( m_filter->frameStyle() );
        container->setMargin( 3 );
        container->setSpacing( 2 );
        container->setBackgroundMode( Qt::PaletteBase );

        box = new KHBox( container );
        box->setMargin( 3 );
        box->setBackgroundMode( Qt::PaletteBase );

        //folder selection combo box
        m_combo = new KUrlComboBox( KUrlComboBox::Directories, true, box );
        m_combo->setObjectName( "path combo" );

        if (!m_medium){
            m_combo->setCompletionObject( new KUrlCompletion( KUrlCompletion::DirCompletion ) );
            m_combo->setAutoDeleteCompletionObject( true );
        }
        m_combo->setMaxItems( 9 );
        m_combo->setUrls( Amarok::config( "Filebrowser" ).readEntry( "Dir History", QStringList() ) );

        if (!m_medium)
            m_combo->lineEdit()->setText( location.path() );
        else
            m_combo->lineEdit()->setText( "/" );

        //The main widget with file listings and that
        m_dir = new MyDirOperator( location, container, m_medium );
        m_dir->setEnableDirHighlighting( true );
        m_dir->setMode( KFile::Mode((int)KFile::Files | (int)KFile::Directory) ); //allow selection of multiple files + dirs
        m_dir->setOnlyDoubleClickSelectsFiles( true ); //Amarok type settings
        KConfigGroup cg(Amarok::config( "Filebrowser" ));
        m_dir->readConfig( cg  );
        m_dir->setView( KFile::Default ); //will set userconfigured view, will load URL
        m_dir->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
        m_dir->setAcceptDrops( true );
        //Automatically open folder after hovering above it...probably a good thing
        //but easily disabled by commenting this line out
        //Disabled for now because can't show . and .. folders.
        //TODO: Find out a way to fix this?
        //m_dir->setDropOptions( KFileView::AutoOpenDirs );

        static_cast<QFrame*>(m_dir->viewWidget())->setFrameStyle( QFrame::NoFrame );
        static_cast<Q3IconView*>(m_dir->viewWidget())->setSpacing( 1 );

        actionCollection = m_dir->actionCollection();

        searchPane = new SearchPane( this );

        setStretchFactor( container, 2 );
    }

    {
        KMenu* const menu = static_cast<KActionMenu*>(actionCollection->action("popupMenu"))->menu();

        menu->clear();
        m_createPlaylistAction = menu->addAction( KIcon( Amarok::icon( "files" ) ), i18n( "&Load" ), this, SLOT( slotCreatePlaylist() ) );
        
        menu->addAction( KIcon( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), this, SLOT( slotAppendToPlaylist() ) );
        
        m_queueTracksAction = menu->addAction( KIcon( Amarok::icon( "queue_track" ) ), i18n( "&Queue Track" ), this, SLOT( slotQueueTrack() ) );

        menu->addAction( KIcon( Amarok::icon( "save" ) ), i18n( "&Save as Playlist..." ), this, SLOT( slotSavePlaylist() ) );
        menu->addSeparator();

        m_mediaDeviceAction = 0; // sanity
        if( !m_medium )
            m_mediaDeviceAction = menu->addAction( KIcon( Amarok::icon( "device" ) ), i18n( "&Transfer to Media Device" ), this, SLOT( slotMediaDevice() ) );

        m_organizeFilesAction    = menu->addAction( KIcon( Amarok::icon( "collection" ) ), i18n( "&Organize Files..." ), this, SLOT( slotOrganizeFiles() ) );
        m_copyToCollectionAction = menu->addAction( KIcon( Amarok::icon( "collection" ) ), i18n( "&Copy Files to Collection..." ), this, SLOT( slotCopyToCollection() ) );
        m_moveToCollectionAction = menu->addAction( KIcon( Amarok::icon( "collection" ) ), i18n( "&Move Files to Collection..." ), this, SLOT( slotMoveToCollection() ) );
        
        QAction *burnAction = menu->addAction( KIcon( Amarok::icon( "burn" ) ), i18n("Burn to CD..."), this, SLOT( slotBurnCd() ) );
        burnAction->setEnabled( K3bExporter::isAvailable() );

        menu->addSeparator();
        menu->addAction( i18n( "&Select All Files" ), this, SLOT( selectAll() ) );
        menu->addSeparator();

        QAction *deleteAction = actionCollection->action( "delete" );
        deleteAction->setIcon( KIcon( Amarok::icon( "remove" ) ) );
        menu->addAction( deleteAction );
        
        menu->addSeparator();
        menu->addAction( KIcon( Amarok::icon( "info" ) ), i18nc( "[only-singular]", "Edit Track &Information..." ), this, SLOT( slotEditTags() ) );
        menu->addAction( actionCollection->action( "properties" ) );

        connect( menu, SIGNAL(aboutToShow()), SLOT(prepareContextMenu()) );
    }

    {
        KActionMenu *a;

        a = static_cast<KActionMenu*>( actionCollection->action( "sorting menu" ) );
        a->setIcon( KIcon( Amarok::icon( "configure" ) ) );
        a->setDelayed( false ); //TODO should be done by KDirOperator

        static_cast<KAction*>(actionCollection->action( "delete" ))->setShortcut( KShortcut( Qt::ShiftModifier + Qt::Key_Delete ) );

        a = new KActionMenu( this );

        a->setText( i18n("Bookmarks") );
        a->setIcon(KIcon("bookmark-toolbar"));
        actionCollection->addAction( "bookmarks", a );

        a->setDelayed( false );

        new KBookmarkHandler( m_dir, a->menu() );
    }

    {
        if ( QAction *a = actionCollection->action( "up" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "back" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "forward" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "home" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "reload" ) ) {
            a->setIcon( KIcon(Amarok::icon( "refresh" )) );
            toolbar->addAction(a);
        }

        toolbar->addSeparator();

        if ( QAction *a = actionCollection->action( "short view" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "detailed view" ) )
            toolbar->addAction(a);

        toolbar->addSeparator();

        if ( QAction *a = actionCollection->action( "sorting menu" ) )
            toolbar->addAction(a);
        if ( QAction *a = actionCollection->action( "bookmarks" ) )
            toolbar->addAction(a);


        KAction *gotoCurrent = new KAction( i18n("Go To Current Track Folder"), this );
        actionCollection->addAction("Go To Current Track Folder", gotoCurrent);
        gotoCurrent->setIcon( KIcon(Amarok::icon( "music" )) );
        connect( gotoCurrent, SIGNAL( triggered() ), this, SLOT(  gotoCurrentFolder() ) );

        toolbar->addAction(gotoCurrent);

        //disconnect( actionCollection->action( "up" ), SIGNAL( triggered() ), m_dir, SLOT( cdUp() ) );
        //connect( actionCollection->action( "up" ), SIGNAL( triggered() ), m_dir, SLOT( myCdUp() ) );
        //disconnect( actionCollection->action( "home" ), SIGNAL( triggered() ), m_dir, SLOT( home() ) );
        //connect( actionCollection->action( "home" ), SIGNAL( triggered() ), m_dir, SLOT( myHome() ) );
    }

    connect( m_filter, SIGNAL(textChanged( const QString& )), SLOT(setFilter( const QString& )) );
    connect( m_combo, SIGNAL(urlActivated( const KUrl& )), SLOT(setUrl( const KUrl& )) );
    connect( m_combo, SIGNAL(returnPressed( const QString& )), SLOT(setUrl( const QString& )) );
    connect( m_dir, SIGNAL(viewChanged( KFileView* )), SLOT(slotViewChanged( KFileView* )) );
    connect( m_dir, SIGNAL(fileSelected( const KFileItem* )), SLOT(activate( const KFileItem* )) );
    connect( m_dir, SIGNAL(urlEntered( const KUrl& )), SLOT(urlChanged( const KUrl& )) );
    connect( m_dir, SIGNAL(urlEntered( const KUrl& )), searchPane, SLOT(urlChanged( const KUrl& )) );
    connect( m_dir, SIGNAL(dropped( const KFileItem*, QDropEvent*, const KUrl::List& )),
                        SLOT(dropped( const KFileItem*, QDropEvent*, const KUrl::List& )) );

    setSpacing( 4 );
    setFocusProxy( m_dir ); //so the dirOperator is focused when we get focus events
    // Toolbar is more than 250px wide, SideBar doesn't allow that. -> Resizing issues.
    setMinimumWidth( 250 /* toolbar->sizeHint().width() */ );
}


FileBrowser::~FileBrowser()
{
    KConfigGroup c = Amarok::config( "Filebrowser" );

    c.writeEntry( "Location", m_dir->url().url() );
    c.writeEntry( "Dir History", m_combo->urls() );
}

//END Constructor/Destructor

void FileBrowser::setUrl( const KUrl &url )
{
    m_dir->setFocus();
    if (!m_medium)
        m_dir->setUrl( url, true );
    else {
        QString urlpath = url.isLocalFile() ? url.path() : url.prettyUrl();
        KUrl newURL( urlpath.prepend( m_medium->mountPoint() ).remove("..") );
        //debug() << "set-url-kurl: changing to: " << newURL.path();
        m_dir->setUrl( newURL, true );
    }
}

void FileBrowser::setUrl( const QString &url )
{
    if (!m_medium)
        m_dir->setUrl( KUrl(url), true );
    else{
        KUrl newURL( QString(url).prepend( m_medium->mountPoint() ).remove("..") );
        //debug() << "set-url-qstring: changing to: " << newURL.path();
        m_dir->setUrl( newURL, true );
    }
}

//BEGIN Private Methods

Meta::TrackList FileBrowser::selectedItems()
{
    Meta::TrackList list;

    QList<KFileItem>  source;
    if ( m_dir->selectedItems().count() )
        source = m_dir->selectedItems();
    else {
       //FIXME: not that simplw to do with the new api
       //get all items...,
       //source = m_dir->view()->omdel()->items();
    }

    for( QList<KFileItem>::const_iterator it = source.begin(); it != source.end(); ++it )
        list.append( CollectionManager::instance()->trackForUrl( (*it).url() ) );

    return list;
}

void FileBrowser::playlistFromURLs( const KUrl::List &urls )
{
    QString suggestion;
    if( urls.count() == 1 && QFileInfo( urls.first().path() ).isDir() )
        suggestion = urls.first().fileName();
    else
        suggestion = i18n( "Untitled" );
    const QString path = PlaylistDialog::getSaveFileName( suggestion );
    if( path.isEmpty() )
        return;

    if( PlaylistBrowser::savePlaylist( path, urls ) )
    {
        Amarok::StatusBar::instance()->shortMessage( "Playlist saved to playlist browser" );
    }
}


//END Private Methods


//BEGIN Public Slots

void
FileBrowser::setFilter( const QString &text )
{
    if ( text.isEmpty() )
        m_dir->clearFilter();

    else {
        QString filter;

        const QStringList terms = text.split( ' ', QString::SkipEmptyParts );
        foreach( QString it, terms )
        {
            filter += '*';
            filter += it;
        }
        filter += '*';

        m_dir->setNameFilter( filter );
    }

    m_dir->updateDir();
}

void
FileBrowser::dropped( const KFileItem* /*item*/, QDropEvent* event, const KUrl::List &urls)
{
    //Do nothing right now
    event->ignore();
    //Run into const problems iterating over the list, so copy it to a malleable one
    //(besides, need to filter for local giles)
    KUrl::List list(urls);

    for ( KUrl::List::iterator it = list.begin(); it != list.end(); ){
        if ( m_medium && !(*it).isLocalFile() )
            it = list.erase( it );
        else{
            debug() << "Dropped: " << (*it);
            it++;
        }
    }
}

//END Public Slots


//BEGIN Private Slots

inline void
FileBrowser::urlChanged( const KUrl &u )
{
    //the DirOperator's URL has changed

    QString url = u.isLocalFile() ? u.path() : u.prettyUrl();

    if( m_medium ){
        //remove the leading mountPoint value
        url.remove( 0, m_medium->mountPoint().length() );
    }

    QStringList urls = m_combo->urls();
    urls.removeAll( url );
    urls.prepend( url );

    m_combo->setUrls( urls, KUrlComboBox::RemoveBottom );
}

inline void
FileBrowser::slotViewChanged( KFileView *view )
{
    if( view->widget()->inherits( "K3ListView" ) )
    {
        using namespace Amarok::ColorScheme;

        static_cast<K3ListView*>(view->widget())->setAlternateBackground( AltBase );
    }
}

inline void
FileBrowser::activate( const KFileItem *item )
{
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( item->url() );
    The::playlistModel()->insertOptioned( track, Playlist::AppendAndPlay );
}

inline void
FileBrowser::prepareContextMenu()
{
    const QList<KFileItem> items = m_dir->selectedItems();
    m_createPlaylistAction->setVisible( items.count() > 1 || ( items.count() == 1 && items.first().isDir() ) );

    if( items.count() == 1 )
        m_queueTracksAction->setText( i18n( "Queue Track" ) );
    else if( items.count() > 1 )
        m_queueTracksAction->setText( i18n( "Queue Tracks" ) );
    
    m_queueTracksAction->setVisible( items.count() > 0 );
    m_mediaDeviceAction->setVisible( MediaBrowser::isAvailable() );
    m_copyToCollectionAction->setVisible( CollectionDB::instance()->isDirInCollection( url().path() ) );
    m_moveToCollectionAction->setVisible( CollectionDB::instance()->isDirInCollection( url().path() ) );
    m_organizeFilesAction->setVisible( CollectionDB::instance()->isDirInCollection( url().path() ) );
}

inline void
FileBrowser::slotCreatePlaylist()
{
    The::playlistModel()->insertOptioned( selectedItems(), Playlist::Replace );
}

inline void
FileBrowser::slotSavePlaylist()
{
//PORT 2.0    playlistFromURLs( selectedItems() );
}

inline void
FileBrowser::slotAppendToPlaylist()
{
    The::playlistModel()->insertOptioned( selectedItems(), Playlist::Append );
}

inline void
FileBrowser::slotQueueTracks()
{
    The::playlistModel()->insertOptioned( selectedItems(), Playlist::Queue );
}

inline void
FileBrowser::slotEditTags()
{
    /*
    KUrl::List list = Amarok::recursiveUrlExpand( selectedItems() );
    TagDialog *dialog = 0;
    
    if( list.count() == 1 )
        dialog = new TagDialog( list.first(), this );
    else
        dialog = new TagDialog( list, this );
    dialog->show();
    */
}

inline void
FileBrowser::slotCopyToCollection()
{
//PORT 2.0        CollectionView::instance()->organizeFiles( selectedItems(), i18n( "Copy Files To Collection" ), true );
}

inline void
FileBrowser::slotMoveToCollection()
{
//PORT 2.0         CollectionView::instance()->organizeFiles( selectedItems(), i18n( "Move Files To Collection" ), false );
}

inline void
FileBrowser::slotOrganizeFiles()
{
//PORT 2.0         CollectionView::instance()->organizeFiles( selectedItems(), i18n( "Organize Collection Files" ), false );
}

inline void
FileBrowser::slotMediaDevice()
{
//PORT 2.0        MediaBrowser::queue()->addUrls( selectedItems() );
}

inline void
FileBrowser::slotBurnCd()
{
//PORT 2.0        K3bExporter::instance()->exportTracks( selectedItems() );
}

inline void
FileBrowser::gotoCurrentFolder()
{
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;
    const KUrl &url = track->playableUrl();
    KUrl dirURL = KUrl( url.directory() );

    m_combo->setUrl( dirURL );
    setUrl( dirURL );
}

//END Private Slots

void
FileBrowser::selectAll()
{

    //FIME: B0rked with the new KDirOpperator API
    /*KFileItemList list( *m_dir->view()->items() );

    // Select all items which represent files
    for( KFileItemList::const_iterator it = list.begin(); it != list.end(); ++it)
        m_dir->view()->setSelected( *it, (*it)->isFile() );
*/
}

#include <QPainter>
#include <q3simplerichtext.h>

class KURLView : public K3ListView
{
public:
    KURLView( QWidget *parent ) : K3ListView( parent )
    {
        reinterpret_cast<QWidget*>(header())->hide();
        addColumn( QString() );
        setResizeMode( K3ListView::LastColumn );
        setDragEnabled( true );
        setSelectionMode( Q3ListView::Extended );
    }

    class Item : public K3ListViewItem {
    public:
        Item( const KUrl &url, KURLView *parent ) : K3ListViewItem( parent, url.fileName() ), m_url( url ) {}
        KUrl m_url;
    };

    virtual Q3DragObject *dragObject()
    {
        QList<Q3ListViewItem*> items = selectedItems();
        KUrl::List urls;
        for( QList<Q3ListViewItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
            urls += ( static_cast<Item*>(*it) )->m_url;
        }

        return new K3URLDrag( urls, this );
    }

    virtual void viewportPaintEvent( QPaintEvent *e )
    {
        K3ListView::viewportPaintEvent( e );

        if ( childCount() == 0 ) {
            QPainter p( viewport() );

            if ( m_text.isEmpty() ) {
                //TODO Perhaps it's time to put this in some header, as we use it in three places now
                Q3SimpleRichText t( i18n(
                        "<div align=center>"
                            "Enter a search term above; you can use wildcards like * and ?"
                        "</div>" ), font() );

                t.setWidth( width() - 50 );

                const uint w = t.width() + 20;
                const uint h = t.height() + 20;

                p.setBrush( colorGroup().background() );
                p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );
                t.draw( &p, 20, 20, QRect(), colorGroup() );
            }
            else {
                p.setPen( palette().color( QPalette::Disabled, QColorGroup::Text ) );
                p.drawText( rect(), Qt::AlignCenter | Qt::TextWordWrap, m_text );
            }
        }
    }

    void unsetText() { setText( QString() ); }
    void setText( const QString &text ) { m_text = text; viewport()->update(); }

private:
    QString m_text;
};



SearchPane::SearchPane( FileBrowser *parent )
        : KVBox( parent )
        , m_lineEdit( 0 )
        , m_listView( 0 )
        , m_lister( 0 )
{
    KVBox *container = new KVBox( this );
    container->setObjectName( "container" );
    container->hide();

    {
        KHBox *box = new KHBox( container );
        //box->setMargin( 5 );
        box->setBackgroundMode( Qt::PaletteBase );

        m_lineEdit = new KLineEdit( box );
        m_lineEdit->setClickMessage( i18n("Search here...") );
        connect( m_lineEdit, SIGNAL(textChanged( const QString& )), SLOT(searchTextChanged( const QString& )) );

        m_listView = new KURLView( container );

        container->setFrameStyle( m_listView->frameStyle() );
        //container->setMargin( 5 );
        container->setBackgroundMode( Qt::PaletteBase );

        m_listView->setFrameStyle( QFrame::NoFrame );
        connect( m_listView, SIGNAL(executed( Q3ListViewItem* )), SLOT(activate( Q3ListViewItem* )) );
    }

    KPushButton *button = new KPushButton( KGuiItem( i18n("&Show Search Panel"), "find" ), this );
    button->setToggleButton( true );
    connect( button, SIGNAL(toggled( bool )), SLOT(toggle( bool )) );

    m_lister = new MyDirLister( true /*delay mimetypes*/ );
    m_lister->setParent( this );
    connect( m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(searchMatches( const KFileItemList& )) );
    connect( m_lister, SIGNAL(completed()), SLOT(searchComplete()) );
}

void
SearchPane::toggle( bool toggled )
{
    if ( toggled )
        m_lineEdit->setFocus();

    findChild<QWidget*>("container")->setVisible( toggled );
}

void
SearchPane::urlChanged( const KUrl& )
{
    searchTextChanged( m_lineEdit->text() );
}

void
SearchPane::searchTextChanged( const QString &text )
{
    //TODO if user changes search directory then we need to update the search too

    m_lister->stop();
    m_listView->clear();
    m_dirs.clear();

    if ( text.isEmpty() ) {
        m_listView->unsetText();
        return;
    }

    m_filter = QRegExp( text.contains( "*" ) ? text : '*'+text+'*', false, true );

    m_lister->openUrl( searchURL() );

    m_listView->setText( i18n( "Searching..." ) );
}

void
SearchPane::searchMatches( const KFileItemList &list )
{
    for( KFileItemList::ConstIterator it = list.begin(), end = list.end(); it != end; ++it ) {
        if( (*it)->isDir() )
            m_dirs += (*it)->url();
        else if( m_filter.exactMatch( (*it)->name() ) )
            new KURLView::Item( (*it)->url(), static_cast<KURLView*>( m_listView ) );
    }
}

void
SearchPane::searchComplete()
{
    //KDirLister crashes if you call openUrl() from a slot
    //connected to KDirLister::complete()
    //TODO fix crappy KDElibs

    QTimer::singleShot( 0, this, SLOT(_searchComplete()) );
}

void
SearchPane::_searchComplete()
{
    if ( !m_dirs.isEmpty() ) {
        KUrl url = m_dirs.first();
        m_dirs.pop_front();
        m_lister->openUrl( url );
    }
    else
        m_listView->setText( i18n("No results found") ); //only displayed if the listview is empty
}

void
SearchPane::activate( Q3ListViewItem *item )
{
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( static_cast<KURLView::Item*>(item)->m_url );
    The::playlistModel()->insertOptioned( track, Playlist::DirectPlay );
}

#include "filebrowser.moc"
