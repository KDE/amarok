// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2005 Christan Baumgart <christianbaumgart@web.de>
// See COPYING file for licensing information.

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "browserToolBar.h"
#include "clicklineedit.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "debug.h"
#include "directorylist.h"
#include "k3bexporter.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "playlist.h"       //insertMedia()
#include "playlistbrowser.h"
#include "statusbar.h"
#include "tagdialog.h"

#include <unistd.h>         //CollectionView ctor

#include <qapplication.h>
#include <qcstring.h>
#include <qdragobject.h>
#include <qlayout.h>        //infobox
#include <qpainter.h>
#include <qpixmap.h>
#include <qptrlist.h>
#include <qpushbutton.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()
#include <qheader.h>

#include <kactioncollection.h>
#include <kapplication.h>   //kapp
#include <kconfig.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <kiconloader.h>    //renderView()
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()

namespace amaroK { extern KConfig *config( const QString& ); }

CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
    , m_cat1Menu( new KPopupMenu( this ) )
    , m_cat2Menu( new KPopupMenu( this ) )
    , m_cat3Menu( new KPopupMenu( this ) )
    , m_timer( new QTimer( this ) )
{
    setSpacing( 4 );

    m_toolbar = new Browser::ToolBar( this );

    { //<Search LineEdit>
        KToolBarButton *button;
        KToolBar* searchToolBar = new Browser::ToolBar( this );

        button       = new KToolBarButton( "locationbar_erase", 0, searchToolBar );
        m_searchEdit = new ClickLineEdit( i18n( "Filter here..." ), searchToolBar );
        m_searchEdit->installEventFilter( this );
        searchToolBar->setStretchableWidget( m_searchEdit );

        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>

    KActionCollection* ac = new KActionCollection( this );
    m_scanAction = new KAction( i18n( "Update" ), "reload", 0, CollectionDB::instance(), SLOT( scanMonitor() ), ac, "Start Scan" );

    // we need m_scanAction to be initialized before CollectionView's CTOR
    m_view = new CollectionView( this );
    m_view->installEventFilter( this );

    m_configureAction = new KAction( i18n( "Configure Folders" ), "configure", 0, this, SLOT( setupDirs() ), ac, "Configure" );
    m_treeViewAction = new KRadioAction( i18n( "Tree View" ), "view_tree", 0, m_view, SLOT( setTreeMode() ), ac, "Tree View" );
    m_flatViewAction = new KRadioAction( i18n( "Flat View" ), "view_detailed", 0, m_view, SLOT( setFlatMode() ), ac, "Flat View" );
    m_treeViewAction->setExclusiveGroup("view mode");
    m_flatViewAction->setExclusiveGroup("view mode");
    if(m_view->m_viewMode == CollectionView::modeTreeView)
        m_treeViewAction->setChecked(true);
    else
        m_flatViewAction->setChecked(true);

    m_tagfilterMenuButton = new KActionMenu( i18n( "Group By" ), "filter", ac );
    m_tagfilterMenuButton->setDelayed( false );
    m_tagfilterMenuButton->setEnabled( m_view->m_viewMode == CollectionView::modeTreeView );
    connect ( m_treeViewAction, SIGNAL ( toggled(bool) ), m_tagfilterMenuButton, SLOT( setEnabled (bool) ) );

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
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    setFocusProxy( m_view ); //default object to get focus
}

void
CollectionBrowser::slotSetFilterTimeout() //SLOT
{
    m_timer->start( 280, true ); //stops the timer for us first
}

void
CollectionBrowser::slotSetFilter() //SLOT
{
    m_timer->stop();

    m_view->setFilter( m_searchEdit->text() );
    m_view->renderView();
}

void
CollectionBrowser::setupDirs()  //SLOT
{
    m_view->setupDirs();
}

bool CollectionBrowser::eventFilter( QObject *o, QEvent *e )
{
    typedef QListViewItemIterator It;

    switch( e->type() )
    {
    case 6/*QEvent::KeyPress*/:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        if( o == m_searchEdit ) //the search lineedit
        {
            QListViewItem *item;
            switch( e->key() )
            {
            case Key_Up:
                item = *It( m_view, It::Visible );
                if( item )
                    while( item->itemBelow() )
                        item = item->itemBelow();
                if( item )
                {
                    m_view->setFocus();
                    m_view->setCurrentItem( item );
                    item->setSelected( true );
                    m_view->ensureItemVisible( item );
                    return true;
                }
                return false;
            case Key_Down:
                if( ( item = *It( m_view, It::Visible ) ) )
                {
                    m_view->setFocus();
                    m_view->setCurrentItem( item );
                    item->setSelected( true );
                    m_view->ensureItemVisible( item );
                    return true;
                }
                return false;

            case Key_PageDown:
            case Key_PageUp:
                QApplication::sendEvent( m_view, e );
                return true;

            case Key_Escape:
                m_searchEdit->clear();
                return true;

            default:
                return false;
            }
        }

        if( m_view->currentItem() && ( ( e->key() == Key_Up   && m_view->currentItem()->itemAbove() == 0 )
                                || ( e->key() == Key_Down && m_view->currentItem()->itemBelow() == 0 ) ) )
        {
            m_view->currentItem()->setSelected( false );
            m_searchEdit->setFocus();
            m_view->ensureItemVisible( *It( m_view, It::Visible ) );
            return true;
        }

        if( ( e->key() >= Key_0 && e->key() <= Key_Z ) || e->key() == Key_Backspace )
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

    if ( !AmarokConfig::monitorChanges() )
        m_scanAction->plug( m_toolbar );

    m_configureAction->plug( m_toolbar );

    setMinimumWidth( m_toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
}



//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView* CollectionView::m_instance = 0;


CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
{
    DEBUG_FUNC_INFO
    m_instance = this;

    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setSorting( 0 );
    setShowSortIndicator( true );
    setAcceptDrops( false );
    setAllColumnsShowFocus( true );

    //<READ CONFIG>
        KConfig* config = amaroK::config( "Collection Browser" );
        m_cat1 = config->readNumEntry( "Category1", CollectionBrowser::IdArtist );
        m_cat2 = config->readNumEntry( "Category2", CollectionBrowser::IdAlbum );
        m_cat3 = config->readNumEntry( "Category3", CollectionBrowser::IdNone );
        m_viewMode = config->readNumEntry( "ViewMode", modeTreeView );
        updateTrackDepth();
    //</READ CONFIG>
     KActionCollection* ac = new KActionCollection( this );
     KStdAction::selectAll( this, SLOT( selectAll() ), ac, "collectionview_select_all" );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ),
             this,                      SLOT( scanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ),
             this,                      SLOT( scanDone( bool ) ) );

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
}


//////////////////////////////////////////////////////////////////////////////////////////
// public slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::renderView()  //SLOT
{
    DEBUG_FUNC_INFO

    if ( childCount() )
        cacheView();

    clear();

    //query database for all records with the specified category
    QStringList values;
    QueryBuilder qb;

    // MODE FLATVIEW
    if ( m_viewMode == modeFlatView )
    {
        if ( m_filter.length() < 3 ) {
            // Redraw bubble help
            triggerUpdate();
            return;
        }

        QValueList<Tag> visibleColumns;
        for ( int c = 0; c < columns(); ++c )
            if ( columnWidth( c ) != 0 )
                visibleColumns.append( (Tag)c );

        //always fetch URL
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

        int filterTables = 0;
        for ( QValueList<Tag>::ConstIterator it = visibleColumns.constBegin(); it != visibleColumns.constEnd(); ++it )
        {
            switch ( *it )
            {
                case Artist: {
                    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
                    filterTables |= QueryBuilder::tabArtist;
                    }
                    break;
                case Album: {
                    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
                    filterTables |= QueryBuilder::tabAlbum;
                    }
                    break;
                case Genre: {
                    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
                    filterTables |= QueryBuilder::tabGenre;
                    }
                    break;
                case Title: {
                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
                    filterTables |= QueryBuilder::tabSong;
                    }
                    break;
                case Length: {
                    qb.addReturnValue ( QueryBuilder::tabSong, QueryBuilder::valLength );
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
                default:
                    qb.addReturnValue( QueryBuilder::tabDummy, QueryBuilder::valDummy );
                    break;
            }
        }

        qb.setGoogleFilter( filterTables, m_filter );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );

        //we leftjoin the query so it can return mysql NULL cells, i.e. for score and playcount
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

        qb.addReturnValue( q_cat1, QueryBuilder::valName );

        if( VisYearAlbum == 1 )
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );

        qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );

        if( VisYearAlbum == 1 )
            qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );

        qb.sortBy( q_cat1, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );

        if( q_cat1 == QueryBuilder::tabArtist )
            qb.setOptions( QueryBuilder::optNoCompilations );

        values = qb.run();

        //add items to the view

        if( values.count() )
        {
            // if year - artist is cat 1 rebuild the list
            if( VisYearAlbum == 1 )
            {
                QStringList tmpvalues;
                for( unsigned int i=0; i < values.count()-1 ; i += 2 )
                {
                    tmpvalues += ( values[i+1].isEmpty() ? "?" : values[i+1]) +
                                i18n( " - " ) +
                                ( values[i].isEmpty() ? i18n( "Unknown" ) : values[i] );
                }
                values = tmpvalues;
            }

            for ( QStringList::Iterator it = values.fromLast(), begin = values.begin(); true; --it )
            {
                if ( (*it).stripWhiteSpace().isEmpty() )
                    (*it) = i18n( "Unknown" );

                if ( (*it).startsWith( "the ", false ) )
                    manipulateThe( *it, true );

                KListViewItem* item = new CollectionItem( this, m_cat1 );
                item->setExpandable( true );
                item->setDragEnabled( true );
                item->setDropEnabled( false );
                item->setText( 0, *it );
                item->setPixmap( 0, pixmap );

                if ( it == begin )
                    break;
            }
        }

        //check if we need to add a Various Artists node
        if ( q_cat1 == QueryBuilder::tabArtist )
        {
            qb.clear();
            qb.addReturnValue( q_cat1, QueryBuilder::valName );
            qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );
            qb.setOptions( QueryBuilder::optOnlyCompilations | QueryBuilder::optRemoveDuplicates );
            qb.setLimit( 0, 1 );
            values = qb.run();

            if ( values.count() )
            {
                KListViewItem* item = new CollectionItem( this, m_cat1 );
                item->setExpandable( true );
                item->setDragEnabled( true );
                item->setDropEnabled( false );
                    item->setText( 0, i18n( "Various Artists" ) );
                item->setPixmap( 0, pixmap );
            }
        }

        int count = childCount();
        QListViewItem *item = firstChild();
        while( count == 1 && item && item->isExpandable() )
        {
            item->setOpen( true );
            count = item->childCount();
            item = item->firstChild();
        }
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
    DEBUG_BLOCK

    if( changed )
        renderView();

    m_parent->m_scanAction->setEnabled( !AmarokConfig::monitorChanges() );
}


