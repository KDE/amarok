// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2005 Christan Baumgart <christianbaumgart@web.de>
// See COPYING file for licensing information.

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "browserbar.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "debug.h"
#include "deletedialog.h"
#include "directorylist.h"
#include "k3bexporter.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "organizecollectiondialog.h"
#include "playlist.h"       //insertMedia()
#include "playlistbrowser.h"
#include "statusbar.h"
#include "tagdialog.h"
#include "threadweaver.h"
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

extern "C"
{
    #if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    #include <X11/Xlib.h>    //ControlMask in contentsDragMoveEvent()
    #endif
}

namespace amaroK { extern KConfig *config( const QString& ); }


CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
    , m_cat1Menu( new KPopupMenu( this ) )
    , m_cat2Menu( new KPopupMenu( this ) )
    , m_cat3Menu( new KPopupMenu( this ) )
    , m_timer( new QTimer( this ) )
    , m_returnPressed( false )
{
    setSpacing( 4 );

    m_toolbar = new Browser::ToolBar( this );

    { //<Search LineEdit>
        KToolBarButton *button;
        KToolBar* searchToolBar = new Browser::ToolBar( this );

        button       = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Enter search terms here" ), searchToolBar );
        m_searchEdit->installEventFilter( this );
        searchToolBar->setStretchableWidget( m_searchEdit );

        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>

    m_timeFilter = new KComboBox( this );
    m_timeFilter->insertItem( i18n( "Entire Collection" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Year" ) );
    m_timeFilter->insertItem( i18n( "Added Within Three Months" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Month" ) );
    m_timeFilter->insertItem( i18n( "Added Within One Week" ) );
    m_timeFilter->insertItem( i18n( "Added Today" ) );

    KActionCollection* ac = new KActionCollection( this );
    m_scanAction = new KAction( i18n( "Scan Changes" ), amaroK::icon( "refresh" ), 0, CollectionDB::instance(), SLOT( scanModifiedDirs() ), ac, "Start Scan" );

    // we need m_scanAction to be initialized before CollectionView's CTOR
    m_view = new CollectionView( this );
    m_view->installEventFilter( this );

    m_configureAction = new KAction( i18n( "Configure Folders" ), amaroK::icon( "configure" ), 0, this, SLOT( setupDirs() ), ac, "Configure" );
    m_treeViewAction = new KRadioAction( i18n( "Tree View" ), "view_tree", 0, m_view, SLOT( setTreeMode() ), ac, "Tree View" );
    m_flatViewAction = new KRadioAction( i18n( "Flat View" ), "view_detailed", 0, m_view, SLOT( setFlatMode() ), ac, "Flat View" );
    m_treeViewAction->setExclusiveGroup("view mode");
    m_flatViewAction->setExclusiveGroup("view mode");
    if(m_view->m_viewMode == CollectionView::modeTreeView)
        m_treeViewAction->setChecked(true);
    else
        m_flatViewAction->setChecked(true);

    m_showDividerAction = new KToggleAction( i18n( "Show Divider" ), "leftjust", 0, this, SLOT( toggleDivider() ), ac, "Show Divider" );
    m_showDividerAction->setChecked(m_view->m_showDivider);

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
    m_cat1Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat1Menu( int ) ), 0, IdYear );

    m_cat2Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( i18n( "&Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( i18n( "(Y&ear) - Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdVisYearAlbum);
    m_cat2Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat2Menu( int ) ), 0, IdYear );

    m_cat3Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat3Menu( int ) ), 0, IdNone );
    m_cat3Menu ->insertSeparator();
    m_cat3Menu ->insertItem( i18n( "A&lbum" ), m_view, SLOT( cat3Menu( int ) ), 0, IdAlbum );
    m_cat3Menu ->insertItem( i18n( "(Y&ear) - Album" ), m_view, SLOT( cat3Menu( int ) ), 0, IdVisYearAlbum);
    m_cat3Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat3Menu( int ) ), 0, IdArtist );
    m_cat3Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat3Menu( int ) ), 0, IdGenre );
    m_cat3Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat3Menu( int ) ), 0, IdYear );

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
    m_searchEdit->clear();
}

