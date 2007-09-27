// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 GÁbor Lehel <illissius@gmail.com>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2005 Christan Baumgart <christianbaumgart@web.de>
// (c) 2006 Joe Rabinoff <bobqwatson@yahoo.com>
// See COPYING file for licensing information.

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "browserbar.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "covermanager.h"
#include "debug.h"
#include "deletedialog.h"
#include "directorylist.h"
#include "editfilterdialog.h"
#include "k3bexporter.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "organizecollectiondialog.h"
#include "playlist.h"       //insertMedia()
#include "playlistbrowser.h"
#include "starmanager.h"
#include "statusbar.h"
#include "tagdialog.h"
#include "threadmanager.h"
#include "qstringx.h"

#include <taglib/tfile.h>   //TagLib::File::isWritable

#include <unistd.h>         //CollectionView ctor

#include <qapplication.h>
#include <qcstring.h>
#include <qdragobject.h>
#include <qlayout.h>        //infobox
#include <qmap.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qptrlist.h>
#include <qpushbutton.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()
#include <qheader.h>
#include <qregexp.h>

#include <kactioncollection.h>
#include <kapplication.h>   //kapp
#include <kconfig.h>
#include <kcombobox.h>
#include <kcursor.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <kiconloader.h>    //renderView()
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()
#include <kio/job.h>
#include <kpushbutton.h>

extern "C"
{
    #if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    #include <X11/Xlib.h>    //ControlMask in contentsDragMoveEvent()
    #endif
}

using namespace CollectionBrowserIds;

namespace Amarok { extern KConfig *config( const QString& ); }

class CoverFetcher;

CollectionBrowser *CollectionBrowser::s_instance = 0;

CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
    , m_cat1Menu( new KPopupMenu( this ) )
    , m_cat2Menu( new KPopupMenu( this ) )
    , m_cat3Menu( new KPopupMenu( this ) )
    , m_timer( new QTimer( this ) )
    , m_returnPressed( false )
{
    s_instance = this;

    setSpacing( 4 );

    m_toolbar = new Browser::ToolBar( this );

    { //<Search LineEdit>
        KToolBarButton *button;
        KToolBar* searchToolBar = new Browser::ToolBar( this );

        button       = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Enter search terms here" ), searchToolBar );
        m_searchEdit->installEventFilter( this );
        KPushButton *filterButton = new KPushButton("...", searchToolBar, "filter");
        searchToolBar->setStretchableWidget( m_searchEdit );

        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL( clicked() ), SLOT( slotClearFilter() ) );
        connect( filterButton, SIGNAL( clicked() ), SLOT( slotEditFilter() ) );

        QToolTip::add( button, i18n( "Clear search field" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to search in the collection" ) );
        QToolTip::add( filterButton, i18n( "Click to edit collection filter" ) );
    } //</Search LineEdit>


    // We put a little toolbar for the forward/back buttons for iPod
    // navigation to the right of m_timeFilter.  This toolbar is
    // hidden when not in iPod browsing mode; it is shown and hidden
    // in CollectionView::setViewMode().  m_ipodHbox holds m_timeFilter
    // and m_ipodToolbar
    m_ipodHbox = new QHBox( this );
    m_ipodHbox->setSpacing( 7 );  // looks better

    m_timeFilter = new KComboBox( m_ipodHbox, "timeFilter" );
    m_ipodHbox->setStretchFactor( m_timeFilter, 1 );
    // Allow the combobox to shrink so the iPod buttons are still visible
    m_timeFilter->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_timeFilter->insertItem( i18n( "Entire Collection" ) );
    m_timeFilter->insertItem( i18n( "Added Today" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Week" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Month" ) );
    m_timeFilter->insertItem( i18n( "Added Within Three Months" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Year" ) );


    // m_ipodToolbar just holds the forward and back buttons, which are
    // plugged below
    m_ipodToolbar = new Browser::ToolBar( m_ipodHbox );
    m_ipodHbox->setStretchFactor( m_ipodToolbar, 0 );
    m_ipodToolbar->setIconText( KToolBar::IconOnly, false );


    KActionCollection* ac = new KActionCollection( this );

    m_view = new CollectionView( this );
    m_view->installEventFilter( this );

    m_configureAction = new KAction( i18n( "Configure Folders" ), Amarok::icon( "configure" ), 0, this, SLOT( setupDirs() ), ac, "Configure" );
    m_treeViewAction = new KRadioAction( i18n( "Tree View" ), "view_tree", 0, m_view, SLOT( setTreeMode() ), ac, "Tree View" );
    m_flatViewAction = new KRadioAction( i18n( "Flat View" ), "view_detailed", 0, m_view, SLOT( setFlatMode() ), ac, "Flat View" );
    m_ipodViewAction = new KRadioAction( i18n( "iPod View" ), Amarok::icon("device"), 0, m_view, SLOT( setIpodMode() ), ac, "iPod View" );
    m_treeViewAction->setExclusiveGroup("view mode");
    m_flatViewAction->setExclusiveGroup("view mode");
    m_ipodViewAction->setExclusiveGroup("view mode");
    switch( m_view->m_viewMode )
      {
      case CollectionView::modeTreeView:
        m_treeViewAction->setChecked( true );
        break;
      case CollectionView::modeFlatView:
        m_flatViewAction->setChecked( true );
        break;
      case CollectionView::modeIpodView:
        m_ipodViewAction->setChecked( true );
        break;
      }

    m_showDividerAction = new KToggleAction( i18n( "Show Divider" ), "leftjust", 0, this, SLOT( toggleDivider() ), ac, "Show Divider" );
    m_showDividerAction->setChecked(m_view->m_showDivider);


    // m_ipodIncrement and m_ipodDecrement are the actions that
    // correspond to moving forward / backward in the iPod collection
    // browser window; see the "For iPod-style navigation" comments below.
    m_ipodDecrement = new KAction( i18n( "Browse backward" ),
                                   QIconSet( m_view->ipodDecrementIcon(), QIconSet::Small ),
                                   0, m_view, SLOT( decrementDepth() ), ac,
                                   "iPod Decrement" );
    m_ipodIncrement = new KAction( i18n( "Browse forward" ),
                                   QIconSet( m_view->ipodIncrementIcon(), QIconSet::Small ),
                                   0, m_view, SLOT( incrementDepth() ), ac,
                                   "iPod Increment" );
    m_ipodDecrement->plug( m_ipodToolbar );
    m_ipodIncrement->plug( m_ipodToolbar );

    // Show / hide m_ipodToolbar based on the view mode
    ipodToolbar( m_view->m_viewMode == CollectionView::modeIpodView );


    m_tagfilterMenuButton = new KActionMenu( i18n( "Group By" ), "filter", ac );
    m_tagfilterMenuButton->setDelayed( false );
    // FIXME: either both or nothing
    //m_tagfilterMenuButton->setEnabled( m_view->m_viewMode == CollectionView::modeTreeView );
    //connect ( m_treeViewAction, SIGNAL ( toggled(bool) ), m_tagfilterMenuButton, SLOT( setEnabled (bool) ) );

    layoutToolbar();

    m_categoryMenu = m_tagfilterMenuButton->popupMenu();
    m_categoryMenu->insertItem( i18n( "Artist" ), m_view, SLOT( presetMenu( int ) ), 0, IdArtist );
    m_categoryMenu->insertItem( i18n( "Artist / Album" ), m_view, SLOT( presetMenu( int ) ), 0, IdArtistAlbum );
    m_categoryMenu->insertItem( i18n( "Artist" )+" / "+ i18n( "Year" ) + i18n( " - " ) + i18n( "Album" ), m_view, SLOT( presetMenu( int ) ), 0, IdArtistVisYearAlbum );
    m_categoryMenu->insertItem( i18n( "Album" ), m_view, SLOT( presetMenu( int ) ), 0, IdAlbum );
    m_categoryMenu->insertItem( i18n( "Genre / Artist" ), m_view, SLOT( presetMenu( int ) ), 0, IdGenreArtist );
    m_categoryMenu->insertItem( i18n( "Genre / Artist / Album" ), m_view, SLOT( presetMenu( int ) ), 0, IdGenreArtistAlbum );

    m_categoryMenu->insertSeparator();

    m_categoryMenu->insertItem( i18n( "&First Level" ), m_cat1Menu );
    m_categoryMenu->insertItem( i18n( "&Second Level"), m_cat2Menu );
    m_categoryMenu->insertItem( i18n( "&Third Level" ), m_cat3Menu );

    m_cat1Menu ->insertItem( i18n( "&Album" ), m_view, SLOT( cat1Menu( int ) ), 0, IdAlbum );
    m_cat1Menu ->insertItem( i18n( "(Y&ear) - Album" ), m_view, SLOT( cat1Menu( int ) ), 0, IdVisYearAlbum);
    m_cat1Menu ->insertItem( i18n( "A&rtist"), m_view, SLOT( cat1Menu( int ) ), 0, IdArtist );
    m_cat1Menu ->insertItem( i18n( "&Composer"), m_view, SLOT( cat1Menu( int ) ), 0, IdComposer );
    m_cat1Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat1Menu( int ) ), 0, IdYear );
    m_cat1Menu ->insertItem( i18n( "&Label" ), m_view, SLOT( cat1Menu( int ) ), 0, IdLabel );

    m_cat2Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( i18n( "&Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( i18n( "(Y&ear) - Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdVisYearAlbum);
    m_cat2Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( i18n( "&Composer"), m_view, SLOT( cat2Menu( int ) ), 0, IdComposer );
    m_cat2Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat2Menu( int ) ), 0, IdYear );
    m_cat2Menu ->insertItem( i18n( "&Label" ), m_view, SLOT( cat2Menu( int ) ), 0, IdLabel );

    m_cat3Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat3Menu( int ) ), 0, IdNone );
    m_cat3Menu ->insertSeparator();
    m_cat3Menu ->insertItem( i18n( "A&lbum" ), m_view, SLOT( cat3Menu( int ) ), 0, IdAlbum );
    m_cat3Menu ->insertItem( i18n( "(Y&ear) - Album" ), m_view, SLOT( cat3Menu( int ) ), 0, IdVisYearAlbum);
    m_cat3Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat3Menu( int ) ), 0, IdArtist );
    m_cat3Menu ->insertItem( i18n( "&Composer"), m_view, SLOT( cat3Menu( int ) ), 0, IdComposer );
    m_cat3Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat3Menu( int ) ), 0, IdGenre );
    m_cat3Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat3Menu( int ) ), 0, IdYear );
    m_cat3Menu ->insertItem( i18n( "&Label" ), m_view, SLOT( cat3Menu( int ) ), 0, IdLabel );

    m_view->cat1Menu( m_view->m_cat1, false );
    m_view->cat2Menu( m_view->m_cat2, false );
    m_view->cat3Menu( m_view->m_cat3, false );
    m_view->setViewMode( m_view->m_viewMode );

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_timeFilter, SIGNAL( activated( int ) ), SLOT( slotSetFilter() ) );

    setFocusProxy( m_view ); //default object to get focus
}

void
CollectionBrowser::slotClearFilter() //SLOT
{
    m_searchEdit->clear();
    kapp->processEvents();  //Let the search bar redraw fully.
    QTimer::singleShot( 0, this, SLOT( slotSetFilter() ) ); //Filter instantly
    QTimer::singleShot( 0, m_view, SLOT( slotEnsureSelectedItemVisible() ) );
}

void
CollectionBrowser::slotSetFilterTimeout() //SLOT
{
    m_returnPressed = false;
    m_timer->start( 280, true ); //stops the timer for us first
}

void
CollectionBrowser::slotSetFilter() //SLOT
{
    m_timer->stop();
    m_view->m_dirty = true;
    m_view->setFilter( m_searchEdit->text() );
    m_view->setTimeFilter( m_timeFilter->currentItem() );
    m_view->renderView();
    if ( m_returnPressed )
        appendSearchResults();
    m_returnPressed = false;
}

void
CollectionBrowser::slotSetFilter( const QString &filter ) //SLOT
{
    m_searchEdit->setText( filter );
    kapp->processEvents();  //Let the search bar redraw fully.
    QTimer::singleShot( 0, this, SLOT( slotSetFilter() ) ); //Filter instantly
    QTimer::singleShot( 0, m_view, SLOT( slotEnsureSelectedItemVisible() ) );
}

