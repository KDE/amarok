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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "amarok.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "enginecontroller.h"
#include "filebrowser.h"
#include "k3bexporter.h"
#include <kaction.h>
#include <kapplication.h>
#include "kbookmarkhandler.h"
#include <kdiroperator.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kpushbutton.h>     ///@see SearchPane
#include <ktoolbarbutton.h>  ///@see ctor
#include <kurlcombobox.h>
#include <kurlcompletion.h>

#include "mediabrowser.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistloader.h"
#include "playlistwindow.h"

#include <qdir.h>
#include <qhbox.h>
#include <qiconview.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>


//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)


class MyDirLister : public KDirLister {
public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister( delayedMimeTypes ) {}

protected:
    virtual bool matchesMimeFilter( const KFileItem *item ) const {
        return
            item->isDir() ||
            EngineController::canDecode( item->url() ) ||
            item->url().protocol() == "audiocd" ||
            PlaylistFile::isPlaylistFile( item->name() ) ||
            item->name().endsWith( ".mp3", false ) || //for now this is less confusing for the user
            item->name().endsWith( ".ogg", false );
    }
};

class MyDirOperator : public KDirOperator {
public:
    MyDirOperator( const KURL &url, QWidget *parent ) : KDirOperator( url, parent ) {
        setDirLister( new MyDirLister( true ) );
//the delete key event was being eaten with the file browser open
//so folks couldn't use the del key to remove items from the playlist
//refer to KDirOperator::setupActions() to understand this code
        KActionCollection* dirActionCollection = static_cast<KActionCollection*>(KDirOperator::child("KDirOperator::myActionCollection"));
        if(dirActionCollection)
        {
            KAction* trash = dirActionCollection->action("trash");
            if(trash)
                trash->setEnabled(false);
        }
    }
public slots:
    //reimplemented due to a bug in KDirOperator::activatedMenu ( KDE 3.4.2 ) - See Bug #103305
    virtual void activatedMenu (const KFileItem *, const QPoint &pos) {
        updateSelectionDependentActions();
        ((KActionMenu*)actionCollection()->action("popupMenu"))->popupMenu()->popup( pos );
    }
};


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
        : QVBox( 0, name )
{
    KActionCollection *actionCollection;
    KConfig* const config = amaroK::config( "Filebrowser" );
    SearchPane *searchPane;

    KURL location( config->readPathEntry( "Location", QDir::homeDirPath() ) );
    KFileItem currentFolder( KFileItem::Unknown, KFileItem::Unknown, location );
    //KIO sucks, NetAccess::exists puts up a dialog and has annoying error message boxes
    //if there is a problem so there is no point in using it anyways.
    //so... setting the diroperator to ~ is the least sucky option
    if ( !location.isLocalFile() || !currentFolder.isReadable() ) {
        location = QDir::homeDirPath() ;
    }

    KToolBar *toolbar = new Browser::ToolBar( this );

    { //Filter LineEdit
        KToolBar* searchToolBar = new Browser::ToolBar( this );
        KToolBarButton *button = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_filter = new ClickLineEdit( i18n( "Filter here..." ), searchToolBar );

        searchToolBar->setStretchableWidget( m_filter );

        connect( button, SIGNAL(clicked()), m_filter, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_filter, i18n( "Enter space-separated terms to filter the directory-listing" ) );
    }

    { //Directory Listing
        QVBox *container; QHBox *box;

        container = new QVBox( this );
        container->setFrameStyle( m_filter->frameStyle() );
        container->setMargin( 3 );
        container->setSpacing( 2 );
        container->setBackgroundMode( Qt::PaletteBase );

        box = new QHBox( container );
        box->setMargin( 3 );
        box->setBackgroundMode( Qt::PaletteBase );

        //folder selection combo box
        m_combo = new KURLComboBox( KURLComboBox::Directories, true, box, "path combo" );
        m_combo->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
        m_combo->setAutoDeleteCompletionObject( true );
        m_combo->setMaxItems( 9 );
        m_combo->setURLs( config->readPathListEntry( "Dir History" ) );
        m_combo->lineEdit()->setText( location.path() );

        //The main widget with file listings and that
        m_dir = new MyDirOperator( location, container );
        m_dir->setEnableDirHighlighting( true );
        m_dir->setMode( KFile::Mode((int)KFile::Files | (int)KFile::Directory) ); //allow selection of multiple files + dirs
        m_dir->setOnlyDoubleClickSelectsFiles( true ); //amaroK type settings
        m_dir->readConfig( config );
        m_dir->setView( KFile::Default ); //will set userconfigured view, will load URL
        m_dir->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
        static_cast<QFrame*>(m_dir->viewWidget())->setFrameStyle( QFrame::NoFrame );
        static_cast<QIconView*>(m_dir->viewWidget())->setSpacing( 1 );

        actionCollection = m_dir->actionCollection();

        searchPane = new SearchPane( this );

        setStretchFactor( container, 2 );
    }

    {
        QPopupMenu* const menu = ((KActionMenu*)actionCollection->action("popupMenu"))->popupMenu();

        menu->clear();
        menu->insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), MakePlaylist );
        menu->insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), AppendToPlaylist );
        menu->insertItem( SmallIconSet( "filesave" ), i18n( "&Save as Playlist..." ), SavePlaylist );
        menu->insertSeparator();

        menu->insertItem( SmallIconSet( "usbpendrive_unmount" ), i18n( "Add to Media Device &Transfer Queue" ), MediaDevice );
        menu->insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD"), BurnCd );
        menu->insertSeparator();
        menu->insertItem( i18n( "&Select All Files" ), SelectAllFiles );
        menu->insertSeparator();
        actionCollection->action( "delete" )->plug( menu );
        menu->insertSeparator();
        actionCollection->action( "properties" )->plug( menu );

        menu->setItemEnabled( BurnCd, K3bExporter::isAvailable() );

        connect( menu, SIGNAL(aboutToShow()), SLOT(prepareContextMenu()) );
        connect( menu, SIGNAL(activated( int )), SLOT(contextMenuActivated( int )) );
    }

    {
        KActionMenu *a;

        a = (KActionMenu*)actionCollection->action( "sorting menu" );
        a->setIcon( "configure" );
        a->setDelayed( false ); //TODO should be done by KDirOperator

        actionCollection->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );

        a = new KActionMenu( i18n("Bookmarks"), "bookmark", actionCollection, "bookmarks" );
        a->setDelayed( false );

        new KBookmarkHandler( m_dir, a->popupMenu() );
    }

    {
        QStringList actions;
        actions << "up" << "back" << "forward" << "home" << "reload" << "short view"
                << "detailed view" << "sorting menu" << "bookmarks";

        foreach( actions )
            if ( KAction *a = actionCollection->action( (*it).latin1() ) )
                a->plug( toolbar );
    }

    connect( m_filter, SIGNAL(textChanged( const QString& )), SLOT(setFilter( const QString& )) );
    connect( m_combo, SIGNAL(urlActivated( const KURL& )), SLOT(setUrl( const KURL& )) );
    connect( m_combo, SIGNAL(returnPressed( const QString& )), SLOT(setUrl( const QString& )) );
    connect( m_dir, SIGNAL(viewChanged( KFileView* )), SLOT(slotViewChanged( KFileView* )) );
    connect( m_dir, SIGNAL(fileSelected( const KFileItem* )), SLOT(activate( const KFileItem* )) );
    connect( m_dir, SIGNAL(urlEntered( const KURL& )), SLOT(urlChanged( const KURL& )) );
    connect( m_dir, SIGNAL(urlEntered( const KURL& )), searchPane, SLOT(urlChanged( const KURL& )) );

    setSpacing( 4 );
    setFocusProxy( m_dir ); //so the dirOperator is focussed when we get focus events
    setMinimumWidth( toolbar->sizeHint().width() );
}