bool
CollectionBrowser::eventFilter( QObject *o, QEvent *e )
{
    typedef QListViewItemIterator It;

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
                m_searchEdit->clear();
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

        if( m_view->currentItem() && ( e->key() == Key_Up && m_view->currentItem()->itemAbove() == 0 ) )
        {
            QListViewItem *lastitem = *It( m_view, It::Visible );
            while( lastitem && lastitem->itemBelow() )
                lastitem = lastitem->itemBelow();
            m_view->currentItem()->setSelected( false );
            m_view->setCurrentItem( lastitem );
            lastitem->setSelected( true );
            m_view->ensureItemVisible( lastitem );
            return true;
        }

        if( m_view->currentItem() && ( e->key() == Key_Down && m_view->currentItem()->itemBelow() == 0 ) )
        {
            m_view->currentItem()->setSelected( false );
            m_view->setCurrentItem( *It( m_view, It::Visible ) );
            (*It( m_view, It::Visible ))->setSelected( true );
            m_view->ensureItemVisible( *It( m_view, It::Visible ) );
            return true;
        }

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
    m_toolbar->insertLineSeparator();

    m_showDividerAction->plug( m_toolbar );

    if ( !AmarokConfig::monitorChanges() ) {
        m_toolbar->setIconText( KToolBar::IconTextRight, false );
        m_scanAction->plug( m_toolbar );
        m_toolbar->setIconText( KToolBar::IconOnly, false );
    }

    m_configureAction->plug( m_toolbar );

    //This would break things if the toolbar is too big, see bug #121915
    //setMinimumWidth( m_toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView* CollectionView::m_instance = 0;


CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
        , m_timeFilter( 0 )
        , m_dirty( true )
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
        KConfig* config = amaroK::config( "Collection Browser" );
        m_cat1 = config->readNumEntry( "Category1", CollectionBrowser::IdArtist );
        m_cat2 = config->readNumEntry( "Category2", CollectionBrowser::IdAlbum );
        m_cat3 = config->readNumEntry( "Category3", CollectionBrowser::IdNone );
        m_viewMode = config->readNumEntry( "ViewMode", modeTreeView );
        m_showDivider = config->readBoolEntry( "ShowDivider", true);
        updateTrackDepth();
    //</READ CONFIG>
     KActionCollection* ac = new KActionCollection( this );
     KStdAction::selectAll( this, SLOT( selectAll() ), ac, "collectionview_select_all" );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ),
             this,                      SLOT( scanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ),
             this,                      SLOT( scanDone( bool ) ) );
    connect( BrowserBar::instance(),   SIGNAL( browserActivated( int ) ),
             this,                      SLOT( renderView() ) ); // renderView() checks if current tab is this

    connect( this,           SIGNAL( expanded( QListViewItem* ) ),
             this,             SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,           SIGNAL( collapsed( QListViewItem* ) ),
             this,             SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,           SIGNAL( returnPressed( QListViewItem* ) ),
             this,             SLOT( invokeItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( invokeItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );
    connect( header(),       SIGNAL( sizeChange( int, int, int ) ),
             this,             SLOT( triggerUpdate() ) );
}


CollectionView::~CollectionView() {
    DEBUG_FUNC_INFO

    KConfig* const config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Category1", m_cat1 );
    config->writeEntry( "Category2", m_cat2 );
    config->writeEntry( "Category3", m_cat3 );
    config->writeEntry( "ViewMode", m_viewMode );
    config->writeEntry( "ShowDivider", m_showDivider );
}

void
CollectionView::setShowDivider( bool show )
{
    if (show != m_showDivider) {
        m_showDivider = show;
        renderView(true);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// public slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::renderView(bool force /* = false */)  //SLOT
{
    SHOULD_BE_GUI
    DEBUG_BLOCK
    if(!force && !m_dirty )
        return;

    if( BrowserBar::instance()->currentBrowser() != m_parent )
    {
        // the collectionbrowser is intensive for sql, so we only renderView() if the tab
        // is currently active.  else, wait until user focuses it.
        debug() << "current browser is not collection, aborting renderView()" << endl;
        m_dirty = true;
        return;
    }
    m_dirty = false;

    if( childCount() )
        cacheView();

    //clear();
    safeClear();

    //query database for all records with the specified category
    QStringList values;
    QueryBuilder qb;

    if ( translateTimeFilter( timeFilter() ) > 0 )
        qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

    // MODE FLATVIEW
    if ( m_viewMode == modeFlatView )
    {
        if ( translateTimeFilter( timeFilter() ) <= 0
                && (m_filter.length() < 3 || (!m_filter.contains( " " ) && m_filter.endsWith( ":" ) ) ) ) {
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

        int filterTables = 0;
        for ( QValueList<Tag>::ConstIterator it = visibleColumns.constBegin(); it != visibleColumns.constEnd(); ++it )
        {
            switch ( *it )
            {
                case Artist: {
                    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabArtist;
                    }
                    break;
                case Album: {
                    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabAlbum;
                    }
                    break;
                case Genre: {
                    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName, true );
                    filterTables |= QueryBuilder::tabGenre;
                    }
                    break;
                case Title: {
                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle, true );
                    filterTables |= QueryBuilder::tabSong;
                    }
                    break;
                case Length: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valLength );
                    filterTables |= QueryBuilder::tabSong;
                    }
                    break;
                case Composer: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valComposer );
                    filterTables |= QueryBuilder::tabSong;
                    filterTables |= QueryBuilder::tabComposer;
                    }
                    break;
                case DiscNumber: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                    filterTables |= QueryBuilder::tabSong;
                    }
                    break;
                case Track: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valTrack );
                    filterTables |= QueryBuilder::tabSong;
                    }
                    break;
                case Year: {
                    qb.addReturnValue ( QueryBuilder::tabYear, QueryBuilder::valName );
                    filterTables |= QueryBuilder::tabYear;
                    }
                    break;
                case Comment: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valComment );
                    filterTables |= QueryBuilder::tabSong;
                    }
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
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valURL );
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
            item->setUrl( *it );
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
                    case Modified: {
                        QDateTime time = QDateTime();
                        time.setTime_t( (*it).toUInt() );
                        item->setText( *it_c, time.date().toString( Qt::LocalDate ) );
                        break;
                    }
                    case Playcount:
                    case Score: {
                        item->setText( *it_c, (*it).isNull() ? "0" : (*it) );
                        break;
                    }
                    case Rating: {
                        item->setText( *it_c, (*it).isNull() ? "0" : (*it) );
                        break;
                    }
                    case Filename:
                        item->setText( *it_c, KURL::fromPathOrURL(*it).filename() );
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

    // MODE TREEVIEW
    if( m_viewMode == modeTreeView )
    {
        setSorting( 0 );
        int VisYearAlbum = -1;
        int q_cat1=m_cat1;
        int q_cat2=m_cat2;
        int q_cat3=m_cat3;
        if( m_cat1 == CollectionBrowser::IdVisYearAlbum ||
            m_cat2 == CollectionBrowser::IdVisYearAlbum ||
            m_cat3 == CollectionBrowser::IdVisYearAlbum )
        {
            if( m_cat1==CollectionBrowser::IdVisYearAlbum )
            {
                VisYearAlbum = 1;
                q_cat1 = CollectionBrowser::IdAlbum;
            }
            if( m_cat2==CollectionBrowser::IdVisYearAlbum )
            {
                VisYearAlbum = 2;
                q_cat2 = CollectionBrowser::IdAlbum;
            }
            if( m_cat3==CollectionBrowser::IdVisYearAlbum )
            {
                VisYearAlbum = 3;
                q_cat3 = CollectionBrowser::IdAlbum;
            }
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
                    (*it) = i18n( "Unknown" );
                    unknown = true;
                }
                else if (m_showDivider)
                {
                    //Dividers for "The Who" should be created as "W", not "T", because
                    //that's how we sort it
                    QString actualStr = *it;
                    if ( m_cat1 == CollectionBrowser::IdArtist && actualStr.startsWith( "the ", false ) )
                        manipulateThe( actualStr, true );

                    QString headerStr = DividerItem::createGroup( actualStr, m_cat1);

                    if ( !containedDivider[headerStr] && headerStr != "" )
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
                qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

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

        int count = childCount() - dividerCount;
        QListViewItem *item = firstChild();
        if( dynamic_cast<DividerItem *>( item ) )
            item = item->nextSibling();
        while( count == 1 && item && item->isExpandable() )
        {
            item->setOpen( true );
            count = item->childCount();
            item = item->firstChild();
        }

        /* Following code depends on the order! */
        sort();

        QValueList<DividerItem *> toDelete;
        DividerItem *current=0, *last=0;
        bool empty;
        /* If we have two consecutive headers, one of them is useless, and should be removed */
        for( item = firstChild(),empty=false; item; item=item->nextSibling() )
        {
            if ( (current = dynamic_cast<DividerItem *>( item )) ) {
                if ( empty ) {
                    if ( !current->text(0).at(0).isLetterOrNumber()
                        || ( last->text(0).at(0).isLetterOrNumber()
                            && current->text(0).at(0).unicode() > last->text(0).at(0).unicode() ) )
                        toDelete += current;
                    else {
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
    restoreView();
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
        const bool rescan = ( AmarokConfig::collectionFolders() != setup->dirs() );
        setup->writeConfig();

        if ( rescan )
            CollectionDB::instance()->startScan();

        m_parent->m_scanAction->setEnabled( !AmarokConfig::monitorChanges() );
        m_parent->layoutToolbar();
    }
}


void
CollectionView::scanStarted() // SLOT
{
    m_parent->m_scanAction->setEnabled( false );
}


void
CollectionView::scanDone( bool changed ) //SLOT
{
    if( changed )
    {
        renderView(true);
    }

    m_parent->m_scanAction->setEnabled( !AmarokConfig::monitorChanges() );
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
    if ( m_cat1 == CollectionBrowser::IdAlbum ||
         m_cat2 == CollectionBrowser::IdAlbum ||
         m_cat3 == CollectionBrowser::IdAlbum )
            SortbyTrackFirst = true;

    // initalization for year - album mode
    QString tmptext;
    int VisYearAlbum = -1;
    int q_cat1=m_cat1;
    int q_cat2=m_cat2;
    int q_cat3=m_cat3;
    if( m_cat1 == CollectionBrowser::IdVisYearAlbum ||
        m_cat2 == CollectionBrowser::IdVisYearAlbum ||
        m_cat3 == CollectionBrowser::IdVisYearAlbum )
    {
        SortbyTrackFirst = true;
        if( m_cat1 == CollectionBrowser::IdVisYearAlbum )
        {
            VisYearAlbum = 1;
            q_cat1 = CollectionBrowser::IdAlbum;
        }
        if( m_cat2 == CollectionBrowser::IdVisYearAlbum )
        {
            VisYearAlbum = 2;
            q_cat2 = CollectionBrowser::IdAlbum;
        }
        if( m_cat3 == CollectionBrowser::IdVisYearAlbum )
        {
            VisYearAlbum = 3;
            q_cat3 = CollectionBrowser::IdAlbum;
        }
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
                if (VisYearAlbum==1)
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
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
            if ( !SortbyTrackFirst ) {
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            }

            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

            category = CollectionBrowser::IdNone;
            break;
    }

    qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    values = qb.run();
    uint countReturnValues = qb.countReturnValues();

    QPixmap pixmap;
    bool expandable = category != CollectionBrowser::IdNone;
    if ( expandable )
        pixmap = iconForCategory( category );

    //this check avoid possible problems on database errors. FIXME: Should we add some real error handling here,
    //like calling a collection update or something?
    if ( values.isEmpty() ) { return; }

    for ( int i = values.count() - countReturnValues; i >= 0; i -= countReturnValues )
    {
        QString text;
        bool unknown=false;

        if (  category == CollectionBrowser::IdVisYearAlbum )
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
}


void
CollectionView::slotCollapse( QListViewItem* item )  //SLOT
{
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
CollectionView::presetMenu( int id )  //SLOT
{
    switch ( id )
    {
        case CollectionBrowser::IdArtist:
            cat1Menu( CollectionBrowser::IdArtist, false );
            cat2Menu( CollectionBrowser::IdNone, false );
            cat3Menu( CollectionBrowser::IdNone, false );
            break;
        case CollectionBrowser::IdAlbum:
            cat1Menu( CollectionBrowser::IdAlbum, false );
            cat2Menu( CollectionBrowser::IdNone, false );
            cat3Menu( CollectionBrowser::IdNone, false );
            break;
        case CollectionBrowser::IdArtistAlbum:
            cat1Menu( CollectionBrowser::IdArtist, false );
            cat2Menu( CollectionBrowser::IdAlbum, false );
            cat3Menu( CollectionBrowser::IdNone, false );
            break;
        case CollectionBrowser::IdArtistVisYearAlbum:
            cat1Menu( CollectionBrowser::IdArtist, false );
            cat2Menu( CollectionBrowser::IdVisYearAlbum, false );
            cat3Menu( CollectionBrowser::IdNone, false );
            break;
        case CollectionBrowser::IdGenreArtist:
            cat1Menu( CollectionBrowser::IdGenre, false );
            cat2Menu( CollectionBrowser::IdArtist, false );
            cat3Menu( CollectionBrowser::IdNone, false );
            break;
        case CollectionBrowser::IdGenreArtistAlbum:
            cat1Menu( CollectionBrowser::IdGenre, false );
            cat2Menu( CollectionBrowser::IdArtist, false );
            cat3Menu( CollectionBrowser::IdAlbum, false );
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
    m_parent->m_cat1Menu->setItemChecked( m_cat1, true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );
    m_parent->m_cat3Menu->setItemEnabled( id , false );

    //if this item is checked in second menu, uncheck it
    if ( m_parent->m_cat2Menu->isItemChecked( id ) ) {
        m_parent->m_cat2Menu->setItemChecked( id, false );
        m_parent->m_cat2Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat2 = CollectionBrowser::IdNone;
        enableCat3Menu( false );
    }
    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat3 = CollectionBrowser::IdNone;
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

    enableCat3Menu( id != CollectionBrowser::IdNone );

    //prevent choosing the same category in both menus
    m_parent->m_cat3Menu->setItemEnabled( m_cat1 , false );
    if( id != CollectionBrowser::IdNone )
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
    updateTrackDepth();
    if ( rerender )
    {
        renderView(true);
    }
}


void
CollectionView::enableCat3Menu( bool enable )
{
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdAlbum, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdVisYearAlbum, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdArtist, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdGenre, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdYear, enable );

    if( !enable ) {
        m_parent->m_cat3Menu->setItemChecked( m_cat3, false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat3 = CollectionBrowser::IdNone;
    }
    updateTrackDepth();
}


void
CollectionView::invokeItem( QListViewItem* item ) //SLOT
{
    if ( !item || dynamic_cast<DividerItem*>(item) )
        return;

    item->setSelected( true );
    setCurrentItem( item );
    //append and prevent doubles in playlist
    if( item->isExpandable() )
        Playlist::instance()->insertMedia( listSelected(), Playlist::Unique | Playlist::Append );
    else
        Playlist::instance()->insertMedia( static_cast<CollectionItem*>( item )->url(), Playlist::Unique | Playlist::Append );

}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( dynamic_cast<DividerItem*>( item ) ) return;

    if ( item ) {
        KPopupMenu menu( this );

        int cat = 0;
        if ( m_viewMode != modeFlatView ) {
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
        }

        #ifdef AMAZON_SUPPORT
        enum Actions { APPEND, QUEUE, MAKE, SAVE, MEDIA_DEVICE, BURN_ARTIST, BURN_ALBUM, BURN_CD, COVER, INFO,
                       COMPILATION_SET, COMPILATION_UNSET, ORGANIZE, DELETE, TRASH, FILE_MENU  };
        #else
        enum Actions { APPEND, QUEUE, MAKE, SAVE, MEDIA_DEVICE, BURN_ARTIST, BURN_ALBUM, BURN_CD, INFO,
                       COMPILATION_SET, COMPILATION_UNSET, ORGANIZE, DELETE, TRASH, FILE_MENU  };
        #endif
        KURL::List selection = listSelected();
        menu.insertItem( SmallIconSet( amaroK::icon( "files" ) ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( amaroK::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( amaroK::icon( "fastforward" ) ), selection.count() == 1 ? i18n( "&Queue Track" )
            : i18n( "&Queue Tracks" ), QUEUE );

        if( selection.count() > 1 || item->isExpandable() )
            menu.insertItem( SmallIconSet( amaroK::icon( "save" ) ), i18n( "&Save as Playlist..." ), SAVE );

        menu.insertSeparator();

        if( MediaBrowser::isAvailable() )
            menu.insertItem( SmallIconSet( amaroK::icon( "device" ) ), i18n( "&Transfer to Media Device" ), MEDIA_DEVICE );

        if( cat == CollectionBrowser::IdArtist )
        {
            menu.insertItem( SmallIconSet( amaroK::icon( "burn" ) ), i18n("Burn All Tracks by This Artist"), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
        }
        else if( cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum )
        {
            menu.insertItem( SmallIconSet( amaroK::icon( "burn" ) ), i18n("Burn This Album"), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
        }
        else if( !item->isExpandable() )
        {
            menu.insertItem( SmallIconSet( amaroK::icon( "burn" ) ), i18n("Burn to CD"), BURN_CD );
            menu.setItemEnabled( BURN_CD, K3bExporter::isAvailable() );
        }

        menu.insertSeparator();

        #ifdef AMAZON_SUPPORT
        menu.insertItem( SmallIconSet( amaroK::icon( "download" ) ), i18n( "&Fetch Cover Image" ), this, SLOT( fetchCover() ), 0, COVER );
        menu.setItemEnabled(COVER, cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum );
        #endif

        menu.insertItem( SmallIconSet( amaroK::icon( "info" ) )
            , i18n( "Edit Track &Information...",  "Edit &Information for %n Tracks...", selection.count())
            , this, SLOT( showTrackInfo() ), 0, INFO );

        KPopupMenu fileMenu;
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n("Organize File...", "Organize %n Files..." , selection.count() )
                , ORGANIZE );

        fileMenu.insertItem( SmallIconSet( amaroK::icon( "remove" ) ), i18n("Delete File...", "Delete %n Files..." , selection.count() )
                , DELETE );

        menu.insertItem( SmallIconSet( amaroK::icon( "files" ) ), i18n("Manage Files"), &fileMenu, FILE_MENU );

        if ( cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum ) {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "ok" ), i18n( "Show under &Various Artists" ), COMPILATION_SET );
            menu.insertItem( SmallIconSet( "cancel" ), i18n( "&Don't Show under Various Artists" ), COMPILATION_UNSET );
        }

        QString trueItemText;

        //Work out the true name of the album ( where Unknown is "" ) , and the
        if ( dynamic_cast<CollectionItem*>( item ) )
        {
            CollectionItem* collectItem = static_cast<CollectionItem*>( item );
            trueItemText = collectItem->getSQLText( 0 );
            if ( cat == CollectionBrowser::IdVisYearAlbum && !collectItem->isUnknown() )
                trueItemText = trueItemText.right( trueItemText.length() - trueItemText.find( i18n( " - " ) ) - i18n( " - " ).length() );
        }
        else
        {
            trueItemText = item->text( 0 );
            warning() << "RMB pressed for non-CollectionItem with text '" << trueItemText << '\'' << endl;
        }

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
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( trueItemText );
                break;
            case BURN_ALBUM:
                K3bExporter::instance()->exportAlbum( trueItemText );
                break;
            case BURN_CD:
                K3bExporter::instance()->exportTracks( selection );
                break;
            case COMPILATION_SET:
                setCompilation( trueItemText, true );
                break;
            case COMPILATION_UNSET:
                setCompilation( trueItemText, false );
                break;
            case ORGANIZE:
                organizeFiles( listSelected(), i18n( "Organize Collection Files" ), false /* don't add to collection, just move */ );
                break;
            case DELETE:
                KURL::List files = listSelected();
                if ( DeleteDialog::showTrashDialog(this, files) )
                    CollectionDB::instance()->removeSongs( files );
                m_dirty = true;
                QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
                break;
        }
    }
}


void
CollectionView::setViewMode( int mode, bool rerender /*=true*/ )
{
    m_viewMode = mode;
    clear();
    updateColumnHeader();

    if ( rerender )
    {
        renderView(true);
    }
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
    if( cat == CollectionBrowser::IdVisYearAlbum )
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
        CollectionDB::instance()->fetchCover( this, values[0], album, false );
    #endif
}

void
CollectionView::showTrackInfo() //SLOT
{
     KURL::List urls = listSelected();
     int selectedTracksNumber = urls.count();

     //If we have only one, call the full dialog. Otherwise, the multiple tracks one.
     if ( selectedTracksNumber == 1 )
     {
          TagDialog* dialog = new TagDialog( urls.first() );
          dialog->show();
     }
     else if ( selectedTracksNumber )
     {
          TagDialog* dialog = new TagDialog( urls );
          dialog->show();
     }
}


void
CollectionView::organizeFiles( const KURL::List &urls, const QString &caption, bool copy )  //SLOT
{
    if( m_organizeURLs.count() )
    {
        if( copy != m_organizeCopyMode )
        {
            QString shortMsg = i18n( "Cannot start organize operation of different kind while another is in progress." );
            amaroK::StatusBar::instance()->shortMessage( shortMsg, KDE::StatusBar::Sorry );
            return;
        }
        else
        {
            m_organizeURLs += amaroK::recursiveUrlExpand( urls );
            amaroK::StatusBar::instance()->incrementProgressTotalSteps( this, urls.count() );
            return;
        }
    }

    QStringList folders = AmarokConfig::collectionFolders();

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

    KURL::List previewURLs = amaroK::recursiveUrlExpand( urls.first(), 1 );
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

        m_organizeURLs = amaroK::recursiveUrlExpand( urls );
        m_organizeCopyMode = copy;
        CollectionDB::instance()->createTables( true ); // create temp tables
        amaroK::StatusBar::instance()->newProgressOperation( this )
            .setDescription( caption )
            .setTotalSteps( m_organizeURLs.count() );

        while( !m_organizeURLs.empty() )
        {
            KURL &src = m_organizeURLs.first();

            if( !CollectionDB::instance()->organizeFile( src, dialog, copy ) )
            {
                skipped += src;
            }

            m_organizeURLs.pop_front();
            amaroK::StatusBar::instance()->incrementProgress( this );
        }

        CollectionDB::instance()->copyTempTables(); // copy temp table contents to permanent tables
        CollectionDB::instance()->dropTables( true ); // and drop them

        if( skipped.count() > 0 )
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
            amaroK::StatusBar::instance()->shortLongMessage( shortMsg, longMsg, KDE::StatusBar::Sorry );
        }
        m_dirty = true;
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
        amaroK::StatusBar::instance()->endProgressOperation( this );
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
                expandedList += amaroK::recursiveUrlExpand( *it );
            else
                expandedList += *it;
        }

        KURL::List cleanList;
        for( KURL::List::iterator it = expandedList.begin();
                it != expandedList.end();
                ++it )
        {
            if( !(*it).isLocalFile() || !CollectionDB::instance()->isFileInCollection( (*it).path() ) )
                cleanList += *it;
            else
                dropped++;
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
            amaroK::StatusBar::instance()->shortMessage( msg );
        if( cleanList.count() > 0 )
            organizeFiles( list, i18n( "Copy Files To Collection" ), true /* copy */ );
    }
}

void
CollectionView::safeClear()
{
    bool block = signalsBlocked();
    blockSignals( true );
    clearSelection();

    QListViewItem *c = firstChild();
    QListViewItem *n;
    while( c ) {
        n = c->nextSibling();
        delete c;
        c = n;
    }
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

        addColumn( captionForTag( Title ) );
#define includesArtist(cat) (((cat)&CollectionBrowser::IdArtist) \
        ||((cat)&CollectionBrowser::IdArtistAlbum) \
        ||((cat)&CollectionBrowser::IdGenreArtist) \
        ||((cat)&CollectionBrowser::IdGenreArtistAlbum) \
        ||((cat)&CollectionBrowser::IdArtistVisYearAlbum))
        if( includesArtist(m_cat1)||includesArtist(m_cat2)||includesArtist(m_cat3) )
            addColumn( captionForTag( Artist ) );
        else
            addColumn( captionForTag( Artist ), 0 );
#undef includesArtist
        addColumn( captionForTag( Composer ), 0 );
#define includesAlbum(cat) (((cat)&CollectionBrowser::IdAlbum) \
        ||((cat)&CollectionBrowser::IdArtistAlbum) \
        ||((cat)&CollectionBrowser::IdGenreArtistAlbum) \
        ||((cat)&CollectionBrowser::IdVisYearAlbum) \
        ||((cat)&CollectionBrowser::IdArtistVisYearAlbum))
        if( includesAlbum(m_cat1)||includesAlbum(m_cat2)||includesAlbum(m_cat3) )
            addColumn( captionForTag( Album ) );
        else
            addColumn( captionForTag( Album ), 0 );
#undef includesAlbum
#define includesGenre(cat) (((cat)&CollectionBrowser::IdGenre) \
        ||((cat)&CollectionBrowser::IdGenreArtist) \
        ||((cat)&CollectionBrowser::IdGenreArtistAlbum))
        if( includesGenre(m_cat1)||includesGenre(m_cat2)||includesGenre(m_cat3) )
            addColumn( captionForTag( Genre ) );
        else
            addColumn( captionForTag( Genre ), 0 );
#undef includesGenre
        addColumn( captionForTag( Length ),0  );
        addColumn( captionForTag( DiscNumber ), 0 );
        addColumn( captionForTag( Track ), 0 );
#define includesYear(cat) (((cat)&CollectionBrowser::IdYear) \
        ||((cat)&CollectionBrowser::IdVisYearAlbum) \
        ||((cat)&CollectionBrowser::IdArtistVisYearAlbum))
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

        setColumnAlignment( Track, Qt::AlignCenter );
        setColumnAlignment( DiscNumber, Qt::AlignCenter );
        setColumnAlignment( Length, Qt::AlignRight );
        setColumnAlignment( Bitrate, Qt::AlignCenter );
        setColumnAlignment( Score, Qt::AlignCenter );
        setColumnAlignment( Playcount, Qt::AlignCenter );

        //QListView allows invisible columns to be resized, so we disable resizing for them
        for ( int i = 0; i < columns(); ++i ) {
            setColumnWidthMode ( i, QListView::Manual );
            if ( columnWidth( i ) == 0 )
                header()->setResizeEnabled( false, i );
        }
        setRootIsDecorated( false );
    }
    else
    {
        setResizeMode( QListView::LastColumn );

        QString caption = captionForCategory( m_cat1 );
        int catArr[2] = {m_cat2, m_cat3};

        for(int i = 0; i < 2; i++) {
            if (catArr[i] != CollectionBrowser::IdNone ) {
                caption += " / " + captionForCategory( catArr[i] );
            }
        }
        addColumn( caption );
        setRootIsDecorated( true );
    }

    //manage column widths
    QResizeEvent rev( size(), QSize() );
    viewportResizeEvent( &rev );

    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdArtist, m_cat1 == CollectionBrowser::IdArtist && m_cat2 == CollectionBrowser::IdNone );
    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdAlbum, m_cat1 == CollectionBrowser::IdAlbum && m_cat2 == CollectionBrowser::IdNone );
    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdArtistAlbum, m_cat1 == CollectionBrowser::IdArtist && m_cat2 == CollectionBrowser::IdAlbum && m_cat3 == CollectionBrowser::IdNone );
    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdArtistVisYearAlbum, m_cat1 == CollectionBrowser::IdArtist && m_cat2 == CollectionBrowser::IdVisYearAlbum && m_cat3 == CollectionBrowser::IdNone );
    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdGenreArtist, m_cat1 == CollectionBrowser::IdGenre && m_cat2 == CollectionBrowser::IdArtist && m_cat3 == CollectionBrowser::IdNone );
    m_parent->m_categoryMenu->setItemChecked( CollectionBrowser::IdGenreArtistAlbum, m_cat1 == CollectionBrowser::IdGenre && m_cat2 == CollectionBrowser::IdArtist && m_cat3 == CollectionBrowser::IdAlbum );
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