void
CollectionBrowser::slotEditFilter() //SLOT
{
    EditFilterDialog *cod = new EditFilterDialog( this, false, m_searchEdit->text() );
    connect( cod, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
    if( cod->exec() )
        m_searchEdit->setText( cod->filter() );
    delete cod;
}

void
CollectionBrowser::setupDirs()  //SLOT
{
    m_view->setupDirs();
}

void
CollectionBrowser::toggleDivider() //SLOT
{
    m_view->setShowDivider( m_showDividerAction->isChecked() );
}

void
CollectionBrowser::appendSearchResults()
{
    //If we are not filtering, or the search string has changed recently, do nothing
    if ( m_searchEdit->text().stripWhiteSpace().isEmpty() || m_timer->isActive() )
        return;
    m_view->selectAll();
    Playlist::instance()->insertMedia( m_view->listSelected(), Playlist::Unique | Playlist::Append );
    m_view->clearSelection();
    slotClearFilter();
}

bool
CollectionBrowser::eventFilter( QObject *o, QEvent *e )
{
    switch( e->type() )
    {
    case 6/*QEvent::KeyPress*/:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        if( o == m_searchEdit ) //the search lineedit
        {
            switch( e->key() )
            {
            case Key_Up:
            case Key_Down:
            case Key_PageDown:
            case Key_PageUp:
                m_view->setFocus();
                QApplication::sendEvent( m_view, e );
                return true;

            case Key_Escape:
                slotClearFilter();
                return true;

            case Key_Return:
            case Key_Enter:
                if ( m_timer->isActive() )
                {
                    //Immediately filter and add results
                    m_timer->stop();
                    m_returnPressed = true;
                    QTimer::singleShot( 0, this, SLOT( slotSetFilter() ) );
                }
                else
                {
                    //Add current results
                    appendSearchResults();
                }
                return true;

            default:
                return false;
            }
        }

        // (Joe Rabinoff) the code that was here which dealt with wrapping
        // the selection around when Key_Up or Key_Down was pressed was
        // moved to CollectionView::keyPressEvent().  That code also
        // skips dividers.

        if( ( e->key() >= Key_0 && e->key() <= Key_Z ) || e->key() == Key_Backspace || e->key() == Key_Escape )
        {
            m_searchEdit->setFocus();
            QApplication::sendEvent( m_searchEdit, e );
            return true;
        }
        #undef e
        break;

    default:
        break;
    }

    return QVBox::eventFilter( o, e );
}

void
CollectionBrowser::layoutToolbar()
{
    if ( !m_toolbar ) return;

    m_toolbar->clear();

    m_toolbar->setIconText( KToolBar::IconTextRight, false );
    m_tagfilterMenuButton->plug( m_toolbar );
    m_toolbar->setIconText( KToolBar::IconOnly, false );

    m_toolbar->insertLineSeparator();
    m_treeViewAction->plug( m_toolbar );
    m_flatViewAction->plug( m_toolbar );
    m_ipodViewAction->plug( m_toolbar );
    m_toolbar->insertLineSeparator();

    m_showDividerAction->plug( m_toolbar );
    m_configureAction->plug( m_toolbar );

    //This would break things if the toolbar is too big, see bug #121915
    //setMinimumWidth( m_toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
}


// (De)activate the iPod toolbar when switching into and out of
// iPod browsing mode
void
CollectionBrowser::ipodToolbar( bool activate )
{
    if( activate )
        m_ipodToolbar->show();

    else
        m_ipodToolbar->hide();
}



//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView* CollectionView::m_instance = 0;


CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
        , m_timeFilter( 0 )
        , m_currentDepth( 0 )
        , m_ipodIncremented ( 1 )
        , m_dirty( true )
        , m_organizingFileCancelled( false )
{
    DEBUG_FUNC_INFO
    m_instance = this;

    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setSorting( 0 );
    setShowSortIndicator( true );
    setAcceptDrops( true );
    setAllColumnsShowFocus( true );

    //<READ CONFIG>
        KConfig* config = Amarok::config( "Collection Browser" );
        m_cat1 = config->readNumEntry( "Category1", IdArtist );
        m_cat2 = config->readNumEntry( "Category2", IdAlbum );
        m_cat3 = config->readNumEntry( "Category3", IdNone );

#define saneCat(x) (x==IdAlbum||x==IdArtist||x==IdComposer||x==IdGenre||x==IdYear \
        ||x==IdNone \
        ||x==IdArtistAlbum||x==IdGenreArtist||x==IdGenreArtistAlbum||x==IdVisYearAlbum||x==IdArtistVisYearAlbum)

        if( !saneCat(m_cat1) )
        {
            m_cat1 = IdArtist;
            m_cat2 = IdAlbum;
            m_cat2 = IdNone;
        }
        if( !saneCat(m_cat2) || !saneCat(m_cat3) )
        {
            m_cat2 = m_cat3 = IdNone;
        }
#undef saneCat

        m_viewMode = config->readNumEntry( "ViewMode", modeTreeView );
        m_showDivider = config->readBoolEntry( "ShowDivider", true);
        updateTrackDepth();

        m_flatColumnWidths.clear();
        QStringList flatWidths = config->readListEntry( "FlatColumnWidths" );
        for( QStringList::iterator it = flatWidths.begin();
                it != flatWidths.end();
                it++ )
            m_flatColumnWidths.push_back( (*it).toInt() );

    //</READ CONFIG>
     KActionCollection* ac = new KActionCollection( this );
     KStdAction::selectAll( this, SLOT( selectAll() ), ac, "collectionview_select_all" );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ),
             this,                      SLOT( scanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ),
             this,                      SLOT( scanDone( bool ) ) );
    connect( BrowserBar::instance(),   SIGNAL( browserActivated( int ) ),
             this,                      SLOT( renderView() ) ); // renderView() checks if current tab is this
    connect( CollectionDB::instance(), SIGNAL( ratingChanged( const QString&, int ) ),
             this, SLOT( ratingChanged( const QString&, int ) ) );

    connect( this,           SIGNAL( expanded( QListViewItem* ) ),
             this,             SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,           SIGNAL( collapsed( QListViewItem* ) ),
             this,             SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,           SIGNAL( returnPressed( QListViewItem* ) ),
             this,             SLOT( invokeItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( invokeItem( QListViewItem*, const QPoint&, int ) ) );
    connect( this,           SIGNAL( clicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( ipodItemClicked( QListViewItem*, const QPoint&, int ) ) );
    connect( this,           SIGNAL( contextMenuRequested( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );
    connect( header(),       SIGNAL( sizeChange( int, int, int ) ),
             this,             SLOT( triggerUpdate() ) );

    connect( MountPointManager::instance(), SIGNAL( mediumConnected( int ) ),
             this,                            SLOT( databaseChanged() ) );
    connect( MountPointManager::instance(), SIGNAL( mediumRemoved( int ) ),
             this,                            SLOT( databaseChanged() ) );
}


CollectionView::~CollectionView() {
    DEBUG_FUNC_INFO

    KConfig* const config = Amarok::config( "Collection Browser" );
    config->writeEntry( "Category1", m_cat1 );
    config->writeEntry( "Category2", m_cat2 );
    config->writeEntry( "Category3", m_cat3 );
    config->writeEntry( "ViewMode", m_viewMode );
    config->writeEntry( "ShowDivider", m_showDivider );

    QStringList flatWidths;
    for( QValueList<int>::iterator it = m_flatColumnWidths.begin();
            it != m_flatColumnWidths.end();
            it++ )
            flatWidths.push_back( QString::number( (*it) ) );
    config->writeEntry( "FlatColumnWidths", flatWidths );
}

void
CollectionView::setShowDivider( bool show )
{
    if (show != m_showDivider) {
        m_showDivider = show;
        renderView(true);
    }
}


// Reimplemented for iPod-style navigation, and to skip dividers
// Specifically, this method traps the Key_Up/Down/Left/Right events.
// When Up or Down is pressed, it skips dividers and wraps around when
// necessary.  When Left or Right is pressed and we are viewing in
// iPod mode, the iPod "move forward / backward" actions are activated.
void
CollectionView::keyPressEvent( QKeyEvent *e )
{
    typedef QListViewItemIterator It;

    // Reimplement up and down to skip dividers and to loop around.
    // Some of this code used to be in CollectionBrowser::eventFilter.
    // This rewritten code is more faithful to the ordinary moving
    // behavior, even when looping around.  (For instance, it behaves
    // correctly if control-up is pressed at the top of the screen.)
    // It sends fake keypress events to the parent instead of programatically
    // selecting items.
    if( (e->key() == Key_Up  ||  e->key() == Key_Down )  && currentItem() )
    {
        // Handle both up and down at once to avoid code duplication (it's
        // a delicate piece of logic, and was hard to get right)

        QListViewItem *cur = currentItem();

        #define nextItem (e->key() == Key_Up ? cur->itemAbove() : cur->itemBelow())

        bool wraparound = true;

        // First skip any dividers directly above / below
        do
        {
            KListView::keyPressEvent( e );
            if( currentItem() == cur ) // Prevent infinite loops
            {
                if( nextItem != 0 )
                    wraparound = false;
                break;
            }
            cur = currentItem();

            if( cur && dynamic_cast<DividerItem*>( cur ) == 0 )
                wraparound = false;  // Found an item above / below

        } while( cur != NULL
                 && dynamic_cast<DividerItem*>(cur) != 0
                 && nextItem != 0 );

        if( cur == 0 ) return;  // Shouldn't happen

        // Wrap around if necessary, by sending a Key_Home/Key_End event.
        if( wraparound )
        {
            QKeyEvent e2 ( e->type(),
                    (e->key() == Key_Up ? Key_End : Key_Home),
                    0, e->state(),
                    QString::null, e->isAutoRepeat(), e->count() );
            QApplication::sendEvent( this, &e2 );
            cur = currentItem();

            // The first item may also be a divider, so keep moving
            // until it's not
            while ( cur != 0
                    && dynamic_cast<DividerItem*>(cur) != 0
                    && nextItem != 0 )
            {
                KListView::keyPressEvent( e );
                if( currentItem() == cur ) // Prevent infinite loops
                    break;
                cur = currentItem();
            }
        }

      #undef nextItem

    }

    // When Right/Left is pressed in iPod view mode, activate the iPod
    // "move forward/backward" action.
    else if( (e->key() == Key_Left  ||  e->key() == Key_Right)
           && m_viewMode == modeIpodView )
    {
        if( e->key() == Key_Right )
            m_parent->m_ipodIncrement->activate();

        else if( e->key() == Key_Left )
            m_parent->m_ipodDecrement->activate();

    }

    else // we don't want the event
        KListView::keyPressEvent( e );
}


//////////////////////////////////////////////////////////////////////////////////////////
// public slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::renderView(bool force /* = false */)  //SLOT
{
    SHOULD_BE_GUI
    if(!force && !m_dirty )
        return;

    if( BrowserBar::instance()->currentBrowser() != m_parent )
    {
        // the collectionbrowser is intensive for sql, so we only renderView() if the tab
        // is currently active.  else, wait until user focuses it.
//         debug() << "current browser is not collection, aborting renderView()" << endl;
        m_dirty = true;
        return;
    }
    m_dirty = false;

    // Don't cache / restore view if we're in ipod mode and we've
    // just incremented or decremented, since we'll run selectIpodItems()
    // below anyway.
    if( childCount()  &&
        !(m_viewMode == modeIpodView && m_ipodIncremented > 0) )
        cacheView();

    //clear();
    safeClear();

    if ( m_viewMode == modeFlatView )
    {
        renderFlatModeView( force );
    }

    if( m_viewMode == modeIpodView )
    {
        renderIpodModeView( force );
    }

    if( m_viewMode == modeTreeView )
    {
        renderTreeModeView( force );
    }

    // Don't cache or restore view when we're just going to run
    // selectIpodItems() below anyway.
    if( !(m_viewMode == modeIpodView && m_ipodIncremented > 0) )
        restoreView();

    else
        selectIpodItems();
}


//////////////////////////////////////////////////////////////////////////////////////////
// private slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs()  //SLOT
{
    KDialogBase dialog( this, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Configure Collection") ) );

    CollectionSetup *setup = new CollectionSetup( &dialog );
    dialog.setMainWidget( setup );
    dialog.showButtonApply( false );
    dialog.adjustSize();
    // Make the dialog a bit bigger, default is too small to be useful
    dialog.resize( dialog.width() + 50, dialog.height() + 150 );

    if ( dialog.exec() != QDialog::Rejected )
    {
        const bool rescan = ( MountPointManager::instance()->collectionFolders() != setup->dirs() );
        setup->writeConfig();

        if ( rescan )
            CollectionDB::instance()->startScan();
    }
}


void
CollectionView::scanStarted() // SLOT
{
    Amarok::actionCollection()->action("update_collection")->setEnabled( false );
}


void
CollectionView::scanDone( bool changed ) //SLOT
{
    if( changed )
    {
        renderView(true);
    }

    Amarok::actionCollection()->action("update_collection")->setEnabled( true );
}

void
CollectionView::slotEnsureSelectedItemVisible() //SLOT
{
    //Scroll to make sure the first selected item is visible

    //Find the first selected item
    QListViewItem *r=0;
    for ( QListViewItem *i = firstChild(); i && !r; i=i->nextSibling() )
    {
        if ( i->isSelected() )
            r = i;
        for ( QListViewItem *j = i->firstChild(); j && !r; j=j->nextSibling() )
        {
            if ( j->isSelected() )
                r = j;
            for ( QListViewItem *k = j->firstChild(); k && !r; k=k->nextSibling() )
            {
                if ( k->isSelected() )
                    r = k;
            }
        }
    }
    if ( r )
    {
        //We've found the selected item. Now let's refocus on it.
        //An elaborate agorithm to try to make as much as possible of the vicinity visible

        //It looks better if things end up consistently in one place.
        //So, scroll to the end so that we come at items from the bottom.
        if ( lastChild() )
            ensureItemVisible( lastChild() );

        //Create a reverse list of parents, grandparents etc.
        //Later we try to make the grandparents in view, then their children etc.
        //This means that the selected item has the most priority as it is done last.
        QValueStack<QListViewItem*> parents;
        while ( r )
        {
            parents.push( r );
            r = r->parent();
        }
        while ( !parents.isEmpty() )
        {
            //We would prefer the next item to be visible.
            if ( parents.top()->nextSibling() )
                ensureItemVisible( parents.top()->nextSibling() );
            //It's even more important the actual item is visible than the next one.
            ensureItemVisible( parents.top() );
            parents.pop();
        }
    }
}

void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    if ( !item || !item->isExpandable() ) return;

    int category = 0;
    QStringList values;

    QueryBuilder qb;
    bool c = false;
    bool SortbyTrackFirst = false;

    //Sort by track number first if album is in one of the categories, otherwise by track name first
    if ( m_cat1 == IdAlbum ||
         m_cat2 == IdAlbum ||
         m_cat3 == IdAlbum )
            SortbyTrackFirst = true;

    // initialization for year - album mode
    QString tmptext;
    int VisYearAlbum = -1;
    int VisLabel = -1;
    int q_cat1=m_cat1;
    int q_cat2=m_cat2;
    int q_cat3=m_cat3;
    if( m_cat1 == IdVisYearAlbum ||
        m_cat2 == IdVisYearAlbum ||
        m_cat3 == IdVisYearAlbum )
    {
        SortbyTrackFirst = true;
        if( m_cat1 == IdVisYearAlbum )
        {
            VisYearAlbum = 1;
            q_cat1 = IdAlbum;
        }
        if( m_cat2 == IdVisYearAlbum )
        {
            VisYearAlbum = 2;
            q_cat2 = IdAlbum;
        }
        if( m_cat3 == IdVisYearAlbum )
        {
            VisYearAlbum = 3;
            q_cat3 = IdAlbum;
        }
    }
    if( m_cat1 == IdLabel ||
        m_cat2 == IdLabel ||
        m_cat3 == IdLabel )
    {
        if( m_cat1 == IdLabel )
            VisLabel = 1;
        if( m_cat2 == IdLabel )
            VisLabel = 2;
        if ( m_cat3 == IdLabel )
            VisLabel = 3;
    }

    if ( translateTimeFilter( timeFilter() ) > 0 )
        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

    QString itemText;
    bool isUnknown;

    if ( dynamic_cast<CollectionItem*>( item ) )
    {
        itemText = static_cast<CollectionItem*>( item )->getSQLText( 0 );
    }
    else
    {
        debug() << "slotExpand in CollectionView of a non-CollectionItem" << endl;
        itemText = item->text( 0 );
    }

    switch ( item->depth() )
    {
        case 0:
            tmptext = itemText;
            isUnknown = tmptext.isEmpty();
            if ( !static_cast<CollectionItem*>( item )->isSampler() )
            {
                if ( m_cat1 == IdArtist )
                    qb.setOptions( QueryBuilder::optNoCompilations );
                if( VisYearAlbum == 1 )
                {
                    tmptext = item->text( 0 );
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year, false, true );
                    if ( isUnknown )
                        tmptext = "";
                }

                qb.addMatch( q_cat1, tmptext, false, true );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            if ( m_cat2 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( q_cat2, QueryBuilder::valTitle, true );
                qb.addReturnValue( q_cat2, QueryBuilder::valURL );
                if ( c ) qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
                if ( SortbyTrackFirst ) {
                    qb.sortBy( q_cat2, QueryBuilder::valDiscNumber );
                    qb.sortBy( q_cat2, QueryBuilder::valTrack );
                }
                if ( c ) qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
                qb.sortBy( q_cat2, QueryBuilder::valTitle );
                if ( !SortbyTrackFirst ) {
                    qb.sortBy( q_cat2, QueryBuilder::valDiscNumber );
                    qb.sortBy( q_cat2, QueryBuilder::valTrack );
                }
                qb.sortBy( q_cat2, QueryBuilder::valURL );
            }
            else
            {
                c = false;
                qb.addReturnValue( q_cat2, QueryBuilder::valName, true );
                if( VisYearAlbum == 2 )
                {
                    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName, true );
                    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
                }
                qb.sortBy( q_cat2, QueryBuilder::valName );
            }

            category = m_cat2;
            break;

        case 1:
            tmptext = dynamic_cast<CollectionItem*>( item->parent() ) ?
                static_cast<CollectionItem*>( item->parent() )->getSQLText( 0 ) :
                item->parent()->text( 0 );
            isUnknown = tmptext.isEmpty();

            if( !static_cast<CollectionItem*>( item->parent() )->isSampler() )
            {
                if ( m_cat1 == IdArtist )
                    qb.setOptions( QueryBuilder::optNoCompilations );
                if( VisYearAlbum == 1 )
                {
                    tmptext = item->parent()->text( 0 );
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year, false, true );
                    if ( isUnknown )
                        tmptext = "";
                }

                qb.addMatch( q_cat1, tmptext, false, true );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            tmptext = itemText;
            isUnknown = tmptext.isEmpty();

            if( VisYearAlbum == 2 )
            {
                tmptext = item->text( 0 );
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year, false, true );
                if ( isUnknown )
                    tmptext = "";
            }

            qb.addMatch( q_cat2, tmptext, false, true );

            if( m_cat3 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( q_cat3, QueryBuilder::valTitle, true );
                qb.addReturnValue( q_cat3, QueryBuilder::valURL );
                if ( c ) qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
                if ( SortbyTrackFirst ) {
                    qb.sortBy( q_cat3, QueryBuilder::valDiscNumber );
                    qb.sortBy( q_cat3, QueryBuilder::valTrack );
                }
                if ( c ) qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
                qb.sortBy( q_cat3, QueryBuilder::valTitle );
                if ( !SortbyTrackFirst ) {
                    qb.sortBy( q_cat3, QueryBuilder::valDiscNumber );
                    qb.sortBy( q_cat3, QueryBuilder::valTrack );
                }
                qb.sortBy( q_cat3, QueryBuilder::valURL );
            }
            else
            {
                c = false;
                qb.addReturnValue( q_cat3, QueryBuilder::valName, true );
                if( VisYearAlbum == 3 )
                {
                    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
                    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
                }
                qb.sortBy( q_cat3, QueryBuilder::valName );
            }

            category = m_cat3;
            break;

        case 2:
            tmptext = dynamic_cast<CollectionItem*> ( item->parent()->parent() ) ?
                static_cast<CollectionItem*>( item->parent()->parent() )->getSQLText( 0 ) :
                item->parent()->parent()->text( 0 );
            isUnknown = tmptext.isEmpty();

            if ( !static_cast<CollectionItem*>( item->parent()->parent() )->isSampler() )
            {
                if ( m_cat1 == IdArtist )
                    qb.setOptions( QueryBuilder::optNoCompilations );
                if( VisYearAlbum == 1 )
                {
                    tmptext = item->parent()->parent()->text( 0 );
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year, false, true );
                    if ( isUnknown )
                        tmptext = "";
                }

                qb.addMatch( q_cat1, tmptext, false, true );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            tmptext = dynamic_cast<CollectionItem*>( item->parent() ) ?
                static_cast<CollectionItem*>( item->parent() )->getSQLText( 0 ) :
                item->parent()->text( 0 );
            isUnknown = tmptext.isEmpty();

            if( VisYearAlbum == 2 )
            {
                tmptext = item->parent()->text( 0 );
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year, false, true );
                if ( isUnknown )
                    tmptext = "";
            }

            qb.addMatch( q_cat2, tmptext, false, true );

            tmptext = itemText;
            isUnknown = tmptext.isEmpty();

            if( VisYearAlbum == 3 )
            {
                tmptext = item->text( 0 );
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year, false, true );
                if ( isUnknown )
                    tmptext = "";
            }

            qb.addMatch( q_cat3, tmptext, false, true );

            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle, true );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            if( c )
                qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
            if ( SortbyTrackFirst ) {
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            }
            if ( c ) qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
            if ( !SortbyTrackFirst ) {
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            }

            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

            category = IdNone;
            break;
    }

    qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    values = qb.run();
    uint countReturnValues = qb.countReturnValues();

    QPixmap pixmap;
    bool expandable = category != IdNone;
    if ( expandable )
        pixmap = iconForCategory( category );

    //this check avoid possible problems on database errors. FIXME: Should we add some real error handling here,
    //like calling a collection update or something?
    if ( values.isEmpty() ) { return; }

    for ( int i = values.count() - countReturnValues; i >= 0; i -= countReturnValues )
    {
        QString text;
        bool unknown=false;

        if (  category == IdVisYearAlbum )
            text += (  values[ i+1 ].isEmpty() ? "?" : values[ i+1 ] ) + i18n(  " - " );

        //show "artist - title" for compilations
        if ( c )
        {
            if ( values[ i + 2 ].stripWhiteSpace().isEmpty() )
            {
                text += i18n( "Unknown" ) + i18n( " - " );
                unknown = true;
            }
            else
                text = values[ i + 2 ] + i18n( " - " );
        }

        if ( values[ i ].stripWhiteSpace().isEmpty() )
        {
            if ( category == IdLabel )
                text += i18n( "No Label" );
            else
                text +=  i18n( "Unknown" );
            unknown = true;
        }
        else
            text += values[ i ];

        CollectionItem* child = new CollectionItem( item, category, unknown );
        child->setDragEnabled( true );
        child->setDropEnabled( false );
        child->setText( 0, text );
        if ( expandable )
            child->setPixmap( 0, pixmap );
        else
            child->setUrl( values[ i + 1 ] );
        child->setExpandable( expandable );
    }

    //Display the album cover for the parent item now it is expanded
    if ( dynamic_cast<CollectionItem*>( item ) )
    {
        CollectionItem *i = static_cast<CollectionItem*>( item );
        if ( i->m_cat == IdAlbum || i->m_cat == IdVisYearAlbum )
            i->setPixmap( 0, QPixmap() );   //The pixmap given is unimportant. The cover is used.
    }
}