FileBrowser::~FileBrowser()
{
    KConfig* const c = amaroK::config( "Filebrowser" );

    m_dir->writeConfig( c ); //uses currently set group

    c->writePathEntry( "Location", m_dir->url().url() );
    c->writePathEntry( "Dir History", m_combo->urls() );
}

//END Constructor/Destructor

//BEGIN Private Methods

KURL::List FileBrowser::selectedItems()
{
    KURL::List list;
    for( KFileItemListIterator it( m_dir->selectedItems()->count() ? *m_dir->selectedItems() : *m_dir->view()->items() ); *it; ++it )
        list.append( (*it)->url() );

    return list;
}

void FileBrowser::playlistFromURLs( const KURL::List &urls )
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
        PlaylistWindow::self()->showBrowser( "PlaylistBrowser" );
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

        const QStringList terms = QStringList::split( ' ', text );
        foreach( terms ) {
            filter += '*';
            filter += *it;
        }
        filter += '*';

        m_dir->setNameFilter( filter );
    }

    m_dir->updateDir();
}

//END Public Slots


//BEGIN Private Slots

inline void
FileBrowser::urlChanged( const KURL &u )
{
    //the DirOperator's URL has changed

    const QString url = u.url();

    QStringList urls = m_combo->urls();
    urls.remove( url );
    urls.prepend( url );
    m_combo->setURLs( urls, KURLComboBox::RemoveBottom );
}