KURL::List
CollectionView::listSelected()
{
    //Here we determine the URLs of all selected items. We use two passes, one for the parent items,
    //and another one for the children.

    KURL::List list;
    QListViewItem* item;
    QStringList values;
    QueryBuilder qb;

    // initalization for year - album mode
    QString tmptext;
    bool unknownText;
    int VisYearAlbum = -1;
    int q_cat1=m_cat1;
    int q_cat2=m_cat2;
    int q_cat3=m_cat3;
    if (m_cat1 == CollectionBrowser::IdVisYearAlbum || m_cat2 == CollectionBrowser::IdVisYearAlbum || m_cat3 == CollectionBrowser::IdVisYearAlbum)
    {
        if (m_cat1==CollectionBrowser::IdVisYearAlbum)
        {
            VisYearAlbum = 1;
            q_cat1 = CollectionBrowser::IdAlbum;
        }
        if (m_cat2==CollectionBrowser::IdVisYearAlbum)
        {
            VisYearAlbum = 2;
            q_cat2 = CollectionBrowser::IdAlbum;
        }
        if (m_cat3==CollectionBrowser::IdVisYearAlbum)
        {
            VisYearAlbum = 3;
            q_cat3 = CollectionBrowser::IdAlbum;
        }
    }

    if ( m_viewMode == modeFlatView )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            if ( item->isSelected() )
                list << static_cast<CollectionItem*>( item ) ->url();

        return list;
    }
    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() )
        {
            const bool sampler = static_cast<CollectionItem*>(  item )->isSampler();
            qb.clear();
            if ( translateTimeFilter( timeFilter() ) > 0 )
                qb.addFilter( QueryBuilder::tabSong, QueryBuilder::valCreateDate, QString().setNum( QDateTime::currentDateTime().toTime_t() - translateTimeFilter( timeFilter() ) ), QueryBuilder::modeGreater );

            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            tmptext = static_cast<CollectionItem*>( item )->getSQLText( 0 );
            unknownText = tmptext.isEmpty();

            if ( !sampler )
            {
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
    if ( m_cat2 == CollectionBrowser::IdNone )
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
    if ( m_cat3 == CollectionBrowser::IdNone )
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
        const QString query = QString("SELECT title, length FROM tags WHERE url = '%1';")
                              .arg( db->escapeString( (*it).path() ) ); //no operator->, how suck!
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
        case CollectionBrowser::IdAlbum:
            icon = "cdrom_unmount";
            break;
        case CollectionBrowser::IdVisYearAlbum:
            icon = "cdrom_unmount";
            break;
        case CollectionBrowser::IdArtist:
            icon = "personal";
            break;

        case CollectionBrowser::IdGenre:
            icon = "kfm";
            break;

        case CollectionBrowser::IdYear:
            icon = "history";
            break;
    }

    return KGlobal::iconLoader()->loadIcon( icon, KIcon::Toolbar, KIcon::SizeSmall );
}