void
CollectionView::slotCollapse( QListViewItem* item )  //SLOT
{
    //On collapse, go back from showing the cover to showing the icon for albums
    if ( dynamic_cast<CollectionItem*>( item ) )
    {
        CollectionItem *i = static_cast<CollectionItem*>( item );
        if ( i->m_cat == IdAlbum || i->m_cat == IdVisYearAlbum )
            i->setPixmap( 0, iconForCategory( i->m_cat ) );
    }

    QListViewItem* child = item->firstChild();
    QListViewItem* childTmp;

    //delete all children
    while ( child )
    {
        childTmp = child;
        child = child->nextSibling();
        delete childTmp;
    }
}

void
CollectionView::ratingChanged( const QString&, int )
{
    m_dirty = true;
    QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
}

void
CollectionView::presetMenu( int id )  //SLOT
{
    switch ( id )
    {
        case IdArtist:
            cat1Menu( IdArtist, false );
            cat2Menu( IdNone, false );
            cat3Menu( IdNone, false );
            break;
        case IdAlbum:
            cat1Menu( IdAlbum, false );
            cat2Menu( IdNone, false );
            cat3Menu( IdNone, false );
            break;
        case IdArtistAlbum:
            cat1Menu( IdArtist, false );
            cat2Menu( IdAlbum, false );
            cat3Menu( IdNone, false );
            break;
        case IdArtistVisYearAlbum:
            cat1Menu( IdArtist, false );
            cat2Menu( IdVisYearAlbum, false );
            cat3Menu( IdNone, false );
            break;
        case IdGenreArtist:
            cat1Menu( IdGenre, false );
            cat2Menu( IdArtist, false );
            cat3Menu( IdNone, false );
            break;
        case IdGenreArtistAlbum:
            cat1Menu( IdGenre, false );
            cat2Menu( IdArtist, false );
            cat3Menu( IdAlbum, false );
            break;
    }

    renderView(true);
}


void
CollectionView::cat1Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat1Menu->setItemChecked( m_cat1, false ); //uncheck old item
    m_parent->m_cat2Menu->setItemEnabled( m_cat1, true );  //enable old items
    m_parent->m_cat3Menu->setItemEnabled( m_cat1, true );
    m_cat1 = id;
    updateColumnHeader();
    resetIpodDepth();
    m_parent->m_cat1Menu->setItemChecked( m_cat1, true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );
    m_parent->m_cat3Menu->setItemEnabled( id , false );

    //if this item is checked in second menu, uncheck it
    if ( m_parent->m_cat2Menu->isItemChecked( id ) ) {
        m_parent->m_cat2Menu->setItemChecked( id, false );
        m_parent->m_cat2Menu->setItemChecked( IdNone, true );
        m_cat2 = IdNone;
        enableCat3Menu( false );
    }
    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        m_parent->m_cat3Menu->setItemChecked( IdNone, true );
        m_cat3 = IdNone;
    }
    updateTrackDepth();
    if ( rerender )
    {
        renderView(true);
    }
}


void
CollectionView::cat2Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( m_cat2, false ); //uncheck old item
    m_parent->m_cat3Menu->setItemEnabled( m_cat3, true );  //enable old item
    m_cat2 = id;
    m_parent->m_cat2Menu->setItemChecked( m_cat2, true );
    updateColumnHeader();
    resetIpodDepth();

    enableCat3Menu( id != IdNone );

    //prevent choosing the same category in both menus
    m_parent->m_cat3Menu->setItemEnabled( m_cat1 , false );
    if( id != IdNone )
        m_parent->m_cat3Menu->setItemEnabled( id , false );

    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        enableCat3Menu( false );
    }
    updateTrackDepth();
    if ( rerender )
    {
        renderView(true);
    }
}


void
CollectionView::cat3Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat3Menu->setItemChecked( m_cat3, false ); //uncheck old item
    m_cat3 = id;
    m_parent->m_cat3Menu->setItemChecked( m_cat3, true );
    updateColumnHeader();
    resetIpodDepth();
    updateTrackDepth();
    if ( rerender )
    {
        renderView(true);
    }
}


void
CollectionView::enableCat3Menu( bool enable )
{
    m_parent->m_cat3Menu->setItemEnabled( IdAlbum, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdVisYearAlbum, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdArtist, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdComposer, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdGenre, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdYear, enable );
    m_parent->m_cat3Menu->setItemEnabled( IdLabel, enable );

    if( !enable ) {
        m_parent->m_cat3Menu->setItemChecked( m_cat3, false );
        m_parent->m_cat3Menu->setItemChecked( IdNone, true );
        m_cat3 = IdNone;
    }
    updateTrackDepth();
}


void
CollectionView::invokeItem( QListViewItem* i, const QPoint& point, int column ) //SLOT
{
    if( column == -1 )
        return;

    QPoint p = mapFromGlobal( point );
    if ( p.x() > header()->sectionPos( header()->mapToIndex( 0 ) ) + treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin()
            || p.x() < header()->sectionPos( header()->mapToIndex( 0 ) ) )
        invokeItem( i );
}

void
CollectionView::invokeItem( QListViewItem* item ) //SLOT
{
    if ( !item || dynamic_cast<DividerItem*>(item) )
        return;

    item->setSelected( true );
    setCurrentItem( item );
    //append and prevent doubles in playlist
    if( item->isExpandable()  ||  m_viewMode == modeIpodView )
        Playlist::instance()->insertMedia( listSelected(), Playlist::DefaultOptions );
    else
        Playlist::instance()->insertMedia( static_cast<CollectionItem*>( item )->url(), Playlist::DefaultOptions );

}


// This slot is here to handle clicks on the right-arrow buttons
// in iPod browsing mode
void
CollectionView::ipodItemClicked( QListViewItem *item, const QPoint&, int c )
{
    if( item == 0 || c == 0 )
        return;
    if( m_viewMode != modeIpodView )
        return;

    // The Qt manual says NOT to delete items from within this slot
    QTimer::singleShot( 0, m_parent->m_ipodIncrement, SLOT( activate() ) );
}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( dynamic_cast<DividerItem*>( item ) ) return;

    int artistLevel = -1;

    if ( item ) {
        KPopupMenu menu( this );

        int cat = 0;
        if ( m_viewMode == modeTreeView ) {
            switch ( item->depth() )
            {
                case 0:
                    cat = m_cat1;
                    break;
                case 1:
                    if( m_cat1 == IdArtist )
                        artistLevel = 0;
                    cat = m_cat2;
                    break;
                case 2:
                    if( m_cat1 == IdArtist )
                        artistLevel = 0;
                    else if( m_cat2 == IdArtist )
                        artistLevel = 1;
                    cat = m_cat3;
                    break;
            }
        }

        else if ( m_viewMode == modeIpodView ) {
            int catArr[3] = {m_cat1, m_cat2, m_cat3};
            if ( m_currentDepth < trackDepth() )
                cat = catArr[m_currentDepth];
        }

        enum Actions { APPEND, QUEUE, MAKE, SAVE, MEDIA_DEVICE, BURN_ARTIST,
                       BURN_COMPOSER, BURN_ALBUM, BURN_CD, FETCH, INFO,
                       COMPILATION_SET, COMPILATION_UNSET, ORGANIZE, DELETE, TRASH,
                       FILE_MENU };

        QString trueItemText = getTrueItemText( cat, item );
        KURL::List selection = listSelected();
        QStringList siblingSelection = listSelectedSiblingsOf( cat, item );

        menu.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( Amarok::icon( "queue_track" ) ), selection.count() == 1 ? i18n( "&Queue Track" )
            : i18n( "&Queue Tracks" ), QUEUE );

        if( selection.count() > 1 || item->isExpandable() )
            menu.insertItem( SmallIconSet( Amarok::icon( "save" ) ), i18n( "&Save as Playlist..." ), SAVE );

        menu.insertSeparator();

        if( MediaBrowser::isAvailable() )
            menu.insertItem( SmallIconSet( Amarok::icon( "device" ) ), i18n( "&Transfer to Media Device" ), MEDIA_DEVICE );

        if( cat == IdArtist )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "burn" ) ), i18n( "&Burn All Tracks by This Artist" ), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() && siblingSelection.count() == 1 );
        }
        else if( cat == IdComposer )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "burn" ) ), i18n( "&Burn All Tracks by This Composer" ), BURN_COMPOSER );
            menu.setItemEnabled( BURN_COMPOSER, K3bExporter::isAvailable() && siblingSelection.count() == 1 );
        }
        else if( (cat == IdAlbum || cat == IdVisYearAlbum) )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "burn" ) ), i18n( "&Burn This Album" ), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() && siblingSelection.count() == 1 );
        }
        // !item->isExpandable() in tree mode corresponds to
        // showing tracks in iPod mode
        else if( !item->isExpandable() &&
                 (m_viewMode != modeIpodView || m_currentDepth == trackDepth()) )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "burn" ) ), i18n( "B&urn to CD" ), BURN_CD );
            menu.setItemEnabled( BURN_CD, K3bExporter::isAvailable() );
        }

        menu.insertSeparator();

        KPopupMenu fileMenu;
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n( "&Organize File..." , "&Organize %n Files..." , selection.count() ) , ORGANIZE );
        fileMenu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "&Delete File..." , "&Delete %n Files..." , selection.count() ) , DELETE );
        menu.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n( "Manage &Files" ), &fileMenu, FILE_MENU );

        if( (cat == IdAlbum || cat == IdVisYearAlbum) && siblingSelection.count() == 1 ) // cover fetch isn't multiselection capable
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "download" ) ), i18n( "&Fetch Cover From amazon.%1" ).arg( CoverManager::amazonTld() ), this, SLOT( fetchCover() ), 0, FETCH );
            #ifndef AMAZON_SUPPORT
            menu.setItemEnabled( FETCH, false );
            #endif
            if( trueItemText.isEmpty() ) // disable cover fetch for unknown albums
                menu.setItemEnabled( FETCH, false );
        }

        if ( ( (cat == IdAlbum || cat == IdVisYearAlbum) && siblingSelection.count() > 0 ) //album
            || ( !item->isExpandable() && m_viewMode == modeTreeView ) ) //or track
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "ok" ), i18n( "Show under &Various Artists" ), COMPILATION_SET );
            menu.insertItem( SmallIconSet( "cancel" ), i18n( "&Do not Show under Various Artists" ), COMPILATION_UNSET );
        }

        menu.insertSeparator();

        menu.insertItem( SmallIconSet( Amarok::icon( "info" ) )
            , i18n( "Edit Track &Information...",  "Edit &Information for %n Tracks...", selection.count())
            , this, SLOT( showTrackInfo() ), 0, INFO );

        switch( menu.exec( point ) )
        {
            case APPEND:
                Playlist::instance()->insertMedia( selection, Playlist::Append );
                break;
            case MAKE:
                Playlist::instance()->insertMedia( selection, Playlist::Replace );
                break;
            case SAVE:
                playlistFromURLs( selection );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( selection, Playlist::Queue );
                break;
            case MEDIA_DEVICE:
                MediaBrowser::queue()->addURLs( selection );
                break;
            case BURN_COMPOSER:
                K3bExporter::instance()->exportComposer( trueItemText );
                break;
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( trueItemText );
                break;
            case BURN_ALBUM:
                if( artistLevel == -1 || static_cast<CollectionItem *>(item)->isSampler() )
                {
                    K3bExporter::instance()->exportAlbum( trueItemText );
                }
                else
                {
                    QString artist;
                    if( item->depth() - artistLevel == 1 )
                        artist = item->parent()->text( 0 );
                    else if( item->depth() - artistLevel == 2 )
                        artist = item->parent()->parent()->text( 0 );
                    else if( item->depth() - artistLevel == 3 )
                        artist = item->parent()->parent()->parent()->text( 0 );
                    K3bExporter::instance()->exportAlbum( artist, trueItemText );
                }
                break;
            case BURN_CD:
                K3bExporter::instance()->exportTracks( selection );
                break;
            case COMPILATION_SET:
                setCompilation( selection, true );
                break;
            case COMPILATION_UNSET:
                setCompilation( selection, false );
                break;
            case ORGANIZE:
                organizeFiles( selection, i18n( "Organize Collection Files" ), false /* do not add to collection, just move */ );
                break;
            case DELETE:
                if ( DeleteDialog::showTrashDialog(this, selection) )
                  {
                    CollectionDB::instance()->removeSongs( selection );
                    foreachType( KURL::List, selection )
                      CollectionDB::instance()->emitFileDeleted( (*it).path() );
                  }
                m_dirty = true;
                QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
                break;
        }
    }
}


void
CollectionView::setViewMode( int mode, bool rerender /*=true*/ )
{
    if( m_viewMode == modeFlatView )
    {
        m_flatColumnWidths.clear();
        for ( int c = 0; c < columns(); ++c )
            m_flatColumnWidths.push_back( columnWidth( c ) );
    }

    m_viewMode = mode;
    clear();
    updateColumnHeader();

    if( m_viewMode == modeIpodView )
    {
        #if KDE_VERSION >= KDE_MAKE_VERSION(3,4,0)
        setShadeSortColumn( false );
        #endif
        m_parent->m_ipodDecrement->setEnabled( m_currentDepth > 0 );
        m_parent->ipodToolbar( true );
    }
    else
    {
        #if KDE_VERSION >= KDE_MAKE_VERSION(3,4,0)
        setShadeSortColumn( true );
        #endif
        m_parent->ipodToolbar( false );
    }

    if ( rerender )
    {
        // Pretend we just incremented the view depth so that
        // renderView() will call selectIpodItems()
        if( m_viewMode == modeIpodView )
            m_ipodIncremented = 1;

        renderView( true );
    }
}

void
CollectionItem::setPixmap(int column, const QPixmap & pix)
{
    //Don't show the cover if the album isn't expanded (for speed)
    if ( !isOpen() )
    {
        QListViewItem::setPixmap( column, pix );
        return;
    }

    //Generate Album name
    QString album( text( 0 ) ), artist;
    if ( m_cat == IdVisYearAlbum )
    {
        QString pointlessString;
        CollectionView::yearAlbumCalc( pointlessString, album );
    }
    else if ( m_cat != IdAlbum )
    {
        QListViewItem::setPixmap( column, pix );
        return;
    }

    //Now m_cat is either IdAlbum or IdVisYearAlbum, and so this is an album as required.

    //Now work out the artist 
    CollectionItem *p = this;
    while ( p->parent() && dynamic_cast<CollectionItem*>( p->parent() ) )
    {
        p = static_cast<CollectionItem*>( p->parent() );
        if ( IdArtist == p->m_cat )
        {
            artist = p->text( 0 );
            break;
        }
    }

    if ( artist.isNull() )
    {
        //Try to guess artist - this will only happen if you don't have an Artist category
        //above the Album category in the tree
        QueryBuilder qb;
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addMatch( QueryBuilder::tabAlbum, QueryBuilder::valName, album );

        QStringList values( qb.run() );

        if ( !values.isEmpty() )
            artist = values[ 0 ];
        else
        {
            //Don't bother trying to create a shadow because it won't work anyway. The
            //nocover image has intial transparency, so adding the shadow doesn't work.
            QListViewItem::setPixmap( column, QPixmap( CollectionDB::instance()->notAvailCover( false, 50 ) ) );
            return;
        }
    }

    QListViewItem::setPixmap( column, QPixmap( CollectionDB::instance()->albumImage( artist, album, true, 50 ) ) );
}


void
CollectionView::fetchCover() //SLOT
{
    #ifdef AMAZON_SUPPORT
    CollectionItem* item = static_cast<CollectionItem*>( currentItem() );
    if ( !item ) return;

    int cat = 0;
    switch ( item->depth() )
    {
        case 0:
            cat = m_cat1;
            break;
        case 1:
            cat = m_cat2;
            break;
        case 2:
            cat = m_cat3;
            break;
    }

    QString album = item->text(0);
    if( cat == IdVisYearAlbum )
    {
        // we can't use findRev since an album may have " - " within it.
        QString sep = i18n(" - ");
        album = album.right( album.length() - sep.length() - album.find( sep ) );
    }

    // find the first artist's name
    QStringList values =
        CollectionDB::instance()->query( QString (
            "SELECT DISTINCT artist.name FROM artist, album, tags "
            "WHERE artist.id = tags.artist AND tags.album = album.id "
            "AND album.name = '%1';" )
            .arg( CollectionDB::instance()->escapeString( album ) ) );

    if ( !values.isEmpty() )
        CollectionDB::instance()->fetchCover( this, values[0], album, false, static_cast<QListViewItem*>(item) );
    #endif
}

void
CollectionView::showTrackInfo() //SLOT
{
    DEBUG_BLOCK
     KURL::List urls = listSelected();
     int selectedTracksNumber = urls.count();

     //If we have only one, call the full dialog. Otherwise, the multiple tracks one.
     if ( selectedTracksNumber == 1 )
     {
          TagDialog* dialog = new TagDialog( urls.first(), instance() );
          dialog->show();
     }
     else if ( selectedTracksNumber )
     {
          TagDialog* dialog = new TagDialog( urls, instance() );
          dialog->show();
     }
}

bool
CollectionView::isOrganizingFiles() const
{
    return m_organizeURLs.count() > 0;
}

void CollectionView::cancelOrganizingFiles()
{
    // Set the indicator
    m_organizingFileCancelled = true;

    // Cancel the current underlying CollectionDB::instance()->moveFile operation
    CollectionDB::instance()->cancelMovingFileJob();
}