inline void
FileBrowser::slotViewChanged( KFileView *view )
{
    if( view->widget()->inherits( "KListView" ) )
    {
        using namespace amaroK::ColorScheme;

        static_cast<KListView*>(view->widget())->setAlternateBackground( AltBase );
    }
}

inline void
FileBrowser::activate( const KFileItem *item )
{
    Playlist::instance()->insertMedia( item->url(), Playlist::DirectPlay );
}

inline void
FileBrowser::prepareContextMenu()
{
    const KFileItemList &items = *m_dir->selectedItems();
    ((KActionMenu*)m_dir->actionCollection()->action("popupMenu"))->popupMenu()->setItemVisible( SavePlaylist,
        items.count() > 1 || ( items.count() == 1 && items.getFirst()->isDir() ) );
    ((KActionMenu*)m_dir->actionCollection()->action("popupMenu"))->popupMenu()->setItemVisible( MediaDevice,
        MediaDevice::instance()->isConnected() );
}

inline void
FileBrowser::contextMenuActivated( int id )
{
    switch( id )
    {
    case MakePlaylist:
        Playlist::instance()->insertMedia( selectedItems(), Playlist::Replace );
        break;

    case SavePlaylist:
        playlistFromURLs( selectedItems() );
        break;

    case AppendToPlaylist:
        Playlist::instance()->insertMedia( selectedItems() );
        break;

    case MediaDevice:
        MediaDevice::instance()->addURLs( selectedItems() );
        break;

    case SelectAllFiles:
    {
        KFileItemList list( *m_dir->view()->items() );

        // Select all items which represent files
        for( KFileItem* item = list.first(); item; item = list.next() )
            m_dir->view()->setSelected( item, item->isFile() );

        break;
    }

    case BurnCd:
        K3bExporter::instance()->exportTracks( selectedItems() );
        break;
    }
}

//END Private Slots



#include <kurldrag.h>
#include <qpainter.h>
#include <qsimplerichtext.h>

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

    virtual void viewportPaintEvent( QPaintEvent *e )
    {
        KListView::viewportPaintEvent( e );

        if ( childCount() == 0 ) {
            QPainter p( viewport() );

            if ( m_text.isEmpty() ) {
                //TODO Perhaps it's time to put this in some header, as we use it in three places now
                QSimpleRichText t( i18n(
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
                p.drawText( rect(), Qt::AlignCenter | Qt::WordBreak, m_text );
            }
        }
    }

    void unsetText() { setText( QString::null ); }
    void setText( const QString &text ) { m_text = text; viewport()->update(); }

private:
    QString m_text;
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
        connect( m_listView, SIGNAL(executed( QListViewItem* )), SLOT(activate( QListViewItem* )) );
    }

    KPushButton *button = new KPushButton( KGuiItem( i18n("&Show Search Panel"), "find" ), this );
    button->setToggleButton( true );
    connect( button, SIGNAL(toggled( bool )), SLOT(toggle( bool )) );

    m_lister = new MyDirLister( true /*delay mimetypes*/ );
    insertChild( m_lister );
    connect( m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(searchMatches( const KFileItemList& )) );
    connect( m_lister, SIGNAL(completed()), SLOT(searchComplete()) );
}

void
SearchPane::toggle( bool toggled )
{
    if ( toggled )
        m_lineEdit->setFocus();

    static_cast<QWidget*>(child("container"))->setShown( toggled );
}

void
SearchPane::urlChanged( const KURL& )
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

    m_lister->openURL( searchURL() );

    m_listView->setText( i18n( "Searching..." ) );
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
    else
        m_listView->setText( i18n("No results found") ); //only displayed if the listview is empty
}

void
SearchPane::activate( QListViewItem *item )
{
    Playlist::instance()->insertMedia( static_cast<KURLView::Item*>(item)->m_url, Playlist::DirectPlay );
}

#include "filebrowser.moc"