QString
CollectionView::captionForCategory( const int cat ) const
{
    switch( cat )
    {
        case CollectionBrowser::IdAlbum:
            return i18n( "Album" );
            break;
        case CollectionBrowser::IdVisYearAlbum:
            return i18n( "Year" ) + i18n( " - " ) + i18n( "Album" );
            break;
        case CollectionBrowser::IdArtist:
            return i18n( "Artist" );
            break;

        case CollectionBrowser::IdGenre:
            return i18n( "Genre" );
            break;

        case CollectionBrowser::IdYear:
            return i18n( "Year" );
            break;
    }

    return QString::null;
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
        default: break;
    }
    return caption;
}

void
CollectionView::setCompilation( const QString &album, bool compilation )
{
    //visual feedback
    QApplication::setOverrideCursor( KCursor::waitCursor() );

    //Set it in the DB. We don't need to update the view now as we do it at the end.
    QStringList files = CollectionDB::instance()->setCompilation( album, compilation, false );

    foreachType( QStringList, files ) {
        if ( !TagLib::File::isWritable( QFile::encodeName( *it ) ) )
            continue;

        MetaBundle mb( KURL::fromPathOrURL( *it ) );

        mb.setCompilation( compilation ? MetaBundle::CompilationYes : MetaBundle::CompilationNo );

        if( mb.save() ) {
            mb.updateFilesize();
            //update the collection db, since filesize might have changed
            CollectionDB::instance()->updateTags( mb.url().path(), mb, false );
        }
    }
    //visual feedback
    QApplication::restoreOverrideCursor();
    if ( !files.isEmpty() ) renderView(true);
}