void
CollectionView::organizeFiles( const KURL::List &urls, const QString &caption, bool copy )  //SLOT
{
    if( m_organizingFileCancelled )
    {
        QString shortMsg = i18n( "Cannot start organize operation until jobs are aborted." );
        Amarok::StatusBar::instance()->shortMessage( shortMsg, KDE::StatusBar::Sorry );
        return;
    }

    if( m_organizeURLs.count() )
    {
        if( copy != m_organizeCopyMode )
        {
            QString shortMsg = i18n( "Cannot start organize operation of different kind while another is in progress." );
            Amarok::StatusBar::instance()->shortMessage( shortMsg, KDE::StatusBar::Sorry );
            return;
        }
        else
        {
            m_organizeURLs += Amarok::recursiveUrlExpand( urls );
            Amarok::StatusBar::instance()->incrementProgressTotalSteps( this, urls.count() );
            return;
        }
    }

    QStringList folders = MountPointManager::instance()->collectionFolders();
    if( folders.isEmpty() )
    {
        QString longMsg = i18n( "You need to configure at least one folder for your collection for organizing your files." );
        Amarok::StatusBar::instance()->longMessage( longMsg, KDE::StatusBar::Sorry );
        return;
    }

    OrganizeCollectionDialogBase base( m_parent, "OrganizeFiles", true, caption,
            KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::Details );
    QVBox* page = base.makeVBoxMainWidget();

    OrganizeCollectionDialog dialog( page );
    dialog.folderCombo->insertStringList( folders, 0 );
    dialog.folderCombo->setCurrentItem( AmarokConfig::organizeDirectory() );
    dialog.overwriteCheck->setChecked( AmarokConfig::overwriteFiles() );
    dialog.filetypeCheck->setChecked( AmarokConfig::groupByFiletype() );
    dialog.initialCheck->setChecked( AmarokConfig::groupArtists() );
    dialog.spaceCheck->setChecked( AmarokConfig::replaceSpace() );
    dialog.coverCheck->setChecked( AmarokConfig::coverIcons() );
    dialog.ignoreTheCheck->setChecked( AmarokConfig::ignoreThe() );
    dialog.vfatCheck->setChecked( AmarokConfig::vfatCompatible() );
    dialog.asciiCheck->setChecked( AmarokConfig::asciiOnly() );
    dialog.customschemeCheck->setChecked( AmarokConfig::useCustomScheme() );
    dialog.formatEdit->setText( AmarokConfig::customScheme() );
    dialog.regexpEdit->setText( AmarokConfig::replacementRegexp() );
    dialog.replaceEdit->setText( AmarokConfig::replacementString() );
    connect( &base, SIGNAL(detailsClicked()), &dialog, SLOT(slotDetails()) );

    if( dialog.customschemeCheck->isChecked() )
    {
        base.setDetails( true );
    }
    else
    {
        dialog.slotDetails();
    }

    KURL::List previewURLs = Amarok::recursiveUrlExpand( urls.first(), 1 );
    if( previewURLs.count() )
    {
        dialog.setPreviewBundle( MetaBundle( previewURLs.first() ) );
        dialog.update( 0 );
    }

    base.setInitialSize( QSize( 450, 350 ) );

    if( base.exec() == KDialogBase::Accepted )
    {
        AmarokConfig::setOrganizeDirectory( dialog.folderCombo->currentItem() );
        AmarokConfig::setOverwriteFiles( dialog.overwriteCheck->isChecked() );
        AmarokConfig::setGroupByFiletype( dialog.filetypeCheck->isChecked() );
        AmarokConfig::setGroupArtists( dialog.initialCheck->isChecked() );
        AmarokConfig::setIgnoreThe( dialog.ignoreTheCheck->isChecked() );
        AmarokConfig::setReplaceSpace( dialog.spaceCheck->isChecked() );
        AmarokConfig::setCoverIcons( dialog.coverCheck->isChecked() );
        AmarokConfig::setVfatCompatible( dialog.vfatCheck->isChecked() );
        AmarokConfig::setAsciiOnly( dialog.asciiCheck->isChecked() );
        AmarokConfig::setUseCustomScheme( dialog.customschemeCheck->isChecked() );
        AmarokConfig::setCustomScheme( dialog.formatEdit->text() );
        AmarokConfig::setReplacementRegexp( dialog.regexpEdit->text() );
        AmarokConfig::setReplacementString( dialog.replaceEdit->text() );
        KURL::List skipped;

        m_organizeURLs = Amarok::recursiveUrlExpand( urls );
        m_organizeCopyMode = copy;
        CollectionDB::instance()->createTables( true ); // create temp tables
        Amarok::StatusBar::instance()->newProgressOperation( this )
            .setDescription( caption )
            .setAbortSlot( this, SLOT( cancelOrganizingFiles() ) )
            .setTotalSteps( m_organizeURLs.count() );

        while( !m_organizeURLs.empty() && !m_organizingFileCancelled )
        {
            KURL &src = m_organizeURLs.first();

            if( !CollectionDB::instance()->organizeFile( src, dialog, copy ) )
            {
                skipped += src;
            }

            m_organizeURLs.pop_front();
            Amarok::StatusBar::instance()->incrementProgress( this );

            if( m_organizingFileCancelled ) m_organizeURLs.clear();
        }

        CollectionDB::instance()->sanitizeCompilations(); //queryBuilder doesn't handle unknownCompilations
        CollectionDB::instance()->copyTempTables(); // copy temp table contents to permanent tables
        CollectionDB::instance()->dropTables( true ); // and drop them

        // and now do an incremental scan since this was disabled while organizing files
        QTimer::singleShot( 0, CollectionDB::instance(), SLOT( scanMonitor() ) );

        if( !m_organizingFileCancelled && skipped.count() > 0 )
        {
            QString longMsg = i18n( "The following file could not be organized: ",
                    "The following %n files could not be organized: ", skipped.count() );
            bool first = true;
            for( KURL::List::iterator it = skipped.begin();
                    it != skipped.end();
                    it++ )
            {
                if( !first )
                    longMsg += i18n( ", " );
                else
                    first = false;
                longMsg += (*it).path();
            }
            longMsg += i18n( "." );

            QString shortMsg = i18n( "Sorry, one file could not be organized.",
                    "Sorry, %n files could not be organized.", skipped.count() );
            Amarok::StatusBar::instance()->shortLongMessage( shortMsg, longMsg, KDE::StatusBar::Sorry );
        }
        else if ( m_organizingFileCancelled )
        {
            Amarok::StatusBar::instance()->shortMessage( i18n( "Aborting jobs..." ) );
            m_organizingFileCancelled = false;
        }

        m_dirty = true;
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
        Amarok::StatusBar::instance()->endProgressOperation( this );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() != viewport() && e->source() != this && KURLDrag::canDecode( e ) );
}

void
CollectionView::contentsDragMoveEvent( QDragMoveEvent *e )
{
    e->accept( e->source() != viewport() && e->source() != this && KURLDrag::canDecode( e ) );
}

void
CollectionView::contentsDropEvent( QDropEvent *e )
{
    KURL::List list;
    if( KURLDrag::decode( e, list ) )
    {
        KURL::List expandedList;
        int dropped = 0;
        int invalid = 0;
        for( KURL::List::iterator it = list.begin();
                it != list.end();
                ++it )
        {
            if( (*it).isLocalFile() && QFileInfo( (*it).path() ).isDir() )
                expandedList += Amarok::recursiveUrlExpand( *it );
            else
                expandedList += *it;
        }

        KURL::List cleanList;
        for( KURL::List::iterator it = expandedList.begin();
                it != expandedList.end();
                ++it )
        {
            QString proto = (*it).protocol();
            if( !MetaBundle::isKioUrl( *it ) )
                invalid++;
            else if( (*it).isLocalFile() && CollectionDB::instance()->isFileInCollection( (*it).path() ) )
                dropped++;
            else
                cleanList += *it;
        }

        QString msg;
        if( dropped > 0 )
            msg += i18n( "One file already in collection",
                    "%n files already in collection", dropped );
        if( invalid > 0 )
            if( msg.isEmpty() )
                msg += i18n( "One dropped file is invalid",
                        "%n dropped files are invalid", invalid );
            else
                msg += i18n( ", one dropped file is invalid",
                        ", %n dropped files are invalid", invalid );
        if( !msg.isEmpty() )
            Amarok::StatusBar::instance()->shortMessage( msg );
        if( cleanList.count() > 0 )
            organizeFiles( list, i18n( "Copy Files To Collection" ), true /* copy */ );
    }
}

void
CollectionView::dropProxyEvent( QDropEvent *e )
{
    contentsDropEvent( e );
}

void
CollectionView::safeClear()
{
    bool block = signalsBlocked();
    blockSignals( true );
    clearSelection();

    QMap<QListViewItem*, CoverFetcher*> *itemCoverMap = CollectionDB::instance()->getItemCoverMap();
    QMutex* itemCoverMapMutex = CollectionDB::instance()->getItemCoverMapMutex();
    QListViewItem *c = firstChild();
    QListViewItem *n;
    itemCoverMapMutex->lock();
    while( c ) {
        if( itemCoverMap->contains( c ) )
            itemCoverMap->erase( c );
        n = c->nextSibling();
        delete c;
        c = n;
    }
    itemCoverMapMutex->unlock();
    blockSignals( block );
    triggerUpdate();
}

void
CollectionView::updateColumnHeader()
{
    // remove all columns
    for ( int i = columns() - 1; i >= 0 ; --i )
        removeColumn( i );

    if ( m_viewMode == modeFlatView )
    {
        setResizeMode( QListView::NoColumn );

        if( m_flatColumnWidths.size() == 0 )
        {
            addColumn( captionForTag( Title ) );
#define includesArtist(cat) (((cat)&IdArtist) \
        ||((cat)&IdArtistAlbum) \
        ||((cat)&IdGenreArtist) \
        ||((cat)&IdGenreArtistAlbum) \
        ||((cat)&IdArtistVisYearAlbum))
            if( includesArtist(m_cat1)||includesArtist(m_cat2)||includesArtist(m_cat3) )
                addColumn( captionForTag( Artist ) );
            else
                addColumn( captionForTag( Artist ), 0 );
#undef includesArtist
            if( m_cat1&IdComposer || m_cat2&IdComposer || m_cat3&IdComposer )
                addColumn( captionForTag( Composer ) );
            else
                addColumn( captionForTag( Composer ), 0 );
#define includesAlbum(cat) (((cat)&IdAlbum) \
        ||((cat)&IdArtistAlbum) \
        ||((cat)&IdGenreArtistAlbum) \
        ||((cat)&IdVisYearAlbum) \
        ||((cat)&IdArtistVisYearAlbum))
            if( includesAlbum(m_cat1)||includesAlbum(m_cat2)||includesAlbum(m_cat3) )
                addColumn( captionForTag( Album ) );
            else
                addColumn( captionForTag( Album ), 0 );
#undef includesAlbum
#define includesGenre(cat) (((cat)&IdGenre) \
        ||((cat)&IdGenreArtist) \
        ||((cat)&IdGenreArtistAlbum))
            if( includesGenre(m_cat1)||includesGenre(m_cat2)||includesGenre(m_cat3) )
                addColumn( captionForTag( Genre ) );
            else
                addColumn( captionForTag( Genre ), 0 );
#undef includesGenre
            addColumn( captionForTag( Length ),0  );
            addColumn( captionForTag( DiscNumber ), 0 );
            addColumn( captionForTag( Track ), 0 );
#define includesYear(cat) (((cat)&IdYear) \
        ||((cat)&IdVisYearAlbum) \
        ||((cat)&IdArtistVisYearAlbum))
            if( includesYear(m_cat1)||includesYear(m_cat2)||includesYear(m_cat3) )
                addColumn( captionForTag( Year ) );
            else
                addColumn( captionForTag( Year ), 0 );
#undef includesYear
            addColumn( captionForTag( Comment ), 0 );
            addColumn( captionForTag( Playcount ), 0 );
            addColumn( captionForTag( Score ), 0 );
            addColumn( captionForTag( Rating ), 0 );
            addColumn( captionForTag( Filename ), 0 );
            addColumn( captionForTag( Firstplay ), 0 );
            addColumn( captionForTag( Lastplay ), 0 );
            addColumn( captionForTag( Modified ), 0 );
            addColumn( captionForTag( Bitrate ), 0 );
            addColumn( captionForTag( Filesize ), 0 );
            addColumn( captionForTag( BPM ), 0 );
        }
        else
        {
            for( uint tag = 0; tag < NUM_TAGS; ++tag ) {
                if( tag < m_flatColumnWidths.size() )
                    addColumn( captionForTag( static_cast<Tag>( tag ) ), m_flatColumnWidths[tag] );
                else
                    addColumn( captionForTag( static_cast<Tag>( tag ) ), 0 );
            }
        }

        setColumnAlignment( Track, Qt::AlignCenter );
        setColumnAlignment( DiscNumber, Qt::AlignCenter );
        setColumnAlignment( Length, Qt::AlignRight );
        setColumnAlignment( Bitrate, Qt::AlignCenter );
        setColumnAlignment( Score, Qt::AlignCenter );
        setColumnAlignment( Playcount, Qt::AlignCenter );
        setColumnAlignment( BPM, Qt::AlignRight );

        //QListView allows invisible columns to be resized, so we disable resizing for them
        for ( int i = 0; i < columns(); ++i ) {
            setColumnWidthMode ( i, QListView::Manual );
            if ( columnWidth( i ) == 0 )
                header()->setResizeEnabled( false, i );
        }
        setRootIsDecorated( false );
    }
    else if ( m_viewMode == modeTreeView )
    {
        setResizeMode( QListView::LastColumn );

        QString caption = captionForCategory( m_cat1 );
        int catArr[2] = {m_cat2, m_cat3};

        for(int i = 0; i < 2; i++) {
            if (catArr[i] != IdNone ) {
                caption += " / " + captionForCategory( catArr[i] );
            }
        }
        addColumn( caption );
        setRootIsDecorated( true );
    }

    // The iPod columns
    // When not in track mode, there are two columns: the first
    // contains the text + pixmap, and the second just has the
    // right-arrow pixmap.  In any case we're not in tree mode.
    else if ( m_viewMode == modeIpodView )
    {
        QString caption;

        if( m_currentDepth == trackDepth() )
          caption = i18n( "Tracks" );

        else
          {
            int catArr[3] = {m_cat1, m_cat2, m_cat3};
            caption = captionForCategory( catArr[m_currentDepth] );
          }

        addColumn( caption );

        if( m_currentDepth != trackDepth() )
          {
            QPixmap pixmap = ipodDecrementIcon();
            // This column is for the "->" buttons.  The width is just
            // a guess, and will be changed once an item is added.
            addColumn( "", pixmap.width() + 10 );
          }
        setRootIsDecorated( false );

        // Neither column should automatically stretch, since this tends
        // to add a scrollbar when it's not needed, and anyway we manually
        // stretch it in viewportResizeEvent()
        header()->setStretchEnabled( false, 0 );
        if( m_currentDepth != trackDepth() )
          header()->setStretchEnabled( false, 1 );

    }

    //manage column widths
    QResizeEvent rev( size(), QSize() );
    viewportResizeEvent( &rev );

    m_parent->m_categoryMenu->setItemChecked( IdArtist, m_cat1 == IdArtist && m_cat2 == IdNone );
    m_parent->m_categoryMenu->setItemChecked( IdAlbum, m_cat1 == IdAlbum && m_cat2 == IdNone );
    m_parent->m_categoryMenu->setItemChecked( IdArtistAlbum, m_cat1 == IdArtist && m_cat2 == IdAlbum && m_cat3 == IdNone );
    m_parent->m_categoryMenu->setItemChecked( IdArtistVisYearAlbum, m_cat1 == IdArtist && m_cat2 == IdVisYearAlbum && m_cat3 == IdNone );
    m_parent->m_categoryMenu->setItemChecked( IdGenreArtist, m_cat1 == IdGenre && m_cat2 == IdArtist && m_cat3 == IdNone );
    m_parent->m_categoryMenu->setItemChecked( IdGenreArtistAlbum, m_cat1 == IdGenre && m_cat2 == IdArtist && m_cat3 == IdAlbum );
}


