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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "amarok.h"
#include "clicklineedit.h"
#include "enginecontroller.h"
#include "filebrowser.h"
#include "k3bexporter.h"
#include <kaction.h>
#include <kapplication.h>
#include "kbookmarkhandler.h"
#include <kdiroperator.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kpushbutton.h>     ///@see SearchPane
#include <ktoolbarbutton.h>  ///@see ctor
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include "playlist.h"
#include "playlistloader.h"
#include <qdir.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>


//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)


class MyDirLister : public KDirLister {
public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister( delayedMimeTypes ) {}

protected:
    virtual bool MyDirLister::matchesMimeFilter( const KFileItem *item ) const {
        return
            item->isDir() ||
            EngineController::canDecode( item->url().path() ) ||
            item->url().protocol() == "audiocd" ||
            PlaylistLoader::isPlaylist( item->url() );
    }
};

class MyDirOperator : public KDirOperator {
public:
    MyDirOperator( const KURL &url, QWidget *parent ) : KDirOperator( url, parent ) {
        setDirLister( new MyDirLister( true ) );
    }
};


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
    : QVBox( 0, name )
    , m_timer( new QTimer( this ) )
{
    setSpacing( 4 );
    setMargin( 5 );

    KConfig* const config = amaroK::config( "Filebrowser" );

    m_toolbar = new FileBrowser::ToolBar( this );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconText( KToolBar::IconOnly );
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );

    QString currentLocation = config->readEntry( "Location" );
    if ( !QDir( currentLocation ).exists() )
        currentLocation = QDir::homeDirPath();

    { //Filter LineEdit
        KToolBarButton *button;
        KToolBar* searchToolBar = new KToolBar( this );
        searchToolBar->setMovingEnabled(false);
        searchToolBar->setFlat(true);
        searchToolBar->setIconSize( 16 );
        searchToolBar->setEnableContextMenu( false );

        button       = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_filterEdit = new ClickLineEdit( i18n( "Filter here..." ), searchToolBar );
        searchToolBar->setStretchableWidget( m_filterEdit );

        connect( button, SIGNAL( clicked() ), m_filterEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_filterEdit, i18n( "Space-separated terms will be used to filter the directory-listing" ) );
    }

    {
        QVBox *container; QHBox *box;

        container = new QVBox( this );
        container->setFrameStyle( m_filterEdit->frameStyle() );
        container->setMargin( 3 );
        container->setSpacing( 2 );
        container->setBackgroundMode( Qt::PaletteBase );

        box = new QHBox( container );
        box->setMargin( 3 );
        box->setBackgroundMode( Qt::PaletteBase );

        //folder selection combo box
        m_cmbPath = new KURLComboBox( KURLComboBox::Directories, true, box, "path combo" );
        m_cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
        m_cmbPath->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
        m_cmbPath->setMaxItems( 9 );
        m_cmbPath->setURLs( config->readListEntry( "Dir History" ) );
        m_cmbPath->lineEdit()->setText( currentLocation );

        //The main widget with file listings and that
        m_dir = new MyDirOperator( KURL(currentLocation), container );
        m_dir->setEnableDirHighlighting( true );
        m_dir->setMode( KFile::Mode((int)KFile::Files | (int)KFile::Directory) ); //allow selection of multiple files + dirs
        m_dir->setOnlyDoubleClickSelectsFiles( true ); //amaroK type settings
        m_dir->readConfig( config );
        m_dir->setView( KFile::Default ); //will set userconfigured view, will load URL
        m_dir->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

        static_cast<QFrame*>(m_dir->viewWidget())->setFrameStyle( QFrame::NoFrame );

        connect( m_dir, SIGNAL(urlEntered( const KURL& )), SLOT(dirUrlEntered( const KURL& )) );

        setStretchFactor( container, 2 );
    }

    KActionCollection *actionCollection = m_dir->actionCollection();

    new SearchPane( this );

    //insert our own actions at front of context menu
    QPopupMenu* const menu = ((KActionMenu*)actionCollection->action("popupMenu"))->popupMenu();
    menu->clear();
    menu->insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), this, SLOT(addToPlaylist()), 1 );
    menu->insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), this, SLOT(makePlaylist()), 0 );
    menu->insertSeparator();

    enum { BURN_DATACD = 100, BURN_AUDIOCD };
    menu->insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), this, SLOT( burnDataCd() ), 0, BURN_DATACD );
    menu->setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
    menu->insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), this, SLOT( burnAudioCd() ), 0, BURN_AUDIOCD );
    menu->setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );

    menu->insertSeparator();
    menu->insertItem( i18n( "&Select All Files" ), this, SLOT(selectAllFiles()) );
    menu->insertSeparator();
    actionCollection->action( "delete" )->plug( menu );
    menu->insertSeparator();
    actionCollection->action( "properties" )->plug( menu );

    {
        KActionMenu *a;

        a = (KActionMenu*)actionCollection->action( "sorting menu" );
        a->setIcon( "configure" );
        a->setDelayed( false ); //TODO should be done by KDirOperator

        actionCollection->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );

        a = new KActionMenu( i18n("Bookmarks"), "bookmark", actionCollection, "bookmarks" );
        a->setDelayed( false );
        KBookmarkHandler *bookmarkHandler = new KBookmarkHandler( this, a->popupMenu() );
        connect( bookmarkHandler, SIGNAL(openURL( const QString& )), SLOT(setDir( const QString& )) );
    }

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_filterEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_cmbPath, SIGNAL( urlActivated( const KURL&  )), SLOT(cmbPathActivated( const KURL& )) );
    connect( m_cmbPath, SIGNAL( returnPressed( const QString&  )), SLOT(cmbPathReturnPressed( const QString& )) );
    connect( m_dir, SIGNAL(viewChanged( KFileView* )), SLOT(slotViewChanged( KFileView* )) );
    connect( m_dir, SIGNAL(fileSelected( const KFileItem* )), SLOT(activateThis( const KFileItem* )) );

    setupToolbar();

    setFocusProxy( m_dir ); //so the dirOperator is focussed when we get focus events
    setMinimumWidth( m_toolbar->sizeHint().width() ); //the m_toolbar minWidth is 0!
}