void
CollectionView::cacheView()
{
    //free cache
    m_cacheOpenItemPaths.clear();
    m_cacheViewportTopItem = QString::null;

    m_cacheCurrentItem = currentItem() ? currentItem()->text( 0 ) : QString::null;

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
    QListViewItem* item = itemAt( QPoint(0, 0) );
    if ( item )
        m_cacheViewportTopItem = item->text(0);
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
    QListViewItem* item = findItem( m_cacheViewportTopItem, 0 );
    if ( item )
        setContentsPos( 0, itemPos(item) );

    item = findItem( m_cacheCurrentItem, 0 );
    if ( item ) {
        setCurrentItem( item );
        item->setSelected( true );
    }

    //free cache
    m_cacheOpenItemPaths.clear();
    m_cacheViewportTopItem = QString::null;
    m_cacheCurrentItem = QString::null;
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
                    "To enable the Flat-View mode, please enter search terms in the filter line above."
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
    bool m3 = (m_cat3 == CollectionBrowser::IdNone);
    bool m2 = (m_cat2 == CollectionBrowser::IdNone);
    bool m1 = (m_cat1 == CollectionBrowser::IdNone);
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
    header()->blockSignals( true );

    const double width = e->size().width();

    int visibleColumns = 0;
    for ( int i = 0; i < columns(); ++i )
        if ( columnWidth( i ) != 0 )
            visibleColumns ++;

    if ( visibleColumns != 0 ) {
        for ( int c = 0; c < columns(); ++c ) {
            if ( columnWidth( c ) != 0 )
                setColumnWidth( c, int( width/visibleColumns) );
        }
    }

    header()->blockSignals( false );

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
            // added within one year
            filterSecs = 60 * 60 * 24 * 365;
            break;

        case 2:
            // added within three months
            filterSecs = 60 * 60 * 24 * 91;
            break;

        case 3:
            // added within one month
            filterSecs = 60 * 60 * 24 * 30;
            break;

        case 4:
            // added within one week
            filterSecs = 60 * 60 * 24 * 7;
            break;

        case 5:
            // added today
            filterSecs = 60 * 60 * 24;
            break;
    }

    return filterSecs;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionItem
//////////////////////////////////////////////////////////////////////////////////////////
int
CollectionItem::compare( QListViewItem* i, int col, bool ascending ) const
{
    QString a =    text( col );
    QString b = i->text( col );
    int ia, ib;

    //Special cases go first to take priority

    // Unknown is always the first one, but if the two items to be compared are Unknown,
    // then compare the normal way
    if ( !( m_isUnknown && dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isUnknown ) )
    {
        if ( m_isUnknown )
            return -1;
        if ( dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isUnknown )
            return 1;
    }

    // Various Artists is always after unknown
    if ( m_cat == CollectionBrowser::IdArtist )
    {
        if ( m_isSampler )
            return -1;
        if ( dynamic_cast<CollectionItem*>( i ) && static_cast<CollectionItem*>( i )->m_isSampler )
            return 1;
    }

    //Group heading should go above the items in that group
    if (dynamic_cast<DividerItem*>(i) && DividerItem::shareTheSameGroup(a, b, m_cat)) {
        return ascending == false ? -1 : 1;
    }

    switch( m_cat ) {
        case CollectionBrowser::IdVisYearAlbum:
            a = a.left( a.find( i18n(" - ") ) );
            b = b.left( b.find( i18n(" - ") ) );
            // "?" are the last ones
            if ( a == "?" )
                return 1;
            if ( b == "?" )
                return -1;
        //fall through
        case CollectionBrowser::IdYear:
            ia = a.toInt();
            ib = b.toInt();
            if (ia==ib)
                return QString::localeAwareCompare( text( col ).lower(), i->text( col ).lower() );
            if (ia<ib)
                return 1;
            else
                return -1;
        //For artists, we sort by ignoring "The" eg "The Who" sorts as if it were "Who"
        case CollectionBrowser::IdArtist:
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
    QFontMetrics fm(p->fontMetrics());
    int x = !QApplication::reverseLayout() ? 25 : width - 25;
    int y = fm.ascent() + (height() - fm.height())/2;
    p->drawText(x, y, m_text);

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
    if (dynamic_cast<CollectionItem*>(i)) {
        return -1 * i->compare(const_cast<DividerItem*>(this), col, ascending);
    }

    if (m_cat == CollectionBrowser::IdYear ||
        m_cat == CollectionBrowser::IdVisYearAlbum)
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
    case CollectionBrowser::IdVisYearAlbum: {
        ret = src.left( src.find(" - ") );
        break;
    }
    case CollectionBrowser::IdYear: {
        ret = src;
        if (ret.length() == 2 || ret.length() == 4) {
            ret = ret.left(ret.length() - 1) + "0";
        }
        break;
    }
    default:
        ret = src.stripWhiteSpace();
        while ( !ret.isEmpty() && !ret.at(0).isLetterOrNumber() ) {
            ret = ret.right( ret.length()-1 );
        }
        if ( !ret.isEmpty() )
            ret = ret.left( 1 ).upper();
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
    case CollectionBrowser::IdVisYearAlbum: {
        QString sa = itemStr.left( itemStr.find( i18n(" - ") ) );
        QString sb = divStr.left(  divStr.find( i18n(" - ") ) );
        if (sa == sb) {
            inGroup = true;
        }
        break;
    }
    case CollectionBrowser::IdYear: {
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
    case CollectionBrowser::IdArtist:
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