void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    DEBUG_FUNC_INFO
    if ( !item || !item->isExpandable() ) return;

    int category = 0;
    QStringList values;

    QueryBuilder qb;
    bool c = false;

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

    switch ( item->depth() )
    {
        case 0:
            if( endsInThe( item->text( 0 ) ) )
            {
                tmptext = item->text( 0 );
                manipulateThe( tmptext );

                QStringList matches;
                matches << item->text( 0 ) << tmptext;

                qb.addMatches( q_cat1, matches );
            }
            else if ( item->text( 0 ) != i18n( "Various Artists" ) )
            {
                QString tmptext = item->text( 0 );
                if( VisYearAlbum == 1 )
                {
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year );
                }
                qb.addMatch( q_cat1, tmptext );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            if ( m_cat2 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( q_cat2, QueryBuilder::valTitle );
                qb.addReturnValue( q_cat2, QueryBuilder::valURL );
                if ( c ) qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
                qb.sortBy( q_cat2, QueryBuilder::valTrack );
                qb.sortBy( q_cat2, QueryBuilder::valURL );
            }
            else
            {
                c = false;
                qb.addReturnValue( q_cat2, QueryBuilder::valName );
                if( VisYearAlbum == 2 )
                {
                    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
                    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
                }
                qb.sortBy( q_cat2, QueryBuilder::valName );
            }

            category = m_cat2;
            break;

        case 1:
            if( endsInThe( item->text( 0 ) ) )
            {
                tmptext = item->text( 0 );
                manipulateThe( tmptext );

                QStringList matches;
                matches << item->text( 0 ) << tmptext;

                qb.addMatches( q_cat1, matches );
            }
            else if( item->parent()->text( 0 ) != i18n( "Various Artists" ) )
            {
                tmptext = item->parent()->text( 0 );
                QStringList matches( item->parent()->text( 0 ) ) ;

                if( endsInThe( tmptext ) ) {
                    manipulateThe( tmptext );
                    matches << tmptext;
                }

                if( VisYearAlbum == 1 )
                {
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year );
                }

                qb.addMatches( q_cat1, matches );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            tmptext = item->text( 0 );

            if( VisYearAlbum == 2 )
            {
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year );
            }
            qb.addMatch( q_cat2, tmptext );

            if( m_cat3 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( q_cat3, QueryBuilder::valTitle );
                qb.addReturnValue( q_cat3, QueryBuilder::valURL );
                if ( c ) qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );

                qb.sortBy( q_cat3, QueryBuilder::valTrack );
                qb.sortBy( q_cat3, QueryBuilder::valURL );
            }
            else
            {
                c = false;
                qb.addReturnValue( q_cat3, QueryBuilder::valName );
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
            // check for compilations
            if( endsInThe( item->text( 0 ) ) )
            {
                tmptext = item->text( 0 );
                manipulateThe( tmptext );
                QStringList matches;
                matches << item->text( 0 ) << tmptext;

                qb.addMatches( q_cat1, matches );
            }
            else if ( item->parent()->parent()->text( 0 ) != i18n( "Various Artists" ) )
            {
                tmptext = item->parent()->parent()->text( 0 );
                QStringList matches( item->parent()->parent()->text( 0 ) ) ;

                if( endsInThe( tmptext ) ) {
                    manipulateThe( tmptext );
                    matches << tmptext;
                }

                if (VisYearAlbum==1)
                {
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year );
                }

                qb.addMatches( q_cat1, matches );
            }
            else
            {
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                c = true;
            }

            tmptext = item->parent()->text( 0 );
            QStringList matches( item->parent()->text( 0 ) ) ;

            if( endsInThe( tmptext ) ) {
                manipulateThe( tmptext );
                matches << tmptext;
            }

            if( VisYearAlbum == 2 )
            {
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year );
            }

            qb.addMatches( q_cat2, matches );

            tmptext = item->text( 0 );
            matches.clear();

            if( VisYearAlbum == 3 )
            {
                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                yearAlbumCalc( year, tmptext );
                qb.addMatch( QueryBuilder::tabYear, year );
            }

            matches << tmptext;

            if( endsInThe( tmptext ) ) {
                manipulateThe( tmptext );
                matches << tmptext;
            }

            qb.addMatches( q_cat3, matches );

            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            if( c )
                qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );

            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valURL );

            category = CollectionBrowser::IdNone;
            break;
    }

    qb.setGoogleFilter( q_cat1 | q_cat2 | q_cat3 | QueryBuilder::tabSong, m_filter );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    values = qb.run();
    int countReturnValues = qb.countReturnValues();


    if( category == CollectionBrowser::IdVisYearAlbum )
    {
        QStringList tmpvalues;
        for( unsigned int i=0; i<=values.count() - countReturnValues; i += countReturnValues )
        {
            tmpvalues += ( values[i+1].isEmpty() ? "?" : values[i+1]) +
                           i18n( " - " ) +
                         ( values[i].isEmpty() ? i18n( "Unknown" ) : values[i] );
        }
        values = tmpvalues;
        countReturnValues--;
    }

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

        //show "artist - title" for compilations
        if ( c )
            text = values[ i + 2 ].stripWhiteSpace().isEmpty() ? i18n( "Unknown" ) : values[ i + 2 ] + i18n(" - ");
        text += values[ i ].stripWhiteSpace().isEmpty() ? i18n( "Unknown" ) : values[ i ];

        CollectionItem* child = new CollectionItem( item, category );
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
    DEBUG_FUNC_INFO

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

    renderView();
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
        renderView();
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
        renderView();
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
        renderView();
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
    if ( !item )
        return;
    //append and prevent doubles in playlist
    if( item->isExpandable() )
        Playlist::instance()->insertMedia( listSelected(), Playlist::Unique | Playlist::Append );
    else
        Playlist::instance()->insertMedia( static_cast<CollectionItem*>( item )->url(), Playlist::Unique | Playlist::Append );

}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
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
                       COMPILATION_SET, COMPILATION_UNSET, ORGANIZE, DELETE, FILE_MENU  };
        #else
        enum Actions { APPEND, QUEUE, MAKE, SAVE, MEDIA_DEVICE, BURN_ARTIST, BURN_ALBUM, BURN_CD, INFO,
                       COMPILATION_SET, COMPILATION_UNSET, ORGANIZE, DELETE, FILE_MENU  };
        #endif
        KURL::List selection = listSelected();
        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), selection.count() == 1 ? i18n( "&Queue Track" )
            : i18n( "&Queue Tracks" ), QUEUE );

        if( selection.count() > 1 || item->isExpandable() )
            menu.insertItem( SmallIconSet( "filesave" ), i18n( "&Save as Playlist..." ), SAVE );

        menu.insertSeparator();

        if( MediaBrowser::isAvailable() )
            menu.insertItem( SmallIconSet( "usbpendrive_unmount" ), i18n( "Add to Media Device &Transfer Queue" ), MEDIA_DEVICE );

        if( cat == CollectionBrowser::IdArtist )
        {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn All Tracks by This Artist"), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
        }
        else if( cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum )
        {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn This Album"), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
        }
        else if( !item->isExpandable() )
        {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD"), BURN_CD );
            menu.setItemEnabled( BURN_CD, K3bExporter::isAvailable() );
        }

        menu.insertSeparator();

        #ifdef AMAZON_SUPPORT
        menu.insertItem( SmallIconSet( "www" ), i18n( "&Fetch Cover Image" ), this, SLOT( fetchCover() ), 0, COVER );
        menu.setItemEnabled(COVER, cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum );
        #endif

        menu.insertItem( SmallIconSet( "info" )
            , i18n( "Edit Track &Information...",  "Edit &Information for %n Tracks...", selection.count())
            , this, SLOT( showTrackInfo() ), 0, INFO );

        KPopupMenu fileMenu;
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n("Organize File...", "Organize %n Files..." , selection.count() )
                , ORGANIZE );
        fileMenu.insertItem( SmallIconSet( "editdelete" ), i18n("Delete File...", "Delete %n Files..." , selection.count() )
                , DELETE );
        menu.insertItem( SmallIconSet( "folder" ), i18n("Manage Files"), &fileMenu, FILE_MENU );

       if ( cat == CollectionBrowser::IdAlbum || cat == CollectionBrowser::IdVisYearAlbum )
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "ok" ), i18n( "&Mark as Compilation" ), COMPILATION_SET );
            menu.insertItem( SmallIconSet( "cancel" ), i18n( "&Unmark as Compilation" ), COMPILATION_UNSET );
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
                MediaDevice::instance()->addURLs( selection );
                break;
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( item->text(0) );
                break;
            case BURN_ALBUM:
                if ( cat == CollectionBrowser::IdVisYearAlbum )
                    K3bExporter::instance()->exportAlbum(
                        item->text(0).right( item->text(0).length() - item->text(0).find( i18n(" - ") ) - i18n(" - ").length() )
                    );
                else
                    K3bExporter::instance()->exportAlbum( item->text(0) );
                break;
            case BURN_CD:
                K3bExporter::instance()->exportTracks( selection );
                break;
            case COMPILATION_SET:
                if ( cat == CollectionBrowser::IdVisYearAlbum )
                    CollectionDB::instance()->setCompilation(
                        item->text(0).right( item->text(0).length() - item->text(0).find( i18n(" - ") ) - i18n(" - ").length() ),
                        true
                    );
                else
                    CollectionDB::instance()->setCompilation( item->text(0), true );
                break;
            case COMPILATION_UNSET:
                if ( cat == CollectionBrowser::IdVisYearAlbum )
                    CollectionDB::instance()->setCompilation(
                        item->text(0).right( item->text(0).length() - item->text(0).find( i18n(" - ") ) - i18n(" - ").length() ),
                        false
                    );
                else
                    CollectionDB::instance()->setCompilation( item->text(0), false );
                break;
            case ORGANIZE:
                organizeFiles( listSelected(), false /* don't add to collection, just move */ );
                break;
            case DELETE:
                deleteSelectedFiles();
                break;
        }
    }
}