void
CollectionView::startDrag()
{
    KURL::List urls = listSelected();
    KURLDrag* d = new KURLDrag( urls, this );
    d->setPixmap( CollectionDB::createDragPixmap(urls),
                  QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X,
                          CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    d->dragCopy();
}

QString
CollectionView::getTrueItemText( int cat, QListViewItem* item ) const
{
    //Work out the true name of the album ( where Unknown is "" ) , and the
    QString trueItemText;
    if ( item == 0 )
    {
        warning() << "getTrueItemText() called for empty CollectionItem" << endl;
        return QString();
    }
    if ( dynamic_cast<CollectionItem*>( item ) )
    {
        CollectionItem* collectItem = static_cast<CollectionItem*>( item );
        trueItemText = collectItem->getSQLText( 0 );
        if ( cat == IdVisYearAlbum && !collectItem->isUnknown() )
            trueItemText = trueItemText.right( trueItemText.length() - trueItemText.find( i18n( " - " ) ) - i18n( " - " ).length() );
    }
    else
    {
        trueItemText = item->text( 0 );
        warning() << "getTrueItemText() called for non-CollectionItem with text '" << trueItemText << '\'' << endl;
    }
    return trueItemText;
}

QStringList
CollectionView::listSelectedSiblingsOf( int cat, QListViewItem* item )
{
    // notice that using the nextSibling()-axis does not work in this case as this
    // would only select items below the specified item.
    QStringList list;
    QString trueItemText;
    int depth = item->depth();

    // go to top most item
    while( item && item->itemAbove() )
    {
        item = item->itemAbove();
        //debug() << "walked up to item: " << getTrueItemText( cat, item ) << endl;
    }
    // walk down to get all selected items in same depth
    while( item )
    {
        if ( item->isSelected() && item->depth() == depth )
        {
            trueItemText = getTrueItemText( cat, item );
            //debug() << "selected item: " << trueItemText << endl;
            list << trueItemText;
        }
        item = item->itemBelow();
    }
    return list;
}

KURL::List
CollectionView::listSelected()
{
    //Here we determine the URLs of all selected items. We use two passes, one for the parent items,
    //and another one for the children.

    KURL::List list;
    QListViewItem* item;
    QStringList values;
    QueryBuilder qb;

    // initialization for year - album mode
    QString tmptext;
    bool unknownText;
    int VisYearAlbum = -1;
    int q_cat1=m_cat1;
    int q_cat2=m_cat2;
    int q_cat3=m_cat3;
    if (m_cat1 == IdVisYearAlbum || m_cat2 == IdVisYearAlbum || m_cat3 == IdVisYearAlbum)
    {
        if (m_cat1==IdVisYearAlbum)
        {
            VisYearAlbum = 1;
            q_cat1 = IdAlbum;
        }
        if (m_cat2==IdVisYearAlbum)
        {
            VisYearAlbum = 2;
            q_cat2 = IdAlbum;
        }
        if (m_cat3==IdVisYearAlbum)
        {
            VisYearAlbum = 3;
            q_cat3 = IdAlbum;
        }
    }

    if ( m_viewMode == modeFlatView )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            if ( item->isSelected() )
                list << static_cast<CollectionItem*>( item ) ->url();

        return list;
    }


    // The iPod selection code is written to resemble the tree mode
    // selection logic, as well as what happens on an actual iPod.  If
    // we're in track mode, just return the list of tracks selected.
    // Otherwise select all children of all currently selected items,
    // sorting first by m_cat1, then m_cat2, then m_cat3.  Sort by
    // track first if one of the categories is Id(VisYear)Album.
    // There is a difficulty with compilation albums -- if we're
    // sorting by track first then we want to group compilation albums
    // separately, and not with the individual artists, even if that's
    // one of the categories (e.g. if the user just adds one
    // compilation album, we can't sort by artist first).  In other
    // words, when one of the categories is Id(VisYear)Album,
    // compilation albums should behave as if the artist is Various
    // Artists, for sorting purposes.  This is handled by making two
    // separate queries, the first for the compilations, and the
    // second for everything else.

    //   Note that if "All" is currently selected then the other
    // selections are ignored.
    if ( m_viewMode == modeIpodView )
    {
        // If we're already displaying tracks, just return the selected ones
        if( m_currentDepth == trackDepth() )
        {
            QPtrList<QListViewItem> selected = selectedItems();
            QPtrList<QListViewItem>::iterator it = selected.begin();
            while( it != selected.end() )
            {
                if( dynamic_cast<CollectionItem*>(*it) != 0 )
                    list << dynamic_cast<CollectionItem*>(*it)->url();
                ++it;
            }
            return list;
        }

        // We're not displaying tracks.
        QueryBuilder qb;

        // Do a "fake" depth incrementation to figure out the
        // correct filters at the current depth
        incrementDepth( false );

        // Copy the filter list before calling decrementDepth() below
        QStringList filters[3];
        for( int i = 0; i < m_currentDepth; ++i )
            filters[i] = m_ipodFilters[i];
        QStringList filterYear = m_ipodFilterYear;

        // Undo the fake depth incrementation
        decrementDepth( false );

        int catArr[3] = {m_cat1, m_cat2, m_cat3};
        int tables = 0;
        bool sortByTrackFirst = false;
        for(int i = 0; i < trackDepth(); ++i)
            tables |= (catArr[i] == IdVisYearAlbum
                    ? IdAlbum
                    : catArr[i]);

        // Figure out if the results will be sorted by track first
        // (i.e., if one of the filters is by album).  If so, we need
        // to search compilations first.
        if( tables & IdAlbum )
            sortByTrackFirst = true;

        if( sortByTrackFirst )
        {
            // Build the query, recursively sorted.  First get compilation
            // albums so they'll be first, not sorted by artist
            buildIpodQuery( qb, trackDepth(), filters, filterYear, true, true );

            if ( translateTimeFilter( timeFilter() ) > 0 )
                qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

            qb.setOptions( QueryBuilder::optOnlyCompilations );
            qb.setGoogleFilter( tables | QueryBuilder::tabSong, m_filter );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            values = qb.run();
        }

        // Now build the non-compilation album query
        qb.clear();

        buildIpodQuery( qb, trackDepth(), filters, filterYear, true, false );

        if ( translateTimeFilter( timeFilter() ) > 0 )
            qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

        if( sortByTrackFirst )
            qb.setOptions( QueryBuilder::optNoCompilations );
        qb.setGoogleFilter( tables | QueryBuilder::tabSong, m_filter );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

        values += qb.run();

        int total = values.count() / qb.countReturnValues();

        // This shouldn't happen
        if( total == 0 )
            return list;


        QStringList::Iterator it = values.begin();
        KURL tmp;
        while ( it != values.end() )
        {
            tmp.setPath( (*(it++)) );
            list << tmp;
        }

        return list;
    }


    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() )
        {
            const bool sampler = static_cast<CollectionItem*>( item )->isSampler();
            qb.clear();
            if ( translateTimeFilter( timeFilter() ) > 0 )
                qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            tmptext = static_cast<CollectionItem*>( item )->getSQLText( 0 );
            unknownText = tmptext.isEmpty();

            if ( !sampler )
            {
                if ( q_cat1 == IdArtist )
                    qb.setOptions( QueryBuilder::optNoCompilations );
                if( VisYearAlbum == 1 )
                {
                    tmptext = item->text( 0 );
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year, false, true );
                    if ( unknownText )
                        tmptext = "";
                }

                qb.addMatch( q_cat1, tmptext, false, true );
            }
            else
                qb.setOptions( QueryBuilder::optOnlyCompilations );

            qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );

            if( VisYearAlbum == 1 )
                qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

            if( !sampler )    qb.sortBy( q_cat1, QueryBuilder::valName );

            if( VisYearAlbum == 2 )
                qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

            if( q_cat2 != QueryBuilder::tabSong )
                qb.sortBy( q_cat2, QueryBuilder::valName );

            if( VisYearAlbum == 3 )
                qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

            if( q_cat3 != QueryBuilder::tabSong )
                qb.sortBy( q_cat3, QueryBuilder::valName );

            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

            qb.setOptions( QueryBuilder::optRemoveDuplicates );
            values = qb.run();

            for ( uint i = 0; i < values.count(); i++ )
            {
                KURL tmp;
                tmp.setPath( values[i] );
                list << tmp;
            }
        }

    //second pass: category 1
    if ( m_cat2 == IdNone )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() && !child->parent()->isSelected() )
                    list << static_cast<CollectionItem*>( child ) ->url();
    }
    else {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() && !child->parent()->isSelected() )
                {
                    const bool sampler = static_cast<CollectionItem*>( item )->isSampler();
                    qb.clear();
                    if ( translateTimeFilter( timeFilter() ) > 0 )
                        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                    if ( !sampler )
                    {
                        if ( q_cat1 == IdArtist )
                            qb.setOptions( QueryBuilder::optNoCompilations );

                        tmptext = static_cast<CollectionItem*>( item )->getSQLText( 0 );
                        unknownText = tmptext.isEmpty();

                        if( VisYearAlbum == 1 )
                        {
                            tmptext = item->text( 0 );
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year, false, true );
                            if ( unknownText )
                                tmptext = "";
                        }

                        qb.addMatch( q_cat1, tmptext, false, true );
                    }
                    else
                        qb.setOptions( QueryBuilder::optOnlyCompilations );


                    tmptext = static_cast<CollectionItem*>( child )->getSQLText( 0 );
                    unknownText = tmptext.isEmpty();

                    if( VisYearAlbum == 2 )
                    {
                        tmptext = child->text( 0 );
                        QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                        yearAlbumCalc( year, tmptext );
                        qb.addMatch( QueryBuilder::tabYear, year, false, true );
                        if ( unknownText )
                            tmptext = "";
                    }

                    qb.addMatch( q_cat2, tmptext, false, true );

                    qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );

                    if( VisYearAlbum == 1 )
                        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                    if( !sampler )
                        qb.sortBy( q_cat1, QueryBuilder::valName );

                    if( VisYearAlbum == 2 )
                        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                    qb.sortBy( q_cat2, QueryBuilder::valName );

                    if( VisYearAlbum == 3 )
                        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                    if( q_cat3 != QueryBuilder::tabSong )
                        qb.sortBy( q_cat3, QueryBuilder::valName );

                    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
                    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

                    qb.setOptions( QueryBuilder::optRemoveDuplicates );
                    values = qb.run();

                    for ( uint i = 0; i < values.count(); i++ )
                    {
                        KURL tmp;
                        tmp.setPath( values[i] );
                        list << tmp;
                    }
                }
    }

    //third pass: category 2
    for ( item = firstChild(); item; item = item->nextSibling() )
        for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
            for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                if ( grandChild->isSelected() && !grandChild->isExpandable() && !child->parent()->isSelected() && !child->isSelected() )
                    list << static_cast<CollectionItem*>( grandChild ) ->url();

    //category 3
    if ( m_cat3 == IdNone )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    for ( QListViewItem* grandChild2 = grandChild->firstChild(); grandChild2; grandChild2 = grandChild2->nextSibling() )
                        if ( grandChild2->isSelected() && !child->parent()->isSelected() && !child->isSelected() && !grandChild->isSelected() )
                            list << static_cast<CollectionItem*>( grandChild2 ) ->url();
    }
    else {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    if ( grandChild->isSelected() && !grandChild->parent()->isSelected() )
                    {
                        const bool sampler = static_cast<CollectionItem*>( item )->isSampler();
                        qb.clear();
                        if ( translateTimeFilter( timeFilter() ) > 0 )
                            qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

                        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                        if ( !sampler )
                        {
                            if ( q_cat1 == IdArtist )
                                qb.setOptions( QueryBuilder::optNoCompilations );

                            tmptext = static_cast<CollectionItem*>( item )->getSQLText( 0 );
                            unknownText = tmptext.isEmpty();

                            if( VisYearAlbum == 1 )
                            {
                                tmptext = item->text( 0 );
                                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                                yearAlbumCalc( year, tmptext );
                                qb.addMatch( QueryBuilder::tabYear, year, false, true );
                                if ( unknownText )
                                    tmptext = "";
                            }

                            qb.addMatch( q_cat1, tmptext, false, true );
                        }
                        else
                            qb.setOptions( QueryBuilder::optOnlyCompilations );

                        tmptext = static_cast<CollectionItem*>( child )->getSQLText( 0 );
                        unknownText = tmptext.isEmpty();

                        if( VisYearAlbum == 2 )
                        {
                            tmptext = child->text( 0 );
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year, false, true );
                            if ( unknownText )
                                tmptext = "";
                        }

                        qb.addMatch( q_cat2, tmptext, false, true );

                        tmptext = static_cast<CollectionItem*>( grandChild )->getSQLText( 0 );
                        unknownText = tmptext.isEmpty();

                        if( VisYearAlbum == 3 )
                        {
                            tmptext = grandChild->text( 0 );
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year, false, true );
                            if ( unknownText )
                                tmptext = "";
                        }

                        qb.addMatch( q_cat3, tmptext, false, true );

                        qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );

                        if( VisYearAlbum == 1 )
                            qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                        if( !sampler )
                            qb.sortBy( q_cat1, QueryBuilder::valName );

                        if( VisYearAlbum == 2 )
                            qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                        qb.sortBy( q_cat2, QueryBuilder::valName );

                        if( VisYearAlbum == 3 )
                            qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName);

                        qb.sortBy( q_cat3, QueryBuilder::valName );

                        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
                        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

                        qb.setOptions( QueryBuilder::optRemoveDuplicates );
                        values = qb.run();

                        for ( uint i = 0; i < values.count(); i++ )
                        {
                            KURL tmp;
                            tmp.setPath( values[i] );
                            list << tmp;
                        }
                    }
    }

    //category 3
    for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    for ( QListViewItem* grandChild2 = grandChild->firstChild(); grandChild2; grandChild2 = grandChild2->nextSibling() )
                        if ( grandChild2->isSelected() && !child->parent()->isSelected() && !child->isSelected() && !grandChild->isSelected() )
                            list << static_cast<CollectionItem*>( grandChild2 ) ->url();

    return list;
}


void
CollectionView::playlistFromURLs( const KURL::List &urls )
{
    QString suggestion;
    typedef QListViewItemIterator It;
    It it( this, It::Visible | It::Selected );
    if( (*it) && !(*(++it)) )
        suggestion = (*It( this, It::Visible | It::Selected ))->text( 0 );
    else
        suggestion = i18n( "Untitled" );
    const QString path = PlaylistDialog::getSaveFileName( suggestion );

    if( path.isEmpty() )
        return;

    CollectionDB* db = CollectionDB::instance();

    QValueList<QString> titles;
    QValueList<int> lengths;
    for( KURL::List::ConstIterator it = urls.constBegin(), end = urls.constEnd(); it != end; ++it )
    {
        int deviceid = MountPointManager::instance()->getIdForUrl( *it );
        KURL rpath;
        MountPointManager::instance()->getRelativePath( deviceid, *it, rpath );
        const QString query = QString("SELECT title, length FROM tags WHERE url = '%1' AND deviceid = %2;")
                              .arg( db->escapeString( rpath.path() ) ).arg( deviceid );
        debug() << "media id: " << deviceid << " rpath: " << rpath.path() << endl;
        QStringList result = db->query( query );
        titles << result[0];
        lengths << result[1].toInt();
    }

    if( PlaylistBrowser::savePlaylist( path, urls, titles, lengths ) )
        PlaylistWindow::self()->showBrowser( "PlaylistBrowser" );
}

QPixmap
CollectionView::iconForCategory( const int cat ) const
{
    QString icon;
    switch( cat )
    {
        case IdAlbum:
            icon = "cdrom_unmount";
            break;
        case IdVisYearAlbum:
            icon = "cdrom_unmount";
            break;
        case IdArtist:
            icon = "personal";
            break;
        case IdComposer:
            icon = "personal";
            break;

        case IdGenre:
            icon = "kfm";
            break;

        case IdYear:
            icon = "history";
            break;

        case IdLabel:
            icon = "kfm";
            break;
    }

    return KGlobal::iconLoader()->loadIcon( icon, KIcon::Toolbar, KIcon::SizeSmall );
}

QString
CollectionView::captionForCategory( const int cat ) const
{
    switch( cat )
    {
        case IdAlbum:
            return i18n( "Album" );
            break;
        case IdVisYearAlbum:
            return i18n( "Year" ) + i18n( " - " ) + i18n( "Album" );
            break;
        case IdArtist:
            return i18n( "Artist" );
            break;
        case IdComposer:
            return i18n( "Composer" );
            break;

        case IdGenre:
            return i18n( "Genre" );
            break;

        case IdYear:
            return i18n( "Year" );
            break;
        case IdLabel:
            return i18n( "Label" );
            break;
    }

    return QString();
}


QString
CollectionView::captionForTag( const Tag tag) const
{
    QString caption;
    switch( tag )
    {
        case Artist:    caption = i18n( "Artist" ); break;
        case Album:     caption = i18n( "Album" );  break;
        case Genre:     caption = i18n( "Genre" );  break;
        case Title:     caption = i18n( "Title" );  break;
        case Length:    caption = i18n( "Length" ); break;
        case DiscNumber:caption = i18n( "Disc Number" );  break;
        case Track:     caption = i18n( "Track" );  break;
        case Year:      caption = i18n( "Year" );   break;
        case Comment:   caption = i18n( "Comment" ); break;
        case Composer:  caption = i18n( "Composer" ); break;
        case Playcount: caption = i18n( "Playcount" ); break;
        case Score:     caption = i18n( "Score" );  break;
        case Rating:    caption = i18n( "Rating" ); break;
        case Filename:  caption = i18n( "Filename" ); break;
        case Firstplay: caption = i18n( "First Play" ); break;
        case Lastplay:  caption = i18n( "Last Play" ); break;
        case Modified:  caption = i18n( "Modified Date" ); break;
        case Bitrate:   caption = i18n( "Bitrate" ); break;
        case Filesize:  caption = i18n( "File Size" ); break;
        case BPM:       caption = i18n( "BPM" ); break;
        default: break;
    }
    return caption;
}


//////////////////////////////
// For iPod-style navigation
//////////////////////////////

/*
 * Overview
 * --------
 *
 * The iPod-style navigation is a (1, 2, or) 3-tier filtering process
 * (depending on the current "Group By" setting).  For concreteness
 * let's say the user is grouping by Genre, Artist, Album.  The first
 * screen he is presented with is a list of the genres, along with an
 * "All genres" option (unless there is only one genre).  He selects
 * one or more genres, and clicks the right arrow in the toolbar.  He
 * is then presented with a list of albums whose genre matches one of
 * the genres he has just chosen, unless he has chosen "All genres",
 * in which case all albums are shown.  This is repeated until he gets
 * to an actual track list.  If the user clicks the left arrow, he is
 * taken back to the previous screen, with his previous selection
 * still intact.
 *
 *
 * Interface
 * ---------
 *
 * The two main actions the user can perform are "browse forward" and
 * "browse backward", otherwise known as increment and decrement the
 * browse depth.  There is a small toolbar with these two buttons
 * located to the right of the time filter combobox, which is enabled
 * in iPod view mode.  If the user is not viewing tracks, there is
 * also a small browse forward button to the right of each entry on
 * his screen.  If the user is viewing tracks, the browse forward
 * action adds the currently selected tracks to the playlist.
 * Pressing the right or left keys is an alternate way of browsing
 * forward or backward.  At any point the user may drag and drop, press
 * return, or double-click to add the current selection to the playlist;
 * the logic for this is explained in a comment in listSelected().  If
 * divider mode is on, dividers are added as expected when not in track
 * mode.
 *
 *
 * Related methods
 * ---------------
 *
 * CollectionBrowser::ipodToolbar()
 *    -- (de)activate the toolbar with the browse buttons
 * CollectionView::keyPressEvent()
 *    -- handle the Left and Right keys, as well as the Up and Down keys
 *       to allow for wrapping around and skipping dividers
 * CollectionView::renderView()
 *    -- logic for applying the current filter and adding the matching
 *       items to the screen
 * CollectionView::ipodItemClicked()
 *    -- slot for activating the browse forward action when the right
 *       arrow on a listview item is clicked
 * CollectionView::setViewMode()
 *    -- enable/disable iPod toolbar
 * CollectionView::updateColumnHeader()
 * CollectionView::listSelected()
 *    -- apply current filters and current selection to get a track
 *       list, and order it correctly
 * CollectionView::allForCategory()
 *    -- return the text for the "All" item in the current category
 * CollectionView::incrementDepth()
 *    -- save the current selection away as a filter, and remember it
 *       in case the user browses back so we can reselect
 * CollectionView::decrementDepth()
 *    -- delete the current filter and reselect the saved selection
 * CollectionView::resetIpodDepth()
 *    -- reset the iPod view mode to the first screen
 * CollectionView::buildIpodQuery()
 *    -- adds query and sort criteria to a QueryBuilder, based on the
 *       saved filters
 * CollectionView::selectIpodItems()
 *    -- if we've just browsed forward, select the All item (or the
 *       unique item if there's only one).  If we've just browsed back,
 *       reselect the saved selection.
 * CollectionView::ipodIncrementIcon(), CollectionView::ipodDecrementIcon()
 *    -- returns a QPixmap of the small version of the right, resp. left
 *       arrow buttons
 * CollectionView::viewportResizeEvent()
 * CollectionItem::compare()
 *    -- in iPod mode the "All" item goes first
 *
 */