FileBrowser::~FileBrowser()
{
    KConfig* const c = amaroK::config( "Filebrowser" );

    m_dir->writeConfig( c ); //uses currently set group

    c->writeEntry( "Location", m_dir->url().directory( false, false ) );
    c->writeEntry( "Dir History", m_cmbPath->urls() );
}

//END Constructor/Destructor


//BEGIN Public Methods

KURL FileBrowser::url() const
{
    return m_dir->url();
}

inline void
FileBrowser::setupToolbar()
{
    QStringList actions;
    actions << "up" << "back" << "forward" << "home" << "reload" << "short view"
            << "detailed view" << "sorting menu" << "bookmarks";

    for( QStringList::ConstIterator it = actions.constBegin(); it != actions.constEnd(); ++it )
        if ( KAction *a = m_dir->actionCollection()->action( (*it).latin1() ) )
            a->plug( m_toolbar );
}

//END Public Methods


//BEGIN Private Methods

KURL::List FileBrowser::selectedItems()
{
    KURL::List list;
    for( KFileItemListIterator it( m_dir->selectedItems()->count() ? *m_dir->selectedItems() : *m_dir->view()->items() ); *it; ++it )
        list.append( (*it)->url() );

    return list;
}

//END Private Methods


//BEGIN Public Slots

void
FileBrowser::slotSetFilter( )
{
    const QString text = m_filterEdit->text();

    if ( text.isEmpty() )
        m_dir->clearFilter();

    else {
        QString filter;
        QStringList terms = QStringList::split( ' ', text );

        for ( QStringList::Iterator it = terms.begin(); it != terms.end(); ++it ) {
            filter += '*';
            filter += *it; }

        filter += '*';
        m_dir->setNameFilter( filter );
    }

    m_dir->updateDir();
}


void
FileBrowser::setDir( const KURL &u )
{
    m_dir->setURL( u, true );
}

//END Public Slots


//BEGIN Private Slots

//NOTE I inline all these as they are private slots and only called from within the moc segment

inline void FileBrowser::cmbPathReturnPressed( const QString& u )
{
    QStringList urls = m_cmbPath->urls();
    urls.remove( u );
    urls.prepend( u );
    m_cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
    m_dir->setFocus();
    m_dir->setURL( KURL(u), true );
}


inline void FileBrowser::dirUrlEntered( const KURL& u )
{
    m_cmbPath->setURL( u );
}


inline void FileBrowser::slotSetFilterTimeout()
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->start( 180, true );
}


inline void FileBrowser::slotViewChanged( KFileView *view )
{
    if( view->widget()->inherits( "KListView" ) )
    {
        using namespace amaroK::ColorScheme;

        static_cast<KListView*>(view->widget())->setAlternateBackground( AltBase );
    }
}


inline void FileBrowser::activateThis( const KFileItem *item )
{
    Playlist::instance()->insertMedia( item->url(), Playlist::DirectPlay );
}