void
CollectionView::setViewMode( int mode, bool rerender )
{
    clear();

    // remove all columns
    for ( int i = columns() - 1; i >= 0 ; --i )
        removeColumn( i );

    if ( mode == modeTreeView )
    {
        QString headerText = captionForCategory( m_cat1 );
        if ( m_cat2 != CollectionBrowser::IdNone )
            headerText += " / " + captionForCategory( m_cat2 );
        if ( m_cat3 != CollectionBrowser::IdNone )
            headerText += " / " + captionForCategory( m_cat3 );

        addColumn( headerText );
        setResizeMode( QListView::LastColumn );
        setRootIsDecorated( true );
    }
    else
    {
        setRootIsDecorated( false );
        setResizeMode( QListView::NoColumn );

        addColumn( captionForTag( Title ) );
        addColumn( captionForTag( Artist ) );
        addColumn( captionForTag( Album ) );
        addColumn( captionForTag( Genre ), 0  );
        addColumn( captionForTag( Length ),0  );
        addColumn( captionForTag( Track ), 0 );
        addColumn( captionForTag( Year ), 0 );
        addColumn( captionForTag( Comment ), 0 );
        addColumn( captionForTag( Playcount ), 0 );
        addColumn( captionForTag( Score ), 0 );
        addColumn( captionForTag( Rating ), 0 );
        addColumn( captionForTag( Filename ), 0 );
        addColumn( captionForTag( Firstplay ), 0 );
        addColumn( captionForTag( Lastplay ), 0 );
        addColumn( captionForTag( Modified ), 0 );
        addColumn( captionForTag( Bitrate ), 0 );

        setColumnAlignment( Track, Qt::AlignCenter );
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

        //manage column widths
        QResizeEvent rev( size(), QSize() );
        viewportResizeEvent( &rev );
    }

    m_viewMode = mode;
    if ( rerender )
        renderView();
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
     if (selectedTracksNumber == 1) {
          TagDialog* dialog = new TagDialog( urls.first() );
          dialog->show();
     }
     else if (selectedTracksNumber)  {
          TagDialog* dialog = new TagDialog( urls, instance() );
          dialog->show();
     }
}