// Returns the text for the "All" filter option for the given category
// and the number of items in that category.
QString
CollectionView::allForCategory( const int cat, const int num ) const
{
    switch( cat )
    {
        // The singular forms shouldn't get used
        case IdAlbum:
        case IdVisYearAlbum:
            return i18n( "Album", "All %n Albums", num );
            break;
        case IdArtist:
            return i18n( "Artist", "All %n Artists", num );
            break;
        case IdComposer:
            return i18n( "Composer", "All %n Composers", num );
            break;
        case IdGenre:
            return i18n( "Genre", "All %n Genres", num );
            break;
        case IdYear:
            return i18n( "Year", "All %n Years", num );
            break;
        case IdLabel:
            return i18n( "Label", "All %n Labels", num );
            break;
    }

    return QString();
}

// This slot is called when the "browse right" action is activated,
// and also in listSelected().  It is responsible for saving the
// current selection in two ways: first, it saves the selection as
// a list of query match criteria in m_ipodFilters and m_ipodFilterYear,
// and second, it saves the currently selected items, the current item,
// and the screen position in m_ipodSelected, m_ipodCurrent, and
// m_ipodTopItem.
//   The reason there's a separate m_ipodFilterYear which is not an
// array of lists, is because if one of the categories is IdVisYearAlbum,
// we want to save the album filter separately from the year filter;
// since there's only one category that is IdVisYearAlbum, we don't need
// an array of years.
//   The main reason the cache data (m_ipodSelected, etc.) is separate
// from the filter data (m_ipodFilters, etc.), apart from the fact it's
// easier to code this way, is that the filter data must be deleted
// immediately upon browsing left, whereas the cache data must not.
//   If we're in track mode, this just inserts the currently selected
// tracks into the playlist.
void
CollectionView::incrementDepth( bool rerender /*= true*/ )
{
    if( m_viewMode != modeIpodView )
        return;
    if( selectedItems().count() == 0 )
        return;

    // Track mode?
    if( m_currentDepth == trackDepth() )
    {
        Playlist::instance()->insertMedia( listSelected(), Playlist::Unique | Playlist::Append );
        return;
    }

    m_parent->m_ipodDecrement->setEnabled( true );

    // We're not in track mode
    int catArr[3] = {m_cat1, m_cat2, m_cat3};
    int cat = catArr[m_currentDepth];

    // Clear filters and cache data at this level
    m_ipodFilters[m_currentDepth].clear();
    if( cat == IdVisYearAlbum )
        m_ipodFilterYear.clear();

    m_ipodSelected[m_currentDepth].clear();
    m_ipodCurrent[m_currentDepth] = QString::null;
    m_ipodTopItem[m_currentDepth] = QString::null;

    // Save the current item
    if( currentItem() )
        m_ipodCurrent[m_currentDepth] = currentItem()->text( 0 );

    //cache viewport's top item
    QListViewItem* item = itemAt( QPoint(0, 0) );
    if ( item )
        m_ipodTopItem[m_currentDepth] = item->text( 0 );

    // Figure out the next filter, and save the current selection
    QPtrList<QListViewItem> selected = selectedItems();
    QPtrList<QListViewItem>::iterator it = selected.begin();

    while( it != selected.end() )
    {
        CollectionItem *item = dynamic_cast<CollectionItem*>( *it );
        ++it;
        if( item == 0 )
            continue;

        // No filter if "All" is selected
        if( item->isSampler() )
        {
            m_ipodFilters[m_currentDepth].clear();
            if( cat == IdVisYearAlbum )
                m_ipodFilterYear.clear();

            // If "All" is selected then don't bother saving this
            // selection, since All will then be reselected automatically
            // in selectIpodItems()
            m_ipodSelected[m_currentDepth].clear();
            m_ipodCurrent[m_currentDepth] = QString::null;

            break;
        }

        if( cat == IdVisYearAlbum )
        {
            QString tmptext = item->text( 0 );
            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
            yearAlbumCalc( year, tmptext );
            if( !item->isUnknown() )
                m_ipodFilters[m_currentDepth] << tmptext;
            else
                m_ipodFilters[m_currentDepth] << "";
            m_ipodFilterYear << year; // May be ""
        }

        else
            m_ipodFilters[m_currentDepth] << item->getSQLText( 0 );

        // Save the selection
        m_ipodSelected[m_currentDepth] << item->text( 0 );
    }

    m_currentDepth++;

    if( rerender )
    {
        updateColumnHeader();
        m_ipodIncremented = 1;
        renderView( true );
    }
}


// This slot is basically responsible for undoing whatever
// incrementDepth did.  Namely, it deletes the filter at
// the previous level; selectIpodItems() will then be called
// from renderView() to reselect the remembered selection.
void
CollectionView::decrementDepth ( bool rerender /*= true*/ )
{
    if( m_viewMode != modeIpodView )
        return;
    if( m_currentDepth <= 0 )
        return;

    m_currentDepth--;
    m_parent->m_ipodDecrement->setEnabled( m_currentDepth > 0 );
    m_ipodFilters[m_currentDepth].clear();
    int catArr[3] = {m_cat1, m_cat2, m_cat3};
    int cat = catArr[m_currentDepth];
    if( cat == IdVisYearAlbum )
        m_ipodFilterYear.clear();

    // Clear the selection on higher levels
    for( int i = m_currentDepth + 1; i < 3; ++i )
    {
        m_ipodSelected[i].clear();
        m_ipodCurrent[i] = QString::null;
        m_ipodTopItem[i] = QString::null;
    }

    if( rerender )
    {
        m_ipodIncremented = 2;
        updateColumnHeader();
        renderView( true );
    }
}


// This resets the ipod view mode to the first screen.
// Call updateColumnHeader() as well whenever you run this
void
CollectionView::resetIpodDepth ( void )
{
    m_currentDepth = 0;
    m_ipodFilterYear.clear();
    m_ipodFilters[0].clear();
    m_ipodFilters[1].clear();
    m_ipodFilters[2].clear();
    m_ipodIncremented = 1;
    m_parent->m_ipodDecrement->setEnabled( false );
}



// This method is the querying workhorse for the iPod browsing code.
// It is used both to populate the content browser (in renderView())
// and to generate track lists (in listSelected()).  This method only
// runs qb.addMatch() and qb.sortBy() (as well as one qb.addFilter()
// below); the caller should run qb.setGoogleFilter(),
// qb.addReturnValue(), etc.
//   The sorting is as follows: if recursiveSort is true (for
// listSelected()), then it sorts first by m_cat1, then by m_cat2,
// then m_cat3, then track; if recursiveSort is false (for
// renderView()), it only sorts by the category at m_currentDepth.
// Tracks are sorted by track number first if either of the two
// following conditions hold:
// (i) recursiveSort is true and one of the categories is (Year +) Album
// (ii) recursiveSort is false and only one album is selected
// This most closely mimics the behavior of the list view, as well as
// an actual iPod.
//   The compilationsOnly argument does *not* set the onlyCompilations
// option (setting options is up to the caller); all it does is disable
// sorting by artist if recursiveSort is on.  This is because when doing
// a compilations-only search, all tracks should behave as if the artist
// were Various Artists for sorting purposes.
void
CollectionView::buildIpodQuery ( QueryBuilder &qb, int depth, QStringList filters[3],
        QStringList filterYear, bool recursiveSort /*= false*/, bool compilationsOnly /*= false*/)
{
    int catArr[3] = {m_cat1, m_cat2, m_cat3};
    int q_cat;
    bool stillFiltering = (depth < trackDepth());
    bool SortbyTrackFirst = false;

    // First apply the filters from previous screens
    for( int i = 0; i < depth; ++i )
    {
        q_cat = catArr[i];

        if( q_cat == IdVisYearAlbum )
        {
            q_cat = IdAlbum;

            if( filters[i].count() > 0 )
            {
                // This is very annoying -- we have to do an OR of queries
                // of the form (album = ? AND year = ??)
                QStringList::iterator album = filters[i].begin();
                QStringList::iterator year  = filterYear.begin();

                qb.beginOR();

                while( album != filters[i].end() )
                {
                    qb.beginAND();
                    qb.addMatch( QueryBuilder::tabAlbum, *album, false, true );
                    qb.addMatch( QueryBuilder::tabYear,  *year,  false, true );
                    qb.endAND();

                    ++album;
                    ++year;
                }

                qb.endOR();
            }

            if( recursiveSort )
                qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );

        }

        else
        {
            if( filters[i].count() > 0 )
                qb.addMatches( q_cat, filters[i], false, true );
        }

        // Don't sort by artist if we're getting compilations
        if( recursiveSort
                && !(compilationsOnly && q_cat == IdArtist) )
            qb.sortBy( q_cat, QueryBuilder::valName );

        // Sort by track first subject to the conditions described above
        if( q_cat == IdAlbum  &&
                (filters[i].count() == 1 || recursiveSort) )
            SortbyTrackFirst = true;
    }


    // Now add the non-recursive sort-by
    if( stillFiltering )   // Are we showing a category?
    {
        q_cat = catArr[depth];
        if( q_cat == IdVisYearAlbum )
        {
            q_cat = IdAlbum;
            qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
        }

        qb.sortBy( q_cat, QueryBuilder::valName );

        // ensure we don't get empty genres/albums/etc due to tag changes
        qb.addFilter( QueryBuilder::tabSong, QString::null );

    }

    // ... or are we showing tracks?
    else
    {
        if ( SortbyTrackFirst ) {
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
        }
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
        if ( !SortbyTrackFirst ) {
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
        }
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );
    }

}


// This method is responsible for either selecting the "All" item
// if the iPod depth has been incremented, or selecting the previously
// remembered items if the depth has been decremented, depending on
// m_ipodIncremented, m_ipodSelected, m_ipodCurrent, and m_ipodTopItem.
// Note that if the previously selected items have disappeared (due to
// a GoogleFilter being applied, e.g.) then we select the "All" item by
// default.
//   Note that if there is only one item in the list then there is no
// All option, so we select the unique item.
void
CollectionView::selectIpodItems ( void )
{
    if( m_viewMode != modeIpodView ||
            m_ipodIncremented == 0 )
    {
        m_ipodIncremented = 0;
        return;
    }

    // If we've just decremented the iPod view depth then remember
    // the selection and the current item last time we incremented.
    // Note that a filter or something may have happened between then
    // and now, so we should allow for those items no longer being
    // here (in which case we pass through to the code below)
    if( m_ipodIncremented == 2 )
    {
        // Pedantry -- presumably we're not at track depth!
        if( m_currentDepth == trackDepth() )
        {
            m_ipodIncremented = 0;
            return;
        }

        // If there's no selection or the selected items have
        // disappeared, pass through to the code below
        if( m_ipodSelected[m_currentDepth].count() == 0 )
            m_ipodIncremented = 1;

        else
        {
            KListView::selectAll( false );
            int selected = 0;
            QStringList::iterator it = m_ipodSelected[m_currentDepth].begin();
            while( it != m_ipodSelected[m_currentDepth].end() )
            {
                QListViewItem *item = findItem( *it, 0 );
                ++it;

                if( !item )
                    continue;

                selected++;
                // If the saved currentItem has disappeared, it's more
                // intuitive if the last selected item is current.
                setCurrentItem( item );
                item->setSelected( true );
                setSelectionAnchor( item );
            }

            // Pass through to below
            if( selected == 0 )
                m_ipodIncremented = 1;

            else
            {
                // Remember the current item and scroll position
                if( !m_ipodTopItem[m_currentDepth].isEmpty() &&
                        !m_ipodTopItem[m_currentDepth].isNull() )
                {
                    //scroll to previous viewport top item
                    QListViewItem* item = findItem( m_ipodTopItem[m_currentDepth], 0 );
                    if ( item )
                        setContentsPos( 0, itemPos( item ) );
                }

                if( !m_ipodCurrent[m_currentDepth].isEmpty() &&
                        !m_ipodCurrent[m_currentDepth].isNull() )
                {
                    QListViewItem *item = findItem( m_ipodCurrent[m_currentDepth], 0);
                    if( item )
                        setCurrentItem( item );
                }
            }
        }
    }

    // If we've just incremented the iPod view depth (or are displaying
    // the iPod window for the first time) then automatically select the
    // All option (or the only element of the list) for keyboard
    // navigation
    if( m_ipodIncremented == 1 )
    {
        KListView::selectAll( false );
        QListViewItem *item = firstChild();

        // There will be a divider in the first slot if there is only
        // one item in the list and m_showDivider is on
        while( item && dynamic_cast<DividerItem*>( item ) )
            item = item->itemBelow();

        if( item )
        {
            setCurrentItem( item );
            item->setSelected( true );
            setSelectionAnchor( item );
            setContentsPos( 0, itemPos( item ) );
        }
    }

    m_ipodIncremented = 0;
}


// Convenience methods for returning the correct (small version of)
// the browse forward / backward buttons

QPixmap
CollectionView::ipodIncrementIcon ( void )
{
    return SmallIcon( Amarok::icon( "fastforward" ) );
}

QPixmap
CollectionView::ipodDecrementIcon ( void )
{
    return SmallIcon( Amarok::icon( "rewind" ) );
}

void
CollectionView::setCompilation( const KURL::List &urls, bool compilation )
{
    //visual feedback
    QApplication::setOverrideCursor( KCursor::waitCursor() );

    //Set it in the DB. We don't need to update the view now as we do it at the end.
    CollectionDB::instance()->setCompilation( urls, compilation, false );

    foreachType( KURL::List, urls ) {
        if ( !TagLib::File::isWritable( QFile::encodeName( ( *it ).path() ) ) )
            continue;

        MetaBundle mb( *it );

        mb.setCompilation( compilation ? MetaBundle::CompilationYes : MetaBundle::CompilationNo );

        if( mb.save() ) {
            mb.updateFilesize();
            //update the collection db, since filesize might have changed
            CollectionDB::instance()->updateTags( mb.url().path(), mb, false );
        }
    }
    //visual feedback
    QApplication::restoreOverrideCursor();
    if ( !urls.isEmpty() ) renderView(true);
}

void
CollectionView::cacheView()
{
    //free cache
    m_cacheOpenItemPaths.clear();

    //Store the current item
    m_cacheCurrentItem = makeStructuredNameList( currentItem() );

    //cache expanded/open items
    if ( m_viewMode == modeTreeView ) {
        QListViewItemIterator it( this );
        while ( it.current() ) {
            QListViewItem *item = it.current();
            if ( item->isOpen() ) {
                //construct path to item
                QStringList itemPath;
                for( const QListViewItem *i = item; i; i = i->parent() )
                    itemPath.prepend( i->text( 0 ) );

                m_cacheOpenItemPaths.append ( itemPath );
            }
            ++it;
        }
    }

    //cache viewport's top item
    m_cacheViewportTopItem = makeStructuredNameList( itemAt( QPoint(0, 0) ) );
}


void
CollectionView::restoreView()
{
    //expand cached items
    if ( m_viewMode == modeTreeView ) {
        QValueList<QStringList>::const_iterator it;
        for ( it = m_cacheOpenItemPaths.begin(); it != m_cacheOpenItemPaths.end(); ++it ) {
            QListViewItem* item = findItem( (*it)[0], 0 );
            if ( item )
                item->setOpen ( true );

            for ( uint i = 1; i < (*it).count() && item; ++i ) {
                item = item->firstChild();
                while ( item ) {
                    if ( item->text(0) == (*it)[ i ] )
                        item->setOpen ( true );
                    item = item->nextSibling();
                }
            }
        }
    }

    //scroll to previous viewport top item
    QListViewItem* item = findFromStructuredNameList( m_cacheViewportTopItem );
    if ( item )
        setContentsPos( 0, itemPos(item) );

    //Restore a selected item (all levels of the tree stored to fully specify which item)
    item = findFromStructuredNameList( m_cacheCurrentItem );
    if ( item )
    {
        setCurrentItem( item );
        item->setSelected( true );
        // More intuitive if shift-click selects from current selection
        setSelectionAnchor( item );
    }

    //free cache
    m_cacheOpenItemPaths.clear();
    m_cacheViewportTopItem = QStringList();
    m_cacheCurrentItem = QStringList();
}

QStringList
CollectionView::makeStructuredNameList( QListViewItem *item ) const
{
    QStringList nameList;
    for ( QListViewItem *current = item; current; current = current->parent() )
        nameList.push_front( current->text( 0 ) );
    return nameList;
}