inline void FileBrowser::makePlaylist()
{
    Playlist::instance()->insertMedia( selectedItems(), Playlist::Replace );
}


inline void FileBrowser::addToPlaylist()
{
    Playlist::instance()->insertMedia( selectedItems() );
}


inline void FileBrowser::selectAllFiles()
{
    KFileItemList list( *m_dir->view()->items() );

    // Select all items which represent files
    for ( KFileItem* item = list.first(); item; item = list.next() )
        m_dir->view()->setSelected( item, item->isFile() );
}


inline void FileBrowser::burnDataCd() // SLOT
{
    K3bExporter::instance()->exportTracks( selectedItems(), K3bExporter::DataCD );
}


inline void FileBrowser::burnAudioCd() // SLOT
{
    K3bExporter::instance()->exportTracks( selectedItems(), K3bExporter::AudioCD );
}

//END Private Slots



#include <kurldrag.h>

class KURLView : public KListView
{
public:
    KURLView( QWidget *parent ) : KListView( parent )
    {
        reinterpret_cast<QWidget*>(header())->hide();
        addColumn( QString() );
        setResizeMode( KListView::LastColumn );
        setDragEnabled( true );
        setSelectionMode( QListView::Extended );
    }

    class Item : public KListViewItem {
    public:
        Item( const KURL &url, KURLView *parent ) : KListViewItem( parent, url.fileName() ), m_url( url ) {}
        KURL m_url;
    };

    virtual QDragObject *dragObject()
    {
        QPtrList<QListViewItem> items = selectedItems();
        KURL::List urls;

        for( Item *item = (Item*)items.first(); item; item = (Item*)items.next() )
            urls += item->m_url;

        return new KURLDrag( urls, this );
    }
};



SearchPane::SearchPane( FileBrowser *parent )
        : QVBox( parent )
        , m_lineEdit( 0 )
        , m_listView( 0 )
        , m_lister( 0 )
{
    QFrame *container = new QVBox( this, "container" );
    container->hide();

    {
        QFrame *box = new QHBox( container );
        box->setMargin( 5 );
        box->setBackgroundMode( Qt::PaletteBase );

        m_lineEdit = new ClickLineEdit( i18n("Search here..."), box );
        connect( m_lineEdit, SIGNAL(textChanged( const QString& )), SLOT(searchTextChanged( const QString& )) );

        m_listView = new KURLView( container );

        container->setFrameStyle( m_listView->frameStyle() );
        container->setMargin( 5 );
        container->setBackgroundMode( Qt::PaletteBase );

        m_listView->setFrameStyle( QFrame::NoFrame );

    }

    KPushButton *button = new KPushButton( KGuiItem( i18n("Perform Search..."), "find" ), this );
    button->setToggleButton( true );
    connect( button, SIGNAL(toggled( bool )), SLOT(toggle( bool )) );

    m_lister = new MyDirLister( true /*delay mimetypes*/ );
    insertChild( m_lister );
    connect( m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(searchMatches( const KFileItemList& )) );
    connect( m_lister, SIGNAL(completed()), SLOT(listingComplete()) );
}

void
SearchPane::toggle( bool toggled )
{
    if ( toggled )
        m_lineEdit->setFocus();

    static_cast<QWidget*>(child("container"))->setShown( toggled );
}

void
SearchPane::searchTextChanged( const QString &text )
{
    //TODO if user changes search directory then we need to update the search too

    m_lister->stop();
    m_listView->clear();
    m_dirs.clear();

    if ( text.isEmpty() )
        return;

    m_filter = QRegExp( text.contains( "*" ) ? text : '*'+text+'*', false, true );

    m_lister->openURL( searchURL() );
}

void
SearchPane::searchMatches( const KFileItemList &list )
{
    for( KFileItemList::ConstIterator it = list.begin(), end = list.end(); it != end; ++it ) {
        if( (*it)->isDir() )
            m_dirs += (*it)->url();
        else if( m_filter.exactMatch( (*it)->name() ) )
            new KURLView::Item( (*it)->url(), (KURLView*)m_listView );
    }
}

void
SearchPane::searchComplete()
{
    //KDirLister crashes if you call openURL() from a slot
    //connected to KDirLister::complete()
    //TODO fix crappy KDElibs

    QTimer::singleShot( 0, this, SLOT(_searchComplete()) );
}

void
SearchPane::_searchComplete()
{
    if ( !m_dirs.isEmpty() ) {
        KURL url = m_dirs.first();
        m_dirs.pop_front();
        m_lister->openURL( url );
    }
}

#include "filebrowser.moc"