void
CollectionView::deleteSelectedFiles() //SLOT
{

     KURL::List urls = listSelected();

    const int count  = urls.count();
    QString text = i18n( "<p>You have selected one file to be <b>irreversibly</b> deleted.",
                     "<p>You have selected %n files to be <b>irreversibly</b> deleted.", count );

    int button = KMessageBox::warningContinueCancel( this,
                                                     text,
                                                     QString::null,
                                                     KStdGuiItem::del() );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );

        job->setAutoErrorHandlingEnabled( false );

        amaroK::StatusBar::instance()->newProgressOperation( job )
                .setDescription( i18n("Deleting files") );

        // we must handle delete errors somehow
        CollectionDB::instance()->removeSongs( urls );
        QTimer::singleShot( 0, this, SLOT( renderView() ) );
    }
}

void
CollectionView::organizeFiles( const KURL::List &urls, bool addToCollection )  //SLOT
{
    QStringList folders = AmarokConfig::collectionFolders();

    KDialogBase dialog( m_parent, 0, false );
    kapp->setTopWidget( &dialog );
    dialog.setCaption( kapp->makeStdCaption( i18n("Organize Collection Files") ) );
    dialog.showButtonApply( false );
    QVBox *box = dialog.makeVBoxMainWidget();

    QLabel *choice = new QLabel( box );
    choice->setText( i18n( "Choose collection folder:" ) );
    QComboBox *dirs = new QComboBox( false, box );
    dirs->setEditable( false );
    dirs->insertStringList( folders, 0 );
    dirs->setCurrentItem( AmarokConfig::organizeDirectory() );

    QCheckBox *overwrite = new QCheckBox( box );
    overwrite->setText( i18n( "Overwrite existing files" ) );
    overwrite->setChecked( AmarokConfig::overwriteFiles() );

    QCheckBox *byFiletype = new QCheckBox( box );
    byFiletype->setText( i18n( "Group files by type" ) );
    byFiletype->setChecked( AmarokConfig::groupByFiletype() );

    QCheckBox *group = new QCheckBox( box );
    group->setText( i18n( "Group artists alphabetically" ) );
    group->setChecked( AmarokConfig::groupArtists() );

    QCheckBox *replaceSpace = new QCheckBox( box );
    replaceSpace->setText( i18n( "Replace space with _" ) );
    replaceSpace->setChecked( AmarokConfig::replaceSpace() );

    QCheckBox *ignore = new QCheckBox( box );
    ignore->setText( i18n( "Ignore 'The' in artist names." ) );
    ignore->setChecked( AmarokConfig::ignoreThe() );

    QCheckBox *covers = new QCheckBox( box );
    covers->setText( i18n( "Use cover art for folder icons" ) );
    covers->setChecked( AmarokConfig::coverIcons() );

    if( dialog.exec() != QDialog::Rejected )
    {
        AmarokConfig::setOrganizeDirectory( dirs->currentItem() );
        AmarokConfig::setOverwriteFiles( overwrite->isChecked() );
        AmarokConfig::setGroupByFiletype( byFiletype->isChecked() );
        AmarokConfig::setGroupArtists( group->isChecked() );
        AmarokConfig::setIgnoreThe( ignore->isChecked() );
        AmarokConfig::setReplaceSpace( replaceSpace->isChecked() );
        AmarokConfig::setCoverIcons( covers->isChecked() );
        bool write = overwrite->isChecked();
        int skipped = 0;
        QString base = dirs->currentText() + "/";
        QString artist;
        QString album;
        QString title;
        QString dest;
        QString type;

        KURL::List::ConstIterator it = urls.begin();
        for( ; it != urls.end(); ++it )
        {
            KURL src = ( *it );

            //Building destination here.
            MetaBundle mb( src );

            if( !mb.artist().isEmpty() )
            {
                artist = cleanPath( mb.artist() );
                if( ignore->isChecked() && artist.startsWith( "the ", false ) )
                    manipulateThe( artist, true );
            }
            else
                artist = i18n("Unknown");

            mb.album().isEmpty() ?
                album = i18n("Unknown") :
                album = cleanPath( mb.album() );

            type = mb.type();

            if( !mb.title().isEmpty() )
            {
                if( mb.track() )
                {
                       if( (QString::number( mb.track() )).length() == 1 )
                               title = "0" + QString::number( mb.track() ) + " - " + cleanPath( mb.title() );
                       else
                       title = QString::number( mb.track() ) + " - " + cleanPath( mb.title() );
                }
                else
                {
                    title = cleanPath( mb.title() );
                }
                title.replace( type, "" );
            }
            else
                title = i18n("Unknown");

            dest = base;
            if( byFiletype->isChecked() )
                dest += src.path().section( ".", -1 ).lower() + "/";

            if( group->isChecked() )
                dest += artist.upper()[ 0 ] + "/";      // Group artists i.e. A/Artist/Album

            dest += artist + "/" + album + "/" + title + "." + type;
            dest.remove( "?" ).remove( ":" ).remove( "\"" ).remove( "," );

            if( replaceSpace->isChecked() )
                dest.replace( " ", "_" );

            debug() << "Destination: " << dest << endl;

            if( src.path() != dest ) //supress error warning that file couldn't be moved
            {
                if( !CollectionDB::instance()->moveFile( src.path(), dest, write, addToCollection ) )
                    skipped++;
            }

            //Use cover image for folder icon
            if( covers->isChecked() && !mb.artist().isEmpty() && !mb.album().isEmpty() )
            {
                KURL dstURL = KURL::fromPathOrURL( dest );
                dstURL.cleanPath();

                QString path  = dstURL.directory();
                QString cover = CollectionDB::instance()->albumImage( mb.artist(), mb.album(), 1 );

                if( !QFile::exists(path + "/.directory") && !cover.endsWith( "nocover.png" ) )
                {
                    //QPixmap thumb;        //Not amazon nice.
                    //if ( thumb.load( cover ) ){
                    //thumb.save(path + "/.front.png", "PNG", -1 ); //hide files

                    KSimpleConfig config(path + "/.directory");
                    config.setGroup("Desktop Entry");

                    if( !config.hasKey("Icon") )
                    {
                        //config.writeEntry("Icon", QString("%1/.front.png").arg( path ));
                        config.writeEntry( "Icon", cover );
                        config.sync();
                        debug() << "Using this cover as icon for: " << path << endl;
                        debug() << cover << endl;
                    }
                    //}         //Not amazon nice.
                }
            }
        }
        if( skipped > 0 )
            amaroK::StatusBar::instance()->longMessage( i18n(
                    "Sorry, one file could not be organized.",
                    "Sorry %n files could not be organized.", skipped ), KDE::StatusBar::Sorry );
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::updateColumnHeader()
{
    if ( m_viewMode == modeFlatView )
    {
        // remove all columns
        for ( int i = columns() - 1; i >= 0 ; --i )
            removeColumn( i );

        addColumn( i18n( "Title" ) );
        addColumn( captionForCategory( m_cat1 ) );
        if( m_cat2 != CollectionBrowser::IdNone ) addColumn( captionForCategory( m_cat2 ) );
        if( m_cat3 != CollectionBrowser::IdNone ) addColumn( captionForCategory( m_cat3 ) );

        setResizeMode( QListView::AllColumns );
    }
    else
    {
        setColumnText( 0, captionForCategory( m_cat1 ) );
        int catArr[2] = {m_cat2, m_cat3};

        for(int i = 0; i < 2; i++) {
            if (catArr[i] != CollectionBrowser::IdNone ) {
                setColumnText( 0, columnText(0) + " / " + captionForCategory( catArr[i] ) );
            }
        }
    }

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
    KURLDrag* d = new KURLDrag( listSelected(), this );
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
            bool sampler = item->text( 0 ) == i18n( "Various Artists" );
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            tmptext = item->text( 0 );
            QStringList matches( item->text( 0 ) );

            if( endsInThe( tmptext ) ) {
                manipulateThe( tmptext );
                matches << tmptext;
            }

            if ( !sampler )
            {
                if( VisYearAlbum == 1 )
                {
                    QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                    yearAlbumCalc( year, tmptext );
                    qb.addMatch( QueryBuilder::tabYear, year );
                    qb.addMatch( q_cat1, tmptext );
                }
                else
                    qb.addMatches( q_cat1, matches );
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
                    bool sampler = item->text( 0 ) == i18n( "Various Artists" );
                    qb.clear();
                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                    if ( !sampler )
                    {

                        tmptext = item->text( 0 );
                        QStringList matches( item->text( 0 ) );

                        if( endsInThe( tmptext ) ) {
                            manipulateThe( tmptext );
                            matches << tmptext;
                        }

                        if( VisYearAlbum == 1 )
                        {
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year );
                            qb.addMatch( q_cat1, tmptext );
                        }
                        else
                            qb.addMatches( q_cat1, matches );
                    }
                    else
                        qb.setOptions( QueryBuilder::optOnlyCompilations );


                    tmptext = child->text( 0 );
                    QStringList matches( child->text( 0 ) );

                    if( endsInThe( tmptext ) ) {
                        manipulateThe( tmptext );
                        matches << tmptext;
                    }

                    if( VisYearAlbum == 2 )
                    {
                        QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                        yearAlbumCalc( year, tmptext );
                        qb.addMatch( QueryBuilder::tabYear, year );
                        qb.addMatch( q_cat2, tmptext );
                    }
                    else
                        qb.addMatches( q_cat2, matches );

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
                        bool sampler = item->text( 0 ) == i18n( "Various Artists" );
                        qb.clear();
                        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                        if ( !sampler )
                        {

                            tmptext = item->text( 0 );
                            QStringList matches( item->text( 0 ) );

                            if( endsInThe( tmptext ) ) {
                                manipulateThe( tmptext );
                                matches << tmptext;
                            }

                            if( VisYearAlbum == 1 )
                            {
                                QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                                yearAlbumCalc( year, tmptext );
                                qb.addMatch( QueryBuilder::tabYear, year );
                                qb.addMatch( q_cat1, tmptext );
                            }
                            else
                                qb.addMatches( q_cat1, matches );
                        }
                        else
                            qb.setOptions( QueryBuilder::optOnlyCompilations );

                        tmptext = child->text( 0 );
                        QStringList matches( child->text( 0 ) );

                        if( endsInThe( tmptext ) ) {
                            manipulateThe( tmptext );
                            matches << tmptext;
                        }

                        if( VisYearAlbum == 2 )
                        {
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year );
                            qb.addMatch( q_cat2, tmptext );
                        }
                        else
                            qb.addMatches( q_cat2, matches );

                        matches.clear();
                        tmptext = grandChild->text( 0 );
                        matches << grandChild->text( 0 );

                        if( endsInThe( tmptext ) ) {
                            manipulateThe( tmptext );
                            matches << tmptext;
                        }

                        if( VisYearAlbum == 3 )
                        {
                            QString year = tmptext.left( tmptext.find( i18n(" - ") ) );
                            yearAlbumCalc( year, tmptext );
                            qb.addMatch( QueryBuilder::tabYear, year );
                            qb.addMatch( q_cat3, tmptext );
                        }
                        else {
                            matches.clear();
                            matches << grandChild->text( 0 ) << tmptext;

                            qb.addMatches( q_cat3, matches );
                        }
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
        case Track:     caption = i18n( "Track" );  break;
        case Year:      caption = i18n( "Year" );   break;
        case Comment:   caption = i18n( "Comment" ); break;
        case Playcount: caption = i18n( "Playcount" ); break;
        case Score:     caption = i18n( "Score" );  break;
        case Rating:    caption = i18n( "Rating" ); break;
        case Filename:  caption = i18n( "Filename" ); break;
        case Firstplay: caption = i18n( "First Play" ); break;
        case Lastplay:  caption = i18n( "Last Play" ); break;
        case Modified:  caption = i18n( "Modified Date" ); break;
        case Bitrate:   caption = i18n( "Bitrate" ); break;
        default: break;
    }
    return caption;
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


// Small function aimed to convert Eagles, The -> The Eagles
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
    if( year == "?" )
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
        && ( (QMouseEvent*)e )->button() == Qt::RightButton
        && m_viewMode == modeFlatView )
    {
        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertTitle( i18n( "Flat View Columns" ), /*id*/ -1, /*index*/ 1 );

        for ( int i = 0; i < columns(); ++i )
        {
            popup.insertItem( captionForTag( (Tag)i ), i );
            popup.setItemChecked( i, ( columnWidth(i) != 0 ) );

            //title column should always be shown
            if ( i == Title )
                popup.setItemEnabled ( i, false );
        }

        const int returnID = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        if ( returnID != -1 )
        {
            if ( columnWidth( returnID ) == 0 ) {
                adjustColumn( returnID );   // show column
                header()->setResizeEnabled( true, returnID );
                renderView();
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

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionItem
//////////////////////////////////////////////////////////////////////////////////////////
int
CollectionItem::compare( QListViewItem* i, int col, bool /* ascending */) const
{
    QString a =    text( col );
    QString b = i->text( col );
    int ia, ib;

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
                return 0;
            if (ia<ib)
                return 1;
            else
                return -1;

        // Various Artists is always the last one
        case CollectionBrowser::IdArtist:
            if ( b == i18n("Various Artists") )
                return -1;
            if ( a == i18n("Various Artists") )
                return 1;
        default:
        // Unknown is always the first one
            if ( a == i18n("Unknown") )
                return -1;
            if ( b == i18n("Unknown") )
                return 1;
    }
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


#include "collectionbrowser.moc"