QListViewItem*
CollectionView::findFromStructuredNameList( const QStringList &nameList ) const
{
    QListViewItem *item( firstChild() );
    bool firstTime = true;
    foreach( nameList )
    {
        if ( !firstTime )
            item = item->firstChild();
        else
            firstTime = false;

        while ( item && item->text( 0 ) != *it )
            item = item->nextSibling();

        if ( !item )
        {
            debug() << "Could not find expected element to select: " << nameList << endl;
            break;
        }
    }
    return nameList.isEmpty() ? 0 : item;
}


// Small function aimed to convert Eagles, The -> The Eagles (and back again)
// TODO Internationlise
void
CollectionView::manipulateThe( QString &str, bool reverse )
{
    if( reverse )
    {
        QString begin = str.left( 3 );
        str = str.append( ", %1" ).arg( begin );
        str = str.mid( 4 );
        return;
    }

    if( !endsInThe( str ) )
        return;

    QString end = str.right( 3 );
    str = str.prepend( "%1 " ).arg( end );

    uint newLen = str.length() - end.length() - 2;

    str.truncate( newLen );
}

bool
CollectionView::endsInThe( const QString & text )
{
    return text.endsWith( ", the", false );
}

// avoid code duplication
void
CollectionView::yearAlbumCalc( QString &year, QString &text )
{
    if( year == "\?" )
        year = "";

    text = text.right( text.length() -
                       text.find( i18n(" - ") ) -
                       i18n(" - ").length() );
}

void
CollectionView::viewportPaintEvent( QPaintEvent *e )
{
    KListView::viewportPaintEvent( e );

    // Superimpose bubble help for Flat-View mode:

    if ( m_viewMode == modeFlatView && childCount() == 0 )
    {
        QPainter p( viewport() );

        QSimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>Flat-View Mode</h3>"
                    "To enable the Flat-View mode, please enter search terms in the search line above."
                "</div>" ), QApplication::font() );

        t.setWidth( width() - 50 );

        const uint w = t.width() + 20;
        const uint h = t.height() + 20;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );
        t.draw( &p, 20, 20, QRect(), colorGroup() );
    }
}


void
CollectionView::updateTrackDepth() {
    bool m3 = (m_cat3 == IdNone);
    bool m2 = (m_cat2 == IdNone);
    bool m1 = (m_cat1 == IdNone);
    if ( m3 || m2 || m1) {
        //The wanted depth, is the lowest IdNone
        if (m3)
            m_trackDepth = 2;
        if (m2)
            m_trackDepth = 1;
        if (m1)
            m_trackDepth = 0;
    }
    else // If there's no IdNone, then it's 3
        m_trackDepth = 3;
}

void
CollectionView::viewportResizeEvent( QResizeEvent* e)
{
    if( m_viewMode != modeIpodView )
    {
        header()->blockSignals( true );

        const double width = e->size().width();
        int visibleColumns = 0;
        for ( int i = 0; i < columns(); ++i )
            if ( columnWidth( i ) != 0 )
                visibleColumns ++;
        int correct = e->size().width() - (e->size().width() / visibleColumns) * visibleColumns;

        if( m_viewMode == modeFlatView )
            m_flatColumnWidths.clear();

        if ( visibleColumns != 0 ) {
            for ( int c = 0; c < columns(); ++c ) {
                int w = columnWidth( c ) ? static_cast<int>( width/visibleColumns ) : 0;
                if ( w > 0 )
                {
                    w += correct;
                    correct = 0;
                    setColumnWidth( c, w );
                }
                if( m_viewMode == modeFlatView )
                    m_flatColumnWidths.push_back( w );
            }
        }

        header()->blockSignals( false );
    }

    // iPod-mode header adjustment code
    else
    {
        // Don't use header()->adjustHeaderSize(), since that doesn't
        // do a very good job.  Instead we treat the browse-forward-button
        // column as rigid, and stretch the text column to exactly fit
        // the width.

        int width     = visibleWidth();
        int col1width = 0;
        // No column 1 for tracks
        if( m_currentDepth != trackDepth() )
            col1width = columnWidth(1);
        setColumnWidth( 0, width - col1width );
    }

    // Needed for correct redraw of bubble help
    triggerUpdate();
}

bool
CollectionView::eventFilter( QObject* o, QEvent* e )
{
    if( o == header()
        && e->type() == QEvent::MouseButtonPress
        && static_cast<QMouseEvent*>( e )->button() == Qt::RightButton
        && m_viewMode == modeFlatView )
    {
        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertTitle( i18n( "Flat View Columns" ), /*id*/ -1, /*index*/ 1 );

        for ( int i = 0; i < columns(); ++i )
        {
            popup.insertItem( captionForTag( static_cast<Tag>( i ) ), i );
            popup.setItemChecked( i, ( columnWidth(i) != 0 ) );
        }

        //title column should always be shown
        popup.setItemEnabled( Title, false );
        popup.setItemVisible( Score, AmarokConfig::useScores() );
        popup.setItemVisible( Rating, AmarokConfig::useRatings() );

        const int returnID = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        if ( returnID != -1 )
        {
            if ( columnWidth( returnID ) == 0 ) {
                adjustColumn( returnID );   // show column
                header()->setResizeEnabled( true, returnID );
                renderView(true);
                }
            else {
                setColumnWidth ( returnID, 0 ); // hide column
                header()->setResizeEnabled( false, returnID );
            }
            //manage column widths
            QResizeEvent rev ( size(), QSize() );
            viewportResizeEvent( &rev );
        }

        m_flatColumnWidths.clear();
        for ( int c = 0; c < columns(); ++c )
            m_flatColumnWidths.push_back( columnWidth( c ) );

        return true;
    }

    return KListView::eventFilter( o, e );
}

uint CollectionView::translateTimeFilter( uint filterMode )
{
    uint filterSecs = 0;
    switch ( filterMode )
    {
        case 1:
            // added today
            filterSecs = 60 * 60 * 24;
            break;

        case 2:
            // added within one week
            filterSecs = 60 * 60 * 24 * 7;
            break;

        case 3:
            // added within one month
            filterSecs = 60 * 60 * 24 * 30;
            break;

        case 4:
            // added within three months
            filterSecs = 60 * 60 * 24 * 91;
            break;

        case 5:
            // added within one year
            filterSecs = 60 * 60 * 24 * 365;
            break;
    }

    return filterSecs;
}

void
CollectionView::renderFlatModeView( bool /*=false*/ )
{
    QStringList values;
    QueryBuilder qb;

    if ( translateTimeFilter( timeFilter() ) > 0 )
        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

    if ( translateTimeFilter( timeFilter() ) <= 0
            && (m_filter.length() < 3 || (!m_filter.contains( " " ) && m_filter.endsWith( ":" ) ) ) )
    {
        // Redraw bubble help
        triggerUpdate();
        return;
    }

    QValueList<Tag> visibleColumns;
    for ( int c = 0; c < columns(); ++c )
        if ( columnWidth( c ) != 0 )
        {
            visibleColumns.append( static_cast<Tag>( c ) );
        }

    //always fetch URL
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    //device automatically added

    int filterTables = 0;
    for ( QValueList<Tag>::ConstIterator it = visibleColumns.constBegin(); it != visibleColumns.constEnd(); ++it )
    {
        switch ( *it )
        {
                case Artist:
                    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabArtist;
                    break;
                case Composer:
                    qb.addReturnValue ( QueryBuilder::tabComposer, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabComposer;
                    break;
                case Album:
                    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabAlbum;
                    break;
                case Genre:
                    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabGenre;
                    break;
                case Title:
                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle, true );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                case Length:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valLength );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                case DiscNumber:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                case Track:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valTrack );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                case Year:
                    qb.addReturnValue ( QueryBuilder::tabYear, QueryBuilder::valName );
                    filterTables |= QueryBuilder::tabYear;
                    break;
                case Comment:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valComment );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                case Playcount:
                    qb.addReturnValue ( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
                    break;
                case Score:
                    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
                    break;
                case Rating:
                    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
                    break;
                case Filename:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valRelativePath );
                    break;
                case Firstplay:
                    qb.addReturnValue ( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
                    break;
                case Lastplay:
                    qb.addReturnValue ( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
                    break;
                case Modified:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valCreateDate );
                    break;
                case Bitrate:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valBitrate );
                    break;
                case Filesize:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valFilesize );
                    break;
                case BPM:
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valBPM );
                    filterTables |= QueryBuilder::tabSong;
                    break;
                default:
                    qb.addReturnValue( QueryBuilder::tabDummy, QueryBuilder::valDummy );
                    break;
        }
    }

    qb.setGoogleFilter( filterTables, m_filter );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );

    //we leftjoin the query so it can return mysql NULL cells, i.e. for score and playcount
    //this is an ugly hack - should be integrated in querybuilder itself instead.
    QString leftQuery = qb.query();
    leftQuery.replace( "INNER JOIN", "LEFT JOIN" );
    values = CollectionDB::instance()->query( leftQuery );

    //construct items
    QStringList::ConstIterator it = values.constBegin();
    QStringList::ConstIterator end = values.constEnd();
    while ( it != end )
    {
        CollectionItem* item = new CollectionItem( this );
        item->setDragEnabled( true );
        item->setDropEnabled( false );
        QString rpath = *it;
        item->setUrl( MountPointManager::instance()->getAbsolutePath( (*++it).toInt(), rpath ) );
        ++it;

        for ( QValueList<Tag>::ConstIterator it_c = visibleColumns.constBegin(); it_c != visibleColumns.constEnd(); ++it_c )
        {
            switch ( *it_c )
            {
                    case Length:
                        item->setText( *it_c, MetaBundle::prettyLength( (*it).toInt(), false) );
                        break;
                    case Bitrate:
                        item->setText( *it_c, MetaBundle::prettyBitrate( (*it).toInt() ) );
                        break;
                    case Firstplay:
                    case Lastplay:
                    case Modified:
                    {
                        QDateTime time = QDateTime();
                        time.setTime_t( (*it).toUInt() );
                        item->setText( *it_c, time.date().toString( Qt::LocalDate ) );
                        break;
                    }
                    case Playcount:
                    case Score:
                        item->setText( *it_c, (*it).isNull() ? "0" : (*it) );
                        break;
                    case Rating:
                        item->setText( *it_c, (*it).isNull() ? "0" : (*it) );
                        break;
                    case Filename:
                        item->setText( *it_c, KURL::fromPathOrURL( (*it).right( (*it).length() -1 ) ).filename() );
                        break;
                    case Filesize:
                        item->setText( *it_c, MetaBundle::prettyFilesize( (*it).toInt() ) );
                        break;
                    default:
                        item->setText( *it_c, (*it) );
                        break;
            }
            ++it;
        }
    }
}

void
CollectionView::renderTreeModeView( bool /*=false*/ )
{
    QStringList values;
    QueryBuilder qb;

    if ( translateTimeFilter( timeFilter() ) > 0 )
        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

    setSorting( 0 );
    int VisYearAlbum = -1;
    int VisLabel = -1;
    int q_cat1=m_cat1;
    int q_cat2=m_cat2;
    int q_cat3=m_cat3;
    if( m_cat1 == IdVisYearAlbum ||
            m_cat2 == IdVisYearAlbum ||
            m_cat3 == IdVisYearAlbum )
    {
        if( m_cat1==IdVisYearAlbum )
        {
            VisYearAlbum = 1;
            q_cat1 = IdAlbum;
        }
        if( m_cat2==IdVisYearAlbum )
        {
            VisYearAlbum = 2;
            q_cat2 = IdAlbum;
        }
        if( m_cat3==IdVisYearAlbum )
        {
            VisYearAlbum = 3;
            q_cat3 = IdAlbum;
        }
    }
    if ( m_cat1 == IdLabel ||
            m_cat2 == IdLabel ||
            m_cat3 == IdLabel )
    {
        if ( m_cat1 == IdLabel )
            VisLabel = 1;
        else if ( m_cat2 == IdLabel )
            VisLabel = 2;
        else
            VisLabel = 3;
    }
    QPixmap pixmap = iconForCategory( m_cat1 );

    qb.addReturnValue( q_cat1, QueryBuilder::valName, true );

    if( VisYearAlbum == 1 )
        qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName, true );

    qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );

    if( VisYearAlbum == 1 )
        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );

    qb.sortBy( q_cat1, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );

    if( q_cat1 == QueryBuilder::tabArtist )
        qb.setOptions( QueryBuilder::optNoCompilations );

    // ensure we don't get empty genres/albums/etc due to tag changes
    qb.addFilter( QueryBuilder::tabSong, QString::null );

    values = qb.run();

    //add items to the view

    uint dividerCount = 0;
    if( values.count() )
    {
        //keep track of headers already added
        QMap<QString, bool> containedDivider;

        for ( QStringList::Iterator it = values.fromLast(), begin = values.begin(); true; --it )
        {
            bool unknown = false;

            //For Year-Album
            if ( VisYearAlbum == 1 )
            {
                ( *it ) = ( *it ).isEmpty() ? "?" : ( *it ) + i18n(  " - " );
                QStringList::Iterator album = it;
                --album;
                if ( (*album).isEmpty() )
                {
                    unknown = true;
                    ( *it ) += i18n( "Unknown" );
                }
                else
                    ( *it ) += *album;
            }

            if ( (*it).stripWhiteSpace().isEmpty() )
            {
                if ( VisLabel == 1 )
                    (*it) = i18n( "No Label" );
                else
                    (*it) = i18n( "Unknown" );
                unknown = true;
            }
            else if (m_showDivider)
            {
                //Dividers for "The Who" should be created as "W", not "T", because
                //that's how we sort it
                QString actualStr = *it;
                if ( m_cat1 == IdArtist && actualStr.startsWith( "the ", false ) )
                    manipulateThe( actualStr, true );

                QString headerStr = DividerItem::createGroup( actualStr, m_cat1);

                if ( !containedDivider[headerStr] && !headerStr.isEmpty() )
                {
                    containedDivider[headerStr] = true;
                    (void)new DividerItem(this, headerStr, m_cat1/*, m_sortYearsInverted*/);
                    dividerCount++;
                }
            }

            CollectionItem* item = new CollectionItem( this, m_cat1, unknown );
            item->setExpandable( true );
            item->setDragEnabled( true );
            item->setDropEnabled( false );
            item->setText( 0, *it );
            item->setPixmap( 0, pixmap );

            //The structure of the return is different if Year - Album is format
            if ( VisYearAlbum == 1 )
                --it;

            if ( it == begin )
                break;
        }
    }

    //check if we need to add a Various Artists node
    if ( q_cat1 == QueryBuilder::tabArtist )
    {
        qb.clear();
        if ( translateTimeFilter( timeFilter() ) > 0 )
            qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate,
                          QString().setNum( QDateTime::currentDateTime().toTime_t() -
                                            translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

        qb.addReturnValue( q_cat1, QueryBuilder::valName, true );
        qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );
        qb.setOptions( QueryBuilder::optOnlyCompilations | QueryBuilder::optRemoveDuplicates );
        qb.setLimit( 0, 1 );
        values = qb.run();

        if ( values.count() )
        {
            //                 KListViewItem* x = new DividerItem(this, i18n( "Various" ), m_cat1);
            //                 x->setExpandable(false);
            //                 x->setDropEnabled( false );
            //                 x->setSelectable(false);
            CollectionItem* item = new CollectionItem( this, m_cat1, false, true );
            item->setExpandable( true );
            item->setDragEnabled( true );
            item->setDropEnabled( false );
            item->setText( 0, i18n( "Various Artists" ) );
            item->setPixmap( 0, pixmap );
        }
    }

    //Algorithm to expand some items after a search in a pretty/useful way:
    //Aim to expand all items with one child, but with a maximum limit to how many items
    //should be shown in the listview ( maxRows) after which we give up. If an item has
    //more than one child and we haven't reached the limit, we may end up expanding it
    //later.
    QValueList<QListViewItem*> couldOpen;
    int totalCount = childCount() - dividerCount;
    const int maxRows = 20; //This seems like a fair limit for a 1024x768 screen
    if ( totalCount < maxRows )
    {
        //Generate initial list of top list items to look at
        for ( QListViewItem* top = firstChild(); top; top = top->nextSibling() )
        {
            if ( !dynamic_cast<CollectionItem*>( top ) )
                continue;
            couldOpen.push_back( top );
        }
        //Expand suggested items and expand or enqueue their children until we run out of
        //rows or have expanded everything
        for ( QValueList<QListViewItem*>::iterator it = couldOpen.begin(); it != couldOpen.end() && totalCount < maxRows; ++it )
        {
            if ( !( *it )->isOpen() )
               ( *it )->setOpen( true );
            totalCount += ( *it )->childCount();
            if ( ( *it )->firstChild()->isExpandable() )    //Check we have not reached the bottom
            {
                for ( QListViewItem *j = ( *it )->firstChild(); j && totalCount < maxRows; j = j->nextSibling() )
                {
                    j->setOpen( true );
                    if ( j->childCount() > 1 )  //More than one child - maybe later
                    {
                        j->setOpen( false );
                        couldOpen.push_back( j );
                    }
                    else
                    {
                        //Prioritize expanding its children - add it immediately next
                        QValueList<QListViewItem*>::iterator next = it;
                        ++next;
                        couldOpen.insert( next, j );
                        ++totalCount;
                    }
                }
            }
        }
    }

    //If the tree has only one branch, at least at the top, make the lowest child
    //which has no siblings (other branches) become selected.
    //Rationale: If you search for something and then clear the search bar, this way it
    //will stay in view, assuming you only had one real result.
    if ( childCount() - dividerCount == 1 )
    {
        QListViewItem *item = firstChild();
        if ( dynamic_cast<DividerItem*>( item ) ) //Skip a divider, if present
            item = item->nextSibling();
        for ( ; item ; item = item->firstChild() )
            if ( !item->isOpen() || item->childCount() > 1 )
                break;
        if ( item )
        {
            setCurrentItem( item );
            item->setSelected( true );
            setSelectionAnchor( item );
        }
    }

    removeDuplicatedHeaders();
}

void
CollectionView::removeDuplicatedHeaders()
{
    /* Following code depends on the order! */
    sort();

    QValueList<DividerItem *> toDelete;
    DividerItem *current=0, *last=0;
    bool empty;
    QListViewItem *item;
    /* If we have two consecutive headers, one of them is useless, and should be removed */
    for( item = firstChild(),empty=false; item; item=item->nextSibling() )
    {
        if ( (current = dynamic_cast<DividerItem *>( item )) )
        {
            if ( empty )
            {
                if ( !current->text(0).at(0).isLetterOrNumber()
                        || ( last->text(0).at(0).isLetterOrNumber()
                             && current->text(0).at(0).unicode() > last->text(0).at(0).unicode() ) )
                    toDelete += current;
                else
                {
                    toDelete += last;
                    last=current;
                }
            }
            else
                last=current;
            empty=true;
        }
        else
            empty=false;
    }

    for ( QValueList<DividerItem *>::iterator it = toDelete.begin(); it != toDelete.end(); ++it )
        delete *it;
}


// MODE IPODVIEW This is the heart of the iPod view mode code.  It
// applies the current filters (as defined by previous "move
// forward" actions).  If we're not viewing tracks (stillFiltering
// == true), then display the results in the standard order, with
// dividers if applicable, with an "All" (i.e., no filter) item if
// there is more than one result, and with "Unknown" items if
// there are any.  Note that the "All" item is a CollectionItem
// with the Sampler bit set, since it behaves similar to the
// Various Artists node.
//   If we are viewing tracks (stillFiltering ==
// false), then just apply all of the filters and show the
// selected tracks.  The track ordering is similar to in list view
// mode; see the comments in buildIpodQuery() for details.
void
CollectionView::renderIpodModeView( bool /*=false*/ )
{
    QStringList values;
    QueryBuilder qb;

    if ( translateTimeFilter( timeFilter() ) > 0 )
        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

    int catArr[3] = {m_cat1, m_cat2, m_cat3};
    // stillFiltering is true when we're not viewing tracks
    bool stillFiltering = (m_currentDepth < trackDepth());
    // q_cat is the "query category" -- it's undefined if
    // stillFiltering is true; otherwise it's the category
    // at the current iPod viewing depth (m_currentDepth), unless
    // that category is IdVisYearAlbum, in which case it's IdAlbum.
    int q_cat = (stillFiltering ? catArr[m_currentDepth] : catArr[0]);
    // m_cat is the current actual category -- it agrees with q_cat
    // if and only if m_cat != IdVisYearAlbum
    int m_cat = q_cat;
    // This is set to true if the current category is IdVisYearAlbum
    // It is only used when stillFiltering == true.
    bool VisYearAlbum = false;
    //This is set to true if the current category is IdLabel
    bool VisLabel = false;

    if( q_cat == IdVisYearAlbum && stillFiltering )
    {
        VisYearAlbum = true;
        q_cat = IdAlbum;
    }
    if ( q_cat == IdLabel && stillFiltering )
        VisLabel = true;

    // If we're viewing tracks, we don't want them to be sorted
    // alphabetically, since we take great pains in
    // buildIpodQuery() to have them returned to us from the
    // database in the correct order.
    setSorting( stillFiltering ? 0 : -1 );

    // Do the grunt work of building the query (this method is also
    // called by listSelected() )
    buildIpodQuery( qb, m_currentDepth, m_ipodFilters, m_ipodFilterYear );
    if(stillFiltering)
        qb.setOptions( QueryBuilder::optRemoveDuplicates );

    int tables = 0;
    for( int i = 0; i < trackDepth(); ++i )
        tables |= (catArr[i] == IdVisYearAlbum
                ? IdAlbum
                : catArr[i]);
    qb.setGoogleFilter( tables | QueryBuilder::tabSong, m_filter );

    // Return values
    if( stillFiltering )
    {
        qb.addReturnValue( q_cat, QueryBuilder::valName, true );
        if( VisYearAlbum )
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName, true );
    }
    else
    {
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    }

    values = qb.run();
    int total = values.count() / qb.countReturnValues();

    // This can happen -- with the filter it might be empty
    if( total == 0 )
        return;

    // We want to load the pixmap only once if we're still filtering
    // Otherwise just load a dummy pixmap
    QPixmap pixmap = iconForCategory( q_cat );
    QPixmap incPixmap = ipodIncrementIcon();
    int width = incPixmap.width() + 10;  // Set the column width below
    // Keep track of the dividers we've created.
    QMap<QString, bool> containedDivider;

    QStringList::Iterator itStep = values.end();
    QStringList::Iterator begin = values.begin();
    itStep -= qb.countReturnValues();
    // It's an awkward business stepping through a list backward
    // when the elements are in tuples, going forward.
    // This loop breaks at the bottom -- don't put a continue in here!
    while( 1 )
    {
        CollectionItem* item;
        QStringList::Iterator it = itStep;

        // Add non-track items
        if( stillFiltering )
        {
            bool unknown = false;

            if( (*it).isEmpty() )
            {
                unknown = true;
                if ( VisLabel )
                    *it = i18n( "No Label" );
                else
                    *it = i18n( "Unknown" );
            }

            item = new CollectionItem( this, m_cat, unknown );

            if( VisYearAlbum )
            {
                QString album = *it;
                QString year = *(++it);
                if( year.isEmpty() )
                    year = "?";
                item->setText( 0, year + i18n(" - ") + album );
            }
            else
                item->setText( 0, *it );

            item->setPixmap( 0, pixmap );
            item->setPixmap( 1, incPixmap );
            item->setText( 1, "" );
            // Calculate width
            width = item->width( fontMetrics(), this, 1 );

            // Only do dividers if we're not showing tracks since
            // dividers don't really make sense in a track-only view
            if( !unknown && m_showDivider )
            {
                //Dividers for "The Who" should be created as "W", not "T",
                //because that's how we sort it
                QString actualStr = item->text( 0 );
                if ( m_cat == IdArtist &&
                        actualStr.startsWith( "the ", false ) )
                    manipulateThe( actualStr, true );

                QString headerStr = DividerItem::createGroup( actualStr, m_cat );

                if ( !containedDivider[headerStr] && !headerStr.isEmpty() )
                {
                    containedDivider[headerStr] = true;
                    (void)new DividerItem( this, headerStr, m_cat );
                }
            }
        }

        else // Add tracks
        {
            item = new CollectionItem( this );
            item->setUrl( *it );
            item->setText( 0, *(++it) );
        }

        item->setDragEnabled( true );
        item->setDropEnabled( false );

        if( itStep == begin )
            break;

        itStep -= qb.countReturnValues();
    }

    // Add the "All" filter if necessary
    if( stillFiltering )
    {
        if( total > 1 )
        {
            // The "All" filter has much the same behavior as the
            // "Various Artists" item, so we use the isSampler bit
            CollectionItem* item = new CollectionItem( this, m_cat, false, true );
            item->setDragEnabled( true );
            item->setDropEnabled( false );
            item->setPixmap( 0, pixmap );
            item->setText( 0, allForCategory( q_cat, total ) );
            item->setPixmap( 1, incPixmap );
            item->setText( 1, "" );

            sort();
        }
        setColumnWidth( 1, width );
        QResizeEvent rev( size(), QSize() );
        viewportResizeEvent( &rev );
    }

    removeDuplicatedHeaders();
}

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionItem
//////////////////////////////////////////////////////////////////////////////////////////
void
CollectionItem::paintCell ( QPainter * painter, const QColorGroup & cg,
                         int column, int width, int align )
{
    if( static_cast<CollectionView::Tag>(column) == CollectionView::Rating )
    {
        QPixmap buf( width, height() );
        QPainter p( &buf, true );

        const QColorGroup _cg = listView()->palette().active();

        QColor bg = isSelected()  ? _cg.highlight()
            : isAlternate() ? listView()->alternateBackground()
            : listView()->viewport()->backgroundColor();
#if KDE_IS_VERSION( 3, 3, 91 )
        if( listView()->shadeSortColumn() && !isSelected() && listView()->columnSorted() == column )
        {
            /* from klistview.cpp
               Copyright (C) 2000 Reginald Stadlbauer <reggie@kde.org>
               Copyright (C) 2000,2003 Charles Samuels <charles@kde.org>
               Copyright (C) 2000 Peter Putzer */
            if ( bg == Qt::black )
                bg = QColor(55, 55, 55);  // dark gray
            else
            {
                int h,s,v;
                bg.hsv(&h, &s, &v);
                if ( v > 175 )
                    bg = bg.dark(104);
                else
                    bg = bg.light(120);
            }
        }
#endif

        buf.fill( bg );

        int rating = text(column).toInt();
        int i = 1, x = 1;
        const int y = height() / 2 - StarManager::instance()->getGreyStar()->height() / 2;
        bool half = rating%2;
        for(; i <= rating/2; ++i )
        {
            bitBlt( p.device(), x, y, StarManager::instance()->getStar( half ? rating/2 + 1 : rating/2 ) );
            x += StarManager::instance()->getGreyStar()->width() + listView()->itemMargin();
        }
        if( half )
        {
            bitBlt( p.device(), x, y, StarManager::instance()->getHalfStar( rating/2 + 1 ) );
            x += StarManager::instance()->getGreyStar()->width() + listView()->itemMargin();
        }

        p.end();
        painter->drawPixmap( 0, 0, buf );
    }
    else
    {
        KListViewItem::paintCell( painter, cg, column, width, align );
    }
}

int
CollectionItem::compare( QListViewItem* i, int col, bool ascending ) const
{
    QString a =    text( col );
    QString b = i->text( col );
    int ia, ib;

    //Special cases go first to take priority

    // Sampler is the first one in iPod view
    CollectionView* view = static_cast<CollectionView*>( listView() );
    if( view->viewMode() == CollectionView::modeIpodView )
    {
        if ( m_isSampler )
            return -1;
        if ( dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isSampler )
            return 1;
    }
    else if( view->viewMode() == CollectionView::modeFlatView )
    {
        ia = ib = 0;
        // correctly treat numeric values
        switch( col )
        {
            case CollectionView::Track:
            case CollectionView::DiscNumber:
            case CollectionView::Bitrate:
            case CollectionView::Score:
            case CollectionView::Rating:
            case CollectionView::Playcount:
            case CollectionView::BPM:
                ia = a.toInt();
                ib = b.toInt();
                break;
            case CollectionView::Length:
                ia = a.section( ':', 0, 0 ).toInt() * 60 + a.section( ':', 1, 1 ).toInt();
                ib = b.section( ':', 0, 0 ).toInt() * 60 + b.section( ':', 1, 1 ).toInt();
                break;
        }

        if( ia || ib )
        {
            if( ia < ib )
                return 1;
            if( ia > ib )
                return -1;
            return 0;
        }
    }

    // Unknown is always the first one unless we're doing iPod view, but if the two items to be compared are Unknown,
    // then compare the normal way
    if ( !( m_isUnknown && dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isUnknown ) )
    {
        if ( m_isUnknown )
            return -1;
        if ( dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isUnknown )
            return 1;
    }

    // Various Artists is always after unknown
    if ( m_isSampler )
      return -1;
    if ( dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isSampler )
      return 1;

    //Group heading should go above the items in that group
    if (dynamic_cast<DividerItem*>(i) && DividerItem::shareTheSameGroup(a, b, m_cat)) {
        return ascending == false ? -1 : 1;
    }

    switch( m_cat ) {
        case IdVisYearAlbum:
            a = a.left( a.find( i18n(" - ") ) );
            b = b.left( b.find( i18n(" - ") ) );
            // "?" are the last ones
            if ( a == "?" )
                return 1;
            if ( b == "?" )
                return -1;
        //fall through
        case IdYear:
            ia = a.toInt();
            ib = b.toInt();
            if (ia==ib)
                return QString::localeAwareCompare( text( col ).lower(), i->text( col ).lower() );
            if (ia<ib)
                return 1;
            else
                return -1;
        //For artists, we sort by ignoring "The" eg "The Who" sorts as if it were "Who"
        case IdArtist:
            if ( a.startsWith( "the ", false ) )
                CollectionView::manipulateThe( a, true );
            if ( b.startsWith( "the ", false ) )
                CollectionView::manipulateThe( b, true );
            break;
    }
    // Need to make single letter artist names sort lower than acented divider items
    // (e.g. The artist "A" should sort below the divider "Ä") so the divider colapsing
    // code works correctly. Making the artist a two letter word makes localeAwareCompare
    // give the result we want. See BR 126545.
    if ( a.length() == 1 && dynamic_cast<DividerItem*>(i) )
        a += a;

    // No special case, then fall on default
    return QString::localeAwareCompare( a.lower(), b.lower() );
}

void
CollectionItem::sortChildItems ( int column, bool ascending ) {
    CollectionView* view = static_cast<CollectionView*>( listView() );
    // Sort only if it's not the tracks
    if ( depth() + 1 < view->trackDepth())
        QListViewItem::sortChildItems( column, ascending );
}

//
// DividerItem

DividerItem::DividerItem( QListView* parent, QString txt, int cat/*, bool sortYearsInverted*/)
: KListViewItem( parent), m_blockText(false), m_text(txt), m_cat(cat)/*, m_sortYearsInverted(sortYearsInverted)*/
{
    setExpandable(false);
    setDropEnabled(false);
    setSelectable(false);
}

void
DividerItem::paintCell ( QPainter * p, const QColorGroup & cg,
                         int column, int width, int align )
{
    p->save();

    // be sure, that KListViewItem::paintCell() does not draw its text
    setBlockText( true );
    KListViewItem::paintCell(p, cg, column, width, align);
    setBlockText( false );

        //use bold font for the divider
    QFont f = p->font();
    f.setBold( true );
    p->setFont( f );

    // draw the text offset a little bit
    if( column == 0 )  // For iPod viewing mode
      {
        QFontMetrics fm( p->fontMetrics() );
        int x = !QApplication::reverseLayout() ? 25 : width - 25;
        int y = fm.ascent() + (height() - fm.height())/2;
        p->drawText( x, y, m_text );
      }

    //draw the baseline
    p->setPen( QPen(Qt::gray, 2) );
    p->drawLine(0, height() -2 , width, height() -2 );

    p->restore();
}

void
DividerItem::paintFocus ( QPainter* /*p*/, const QColorGroup& /*cg*/, const QRect& /*r*/ )
{
    //not implemented, we don't to show focus
}

//to draw the text on my own I have to be able to block the text, otherwise I could
// not use QListViewItem::paintCell() to draw the basic cell
QString
DividerItem::text(int column) const
{
    if (column == 0) {
        return m_blockText ? "" : m_text;
    }
    return KListViewItem::text(column);
}

int
DividerItem::compare( QListViewItem* i, int col, bool ascending ) const
{
    if (!i) {
	return QString::localeAwareCompare( text(col).lower(), QString("") );
    }
    if (dynamic_cast<CollectionItem*>(i)) {
        return -1 * i->compare(const_cast<DividerItem*>(this), col, ascending);
    }

    if (m_cat == IdYear ||
        m_cat == IdVisYearAlbum)
    {
        bool ok_a, ok_b;
        int ia =    text(col).toInt(&ok_a);
        int ib = i->text(col).toInt(&ok_b);

        if (ok_a && ok_b)
        {
            if      (ia == ib) return 0;
            else if (ia  < ib) return 1;
            else               return -1;
        }
    }
    return QString::localeAwareCompare( text(col).lower(), i->text(col).lower() );
}

QString
DividerItem::createGroup(const QString& src, int cat)
{
    QString ret;
    switch (cat) {
    case IdVisYearAlbum: {
        ret = src.left( src.find(" - ") );
        break;
    }
    case IdYear: {
        ret = src;
        if (ret.length() == 2 || ret.length() == 4) {
            ret = ret.left(ret.length() - 1) + '0';
        }
        break;
    }
    default:
        ret = src.stripWhiteSpace();
        // (Joe Rabinoff) deleted.  The bug this fixes is as follows.
        // If the category is Album and the album name is "Heroes" (by
        // David Bowie -- the quotes are part of the title), it gets
        // put under the H group, but then gets sorted under '"'.
        // What I've done is the wrong fix -- albums should be sorted
        // by their first alphanumeric character.
        /*
        while ( !ret.isEmpty() && !ret.at(0).isLetterOrNumber() ) {
            ret = ret.right( ret.length()-1 );
        }
        */
        if ( !ret.isEmpty()  &&  ret.at(0).isLetterOrNumber() )
          ret = ret.left( 1 ).upper();
        else
          return "";
        /*else
            ret = i18n( "Others" );*/
        /* By returning an empty string, no header is created. */

        if (QChar(ret.at(0)).isDigit()) {
            ret = "0-9";
        }
    }

    return ret;
}

bool
DividerItem::shareTheSameGroup(const QString& itemStr, const QString& divStr, int cat)
{
    bool inGroup = false;
    QString tmp = itemStr.stripWhiteSpace();

    switch (cat) {
    case IdVisYearAlbum: {
        QString sa = itemStr.left( itemStr.find( i18n(" - ") ) );
        QString sb = divStr.left(  divStr.find( i18n(" - ") ) );
        if (sa == sb) {
            inGroup = true;
        }
        break;
    }
    case IdYear: {
        int ia = itemStr.toInt();
        int ib = divStr.toInt();
        // they share one group if:
        //   o they are < 100 (short years '98')
        //   o they are > 1000 (long years '1998')
        //      o their 'century' is the same
        //   o are the same
        if (((ia < 100 || ia > 1000) && ia/10 == ib/10) ||
            (ia == ib)) {
            inGroup = true;
        }
        break;
    }
    case IdArtist:
        //"The Who" should count as being in "W" and not "T"
        if ( tmp.startsWith( "the ", false ) )
            CollectionView::manipulateThe( tmp, true );
        //Fall through
    default:
        if ( !tmp.isEmpty() ) {
            if (divStr == "0-9" && QChar(tmp.at(0)).isDigit()) {
                inGroup = true;
            }
            else if (tmp.startsWith(divStr, 0)) {
                inGroup = true;
            }
        }
    }

    return inGroup;
}

#include "collectionbrowser.moc"
