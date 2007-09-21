/***************************************************************************
 * copyright            : (C) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"             //foreach macro
#include "browserToolBar.h"     //search toolbar
#include "clicklineedit.h"
#include "collectiondb.h"
#include "debug.h"
#include "playlist.h"
#include "statistics.h"
#include "tagdialog.h"         //showContextMenu()

#include <kapplication.h>
#include <kdeversion.h>        //KDE_VERSION ifndefs.  Remove this once we reach a kde 4 dep
#include <kiconloader.h>
#include <klocale.h>
#include <kmultipledrag.h>     //startDrag()
#include <kpopupmenu.h>
#include <kstringhandler.h>    //paintCell
#include <ktoolbarbutton.h>    //ctor
#include <kurldrag.h>          //startDrag()
#include <kwin.h>

#include <qcolor.h>
#include <qdatetime.h>
#include <qheader.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qvbox.h>

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS Statistics
//////////////////////////////////////////////////////////////////////////////////////////

Statistics *Statistics::s_instance = 0;

Statistics::Statistics( QWidget *parent, const char *name )
    : KDialogBase( KDialogBase::Swallow, 0, parent, name, false, 0, Close )
    , m_timer( new QTimer( this ) )
{
    s_instance = this;

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n("Collection Statistics") ) );
    setInitialSize( QSize( 400, 550 ) );

    QVBox *mainBox = new QVBox( this );
    setMainWidget( mainBox );

    QVBox *box = new QVBox( mainWidget() );
    box->setSpacing( 5 );

    { //<Search LineEdit>
        KToolBar *bar = new Browser::ToolBar( box );
        bar->setIconSize( 22, false ); //looks more sensible
        bar->setFlat( true ); //removes the ugly frame
        bar->setMovingEnabled( false ); //removes the ugly frame

        QWidget *button = new KToolBarButton( "locationbar_erase", 1, bar );
        m_lineEdit = new ClickLineEdit( i18n( "Enter search terms here" ), bar );

        bar->setStretchableWidget( m_lineEdit );
        m_lineEdit->setFrame( QFrame::Sunken );
        m_lineEdit->installEventFilter( this ); //we intercept keyEvents

        connect( button,     SIGNAL( clicked() )      , m_lineEdit  , SLOT( clear() ) );
        connect( m_timer,    SIGNAL( timeout() )                    , SLOT( slotSetFilter() ) );
        connect( m_lineEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
        connect( m_lineEdit, SIGNAL( returnPressed() )              , SLOT( slotSetFilter() ) );

        QToolTip::add( button, i18n( "Clear search field" ) );
    } //</Search LineEdit>

    m_listView = new StatisticsList( box );
}

Statistics::~Statistics()
{
    s_instance = 0;
}

void
Statistics::slotSetFilterTimeout() //SLOT
{
    m_timer->start( 280, true ); //stops the timer for us first
}

void
Statistics::slotSetFilter() //SLOT
{
    m_timer->stop();
    m_listView->setFilter( m_lineEdit->text() );
    if( m_listView->childCount() > 1 )
        m_listView->renderView();
    else
        m_listView->refreshView();
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS StatisticsList
//////////////////////////////////////////////////////////////////////////////////////////

StatisticsList::StatisticsList( QWidget *parent, const char *name )
    : KListView( parent, name )
    , m_currentItem( 0 )
    , m_expanded( false )
{
    header()->hide();

    addColumn( i18n("Name") );
    setResizeMode( QListView::LastColumn );
    setSelectionMode( QListView::Extended );
    setSorting( -1 );

    setAcceptDrops( false );
    setDragEnabled( true );

    connect( this, SIGNAL( onItem( QListViewItem*) ),  SLOT( startHover( QListViewItem* ) ) );
    connect( this, SIGNAL( onViewport() ),             SLOT( clearHover() ) );
    connect( this, SIGNAL( clicked( QListViewItem*) ), SLOT( itemClicked( QListViewItem* ) ) );
    connect( this, SIGNAL( contextMenuRequested( QListViewItem *, const QPoint &, int ) ),
             this,   SLOT( showContextMenu( QListViewItem *, const QPoint &, int )  ) );

    if( CollectionDB::instance()->isEmpty() )
        return;

    renderView();
}

void
StatisticsList::startDrag()
{
    // there is only one item ever selected in this tool.  maybe this needs to change

    DEBUG_FUNC_INFO

    KURL::List list;
    KMultipleDrag *drag = new KMultipleDrag( this );

    QListViewItemIterator it( this, QListViewItemIterator::Selected );

    StatisticsDetailedItem *item = dynamic_cast<StatisticsDetailedItem*>(*it);

    if ( !item )
        return;

    if( item->itemType() == StatisticsDetailedItem::TRACK )
    {
        list += KURL::fromPathOrURL( item->url() );
        drag->addDragObject( new KURLDrag( list, viewport() ) );
        drag->setPixmap( CollectionDB::createDragPixmap(list),
                         QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X,
                                 CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    }
    else
    {
        QTextDrag *textdrag = new QTextDrag( '\n' + item->getSQL(), 0 );
        textdrag->setSubtype( "amarok-sql" );
        drag->addDragObject( textdrag );
        drag->setPixmap( CollectionDB::createDragPixmapFromSQL( item->getSQL() ),
                         QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X,
                                 CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    }

    clearSelection();
    drag->dragCopy();
}

void
StatisticsList::refreshView()
{
    if( m_expanded )
    {
        if( !firstChild() )
        {
            error() << "Statistics: uh oh, no first child!" << endl;
            return;
        }
        while( firstChild()->firstChild() )
            delete firstChild()->firstChild();

        expandInformation( static_cast<StatisticsItem*>(firstChild()), true /*refresh*/ );
    }
    else
        renderView();
}

void
StatisticsList::renderView()
{
    m_expanded = false;

    //ensure cleanliness - this function is not just called from the ctor, but also when returning to the initial display
    while( firstChild() )
        delete firstChild();
    m_currentItem = 0;

    QueryBuilder qb;
    QStringList a;

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_trackItem = new StatisticsItem( i18n("Favorite Tracks"), this, 0 );
    m_trackItem->setSubtext( i18n("%n track", "%n tracks", a[0].toInt()) );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcSum, QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    a = qb.run();

    m_mostplayedItem = new StatisticsItem( i18n("Most Played Tracks"), this, m_trackItem );
    m_mostplayedItem->setSubtext( i18n("%n play", "%n plays", a[0].toInt()) );

    qb.clear();
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valArtistID );
    //I can't get the correct value w/o using a subquery, and querybuilder doesn't support those
    a = QString::number( qb.run().count() );

    m_artistItem = new StatisticsItem( i18n("Favorite Artists"), this, m_mostplayedItem );
    m_artistItem->setSubtext( i18n("%n artist", "%n artists", a[0].toInt()) );

    qb.clear();
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valAlbumID );
    //I can't get the correct value w/o using a subquery, and querybuilder doesn't support those
    a = QString::number( qb.run().count() );

    m_albumItem = new StatisticsItem( i18n("Favorite Albums"), this, m_artistItem );
    m_albumItem->setSubtext( i18n("%n album", "%n albums", a[0].toInt()) );

    qb.clear();
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valGenreID );
    //I can't get the correct value w/o using a subquery, and querybuilder doesn't support those
    a = QString::number( qb.run().count() );

    m_genreItem = new StatisticsItem( i18n("Favorite Genres"), this, m_albumItem );
    m_genreItem->setSubtext( i18n("%n genre", "%n genres", a[0].toInt()) );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcMin, QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();
    QDateTime firstPlay = QDateTime::currentDateTime();
    if ( a[0].toUInt() )
        firstPlay.setTime_t( a[0].toUInt() );

    m_newestItem = new StatisticsItem( i18n("Newest Items"), this, m_genreItem );
    m_newestItem->setSubtext( i18n("First played %1").arg( Amarok::verboseTimeSince( firstPlay ) ) );

    m_trackItem     ->setIcon( Amarok::icon("track") );
    m_mostplayedItem->setIcon( Amarok::icon("mostplayed") );
    m_artistItem    ->setIcon( Amarok::icon("artist") );
    m_albumItem     ->setIcon( Amarok::icon("album") );
    m_genreItem     ->setIcon( Amarok::icon("favourite_genres") );
    m_newestItem    ->setIcon( Amarok::icon("clock") );
}

void
StatisticsList::itemClicked( QListViewItem *item ) //SLOT
{
    if( !item )
        return;

    if( item->depth() != 0 ) //not very flexible, *shrug*
        return;

    #define item static_cast<StatisticsItem*>(item)

    if( item->isExpanded() )
    {
        renderView();
        return;
    }

    expandInformation( item );
    item->setOpen( true );

    #undef item
}

void
StatisticsList::expandInformation( StatisticsItem *item, bool refresh )
{
    m_expanded = true;
    KLocale *locale = new KLocale( "locale" );

    QueryBuilder qb;

    StatisticsDetailedItem *m_last = 0;
    uint c = 1;

    if( item == m_trackItem )
    {
        if( !refresh ) {
            delete m_newestItem;
            delete m_genreItem;
            delete m_albumItem;
            delete m_artistItem;
            delete m_mostplayedItem;
        }

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
        qb.addNumericFilter( QueryBuilder::tabStats, QueryBuilder::valForFavoriteSorting(), "0", QueryBuilder::modeGreater );
        qb.setGoogleFilter( QueryBuilder::tabSong | QueryBuilder::tabArtist, m_filter );
        qb.sortByFavorite();
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c),
                    fave[i].isEmpty() ? i18n( "Unknown" ) : fave[i],
                    fave[i+1].isEmpty() ? i18n( "Unknown" ) : fave[i+1]);
            QString score = locale->formatNumber( fave[i+3].toDouble(), 0 );
            QString rating = locale->formatNumber( fave[i+4].toDouble() / 2.0, 1 );
            m_last = new StatisticsDetailedItem( name, subText( score, rating ), item, m_last );
            m_last->setItemType( StatisticsDetailedItem::TRACK );
            m_last->setUrl( fave[i+2] );
            c++;
        }
    }

    else if( item == m_mostplayedItem )
    {
        if( !refresh ) {
            delete m_newestItem;
            delete m_genreItem;
            delete m_albumItem;
            delete m_artistItem;
            delete m_trackItem;
        }

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
        qb.addNumericFilter( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, "0", QueryBuilder::modeGreater );
        qb.setGoogleFilter( QueryBuilder::tabSong | QueryBuilder::tabArtist, m_filter );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c),
                    fave[i].isEmpty() ? i18n( "Unknown" ) : fave[i],
                    fave[i+1].isEmpty() ? i18n( "Unknown" ) : fave[i+1]);
            double plays  = fave[i+3].toDouble();
            QString subtext = i18n("%1: %2").arg( i18n( "Playcount" ) ).arg( plays );
            m_last = new StatisticsDetailedItem( name, subtext, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::TRACK );
            m_last->setUrl( fave[i+2] );
            c++;
        }
    }

    else if( item == m_artistItem )
    {
        if( !refresh ) {
            delete m_newestItem;
            delete m_genreItem;
            delete m_albumItem;
            delete m_mostplayedItem;
            delete m_trackItem;
        }

        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valRating );
        qb.sortByFavoriteAvg();
        // only artists with more than 3 tracks
        qb.having( QueryBuilder::tabArtist, QueryBuilder::valID, QueryBuilder::funcCount, QueryBuilder::modeGreater, "3" );
        qb.setGoogleFilter( QueryBuilder::tabArtist, m_filter );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name   = i18n("%1. %2").arg( QString::number(c),
                    fave[i].isEmpty() ? i18n( "Unknown" ) : fave[i] );
            QString score  = locale->formatNumber( fave[i+1].toDouble(), 2 );
            QString rating = locale->formatNumber( fave[i+2].toDouble() / 2.0, 2 );
            m_last = new StatisticsDetailedItem( name, subText( score, rating ), item, m_last );
            m_last->setItemType( StatisticsDetailedItem::ARTIST );
            QString url = QString("%1").arg( fave[i] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_albumItem )
    {
        if( !refresh ) {
            delete m_newestItem;
            delete m_genreItem;
            delete m_artistItem;
            delete m_mostplayedItem;
            delete m_trackItem;
        }

        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valRating );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );
        // only albums with more than 3 tracks
        qb.having( QueryBuilder::tabAlbum, QueryBuilder::valID, QueryBuilder::funcCount, QueryBuilder::modeGreater, "3" );
        qb.setOptions( QueryBuilder::optNoCompilations ); // samplers __need__ to be handled differently
        qb.setGoogleFilter( QueryBuilder::tabAlbum | QueryBuilder::tabArtist, m_filter );
        qb.sortByFavoriteAvg();
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.groupBy( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );

        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        const QString trueValue = CollectionDB::instance()->boolT();
        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            const bool isSampler = (fave[i+6] == trueValue);
            QString name = i18n("%1. %2 - %3").arg( QString::number(c),
                    fave[i].isEmpty() ? i18n( "Unknown" ) : fave[i],
                    isSampler ? i18n( "Various Artists" ) :
                        ( fave[i+1].isEmpty() ? i18n( "Unknown" ) : fave[i+1] ) );
            QString score = locale->formatNumber( fave[i+4].toDouble(), 2 );
            QString rating = locale->formatNumber( fave[i+5].toDouble() / 2.0, 2 );

            m_last = new StatisticsDetailedItem( name, subText( score, rating ), item, m_last );
            m_last->setItemType( StatisticsDetailedItem::ALBUM );
            QString url = QString("%1 @@@ %2").arg( isSampler ? "0" : fave[i+2], fave[i+3] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_genreItem )
    {
        if( !refresh ) {
            delete m_newestItem;
            delete m_albumItem;
            delete m_artistItem;
            delete m_mostplayedItem;
            delete m_trackItem;
        }

        qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valRating );
        // only genres with more than 3 tracks
        qb.having( QueryBuilder::tabGenre, QueryBuilder::valID, QueryBuilder::funcCount, QueryBuilder::modeGreater, "3" );
        // only genres which have been played/rated
        qb.setGoogleFilter( QueryBuilder::tabGenre, m_filter );
        qb.sortByFavoriteAvg();
        qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2").arg( QString::number(c),
                    fave[i].isEmpty() ? i18n( "Unknown" ) : fave[i] );
            QString score  = locale->formatNumber( fave[i+1].toDouble(), 2 );
            QString rating = locale->formatNumber( fave[i+2].toDouble() / 2.0, 2 );

            m_last = new StatisticsDetailedItem( name, subText( score, rating ), item, m_last );
            m_last->setItemType( StatisticsDetailedItem::GENRE );
            QString url = QString("%1").arg( fave[i] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_newestItem )
    {
        if( !refresh ) {
            delete m_genreItem;
            delete m_albumItem;
            delete m_artistItem;
            delete m_mostplayedItem;
            delete m_trackItem;
        }

        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addReturnFunctionValue( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate );
        qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.setGoogleFilter( QueryBuilder::tabAlbum | QueryBuilder::tabArtist, m_filter );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valID);
        qb.setOptions( QueryBuilder::optNoCompilations ); // samplers __need__ to be handled differently
        qb.setLimit( 0, 50 );
        QStringList newest = qb.run();

        for( uint i=0; i < newest.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c),
                    newest[i].isEmpty() ? i18n( "Unknown" ) : newest[i],
                    newest[i+1].isEmpty() ? i18n( "Unknown" ) : newest[i+1] );
            QDateTime added = QDateTime();
            added.setTime_t( newest[i+4].toUInt() );
            QString subtext = i18n("Added: %1").arg( Amarok::verboseTimeSince( added ) );
            m_last = new StatisticsDetailedItem( name, subtext, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::HISTORY );
            QString url = QString("%1 @@@ %2").arg( newest[i+2] ).arg( newest[i+3] );
            m_last->setUrl( url );
            c++;
        }
    }

    item->setExpanded( true );
    repaintItem( item );  // Better than ::repaint(), flickers less
    delete locale;
}

QString StatisticsList::subText( const QString &score, const QString &rating ) //static
{
    if( AmarokConfig::useScores() && AmarokConfig::useRatings() )
        return i18n( "Score: %1 Rating: %2" ).arg( score ).arg( rating );
    else if( AmarokConfig::useScores() )
        return i18n( "Score: %1" ).arg( score );
    else if( AmarokConfig::useRatings() )
        return i18n( "Rating: %1" ).arg( rating );
    else
        return QString();
}

void
StatisticsList::startHover( QListViewItem *item ) //SLOT
{
    if( m_currentItem && item != m_currentItem )
        static_cast<StatisticsItem*>(m_currentItem)->leaveHover();

    if( item->depth() != 0 )
    {
        m_currentItem = 0;
        return;
    }

    static_cast<StatisticsItem*>(item)->enterHover();
    m_currentItem = item;
}

void
StatisticsList::clearHover() //SLOT
{
    if( m_currentItem )
        static_cast<StatisticsItem*>(m_currentItem)->leaveHover();

    m_currentItem = 0;
}

void
StatisticsList::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e );

    if( CollectionDB::instance()->isEmpty() && e )
    {
        QPainter p( viewport() );
        QString minimumText(i18n(
                "<div align=center>"
                "<h3>Statistics</h3>"
                    "You need a collection to use statistics!  "
                    "Create a collection and then start playing  "
                    "tracks to accumulate data on your play habits!"
                "</div>" ) );
        QSimpleRichText t( minimumText, QApplication::font() );

        if ( t.width()+30 >= viewport()->width() || t.height()+30 >= viewport()->height() )
            //too big, giving up
            return;

        const uint w = t.width();
        const uint h = t.height();
        const uint x = (viewport()->width() - w - 30) / 2 ;
        const uint y = (viewport()->height() - h - 30) / 2 ;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
        t.draw( &p, x+15, y+15, QRect(), colorGroup() );
    }
}

void
StatisticsList::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{
    if( !item || item->rtti() == StatisticsItem::RTTI ) return;

#define item static_cast<StatisticsDetailedItem*>(item)

    bool hasSQL = !( item->itemType() == StatisticsDetailedItem::TRACK ); //track is url

    KPopupMenu menu( this );
    enum Actions { APPEND, QUEUE, INFO };

    menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
    menu.insertItem( SmallIconSet( Amarok::icon( "queue_track" ) ), i18n( "&Queue Track" ), QUEUE );

    menu.insertSeparator();

    menu.insertItem( SmallIconSet( Amarok::icon( "info" ) ), i18n( "Edit Track &Information..." ), INFO );

    switch( menu.exec( p ) )
    {
        case APPEND:
            hasSQL ?
                Playlist::instance()->insertMediaSql( item->getSQL() ):
                Playlist::instance()->insertMedia( KURL::fromPathOrURL( item->url() ) );
            break;

        case QUEUE:
            hasSQL ?
                Playlist::instance()->insertMediaSql( item->getSQL(), Playlist::Queue ):
                Playlist::instance()->insertMedia( KURL::fromPathOrURL( item->url() ), Playlist::Queue );
            break;

        case INFO:
            if( hasSQL )
            {
                TagDialog* dialog = new TagDialog( item->getURLs(), Statistics::instance() );
                dialog->show();
            }
            else
            {
                TagDialog* dialog = new TagDialog( KURL::fromPathOrURL( item->url() ), Statistics::instance() );
                dialog->show();
            }
    }
#undef item
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS StatisticsItem
//////////////////////////////////////////////////////////////////////////////////////////

StatisticsItem::StatisticsItem( QString text, StatisticsList *parent, KListViewItem *after, const char *name )
    : KListViewItem( static_cast<KListView*>(parent), after, name )
    , m_animTimer( new QTimer( this ) )
    , m_animCount( 0 )
    , m_isActive( false )
    , m_isExpanded( false )
{
    setDragEnabled( false );
    setDropEnabled( false );
    setSelectable( false );

    setText( 0, text );

    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
}

void
StatisticsItem::setIcon( const QString &icon )
{
    QString path = kapp->iconLoader()->iconPath( icon, -KIcon::SizeHuge );
    path.replace( "32x32", "48x48" ); //HACK fucking KIconLoader only returns 32x32 max. Why?

//     debug() << "ICONPATH: " << path << endl;

    setPixmap( 0, path );
}

void
StatisticsItem::enterHover()
{
    m_animEnter = true;
    m_animCount = 0;
    m_isActive = true;
    m_animTimer->start( ANIM_INTERVAL );
}

void
StatisticsItem::leaveHover()
{
    // This can happen if you enter and leave the tab quickly
    if( m_animCount == 0 )
        m_animCount = 1;

    m_animEnter = false;
    m_isActive = true;
    m_animTimer->start( ANIM_INTERVAL );
}

void
StatisticsItem::slotAnimTimer()
{
    if( m_animEnter )
    {
        m_animCount += 1;
        listView()->repaintItem( this );  // Better than ::repaint(), flickers less

        if( m_animCount >= ANIM_MAX )
            m_animTimer->stop();
    }
    else
    {
        m_animCount -= 1;
        listView()->repaintItem( this );
        if( m_animCount <= 0 )
        {
            m_animTimer->stop();
            m_isActive = false;
        }
    }
}

void
StatisticsItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QColor fillColor, textColor;

    if( m_isActive ) //glowing animation
    {
        fillColor = blendColors( cg.background(), cg.highlight(), static_cast<int>( m_animCount * 3.5 ) );
        textColor = blendColors( cg.text(), cg.highlightedText(), static_cast<int>( m_animCount * 4.5 ) );
    }
    else //alternate colours
    {
    #if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
        fillColor = isSelected() ? cg.highlight() : backgroundColor();
    #else
        fillColor = isSelected() ? cg.highlight() : backgroundColor(0);
    #endif
        textColor = isSelected() ? cg.highlightedText() : cg.text();
    }

    //flicker-free drawing
    static QPixmap buffer;

    buffer.resize( width, height() );

    if( buffer.isNull() )
    {
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    buffer.fill( fillColor );

    QPainter pBuf( &buffer, true );

    KListView *lv = static_cast<KListView *>( listView() );

    QFont font( p->font() );
    font.setBold( true );
    QFontMetrics fm( p->fontMetrics() );

    int textHeight = height();
    int text_x = 0;

    pBuf.setPen( textColor );

    if( pixmap( column ) )
    {
        int y = (textHeight - pixmap(column)->height())/2;
        pBuf.drawPixmap( 0, y, *pixmap(column) );
        text_x += pixmap(column)->width() + 4;
    }

    pBuf.setFont( font );
    QFontMetrics fmName( font );

    QString name = text(column);
    if( fmName.width( name ) + text_x + lv->itemMargin()*2 > width )
    {
        const int _width = width - text_x - lv->itemMargin()*2;
        name = KStringHandler::rPixelSqueeze( name, pBuf.fontMetrics(), _width );
    }

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( !m_subText.isEmpty() )
    {
        font.setBold( false );
        pBuf.setFont( font );

        pBuf.drawText( text_x, fmName.height() + 1, width, textHeight, AlignVCenter, m_subText );
    }

    if( m_isExpanded )
    {
        QPen pen( cg.highlight(), 1 );
        pBuf.setPen( pen );
        int y = textHeight - 1;
        pBuf.drawLine( 0, y, width, y );
    }

    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}

QColor
StatisticsItem::blendColors( const QColor& color1, const QColor& color2, int percent )
{
    const float factor1 = ( 100 - ( float ) percent ) / 100;
    const float factor2 = ( float ) percent / 100;

    const int r = static_cast<int>( color1.red()   * factor1 + color2.red()   * factor2 );
    const int g = static_cast<int>( color1.green() * factor1 + color2.green() * factor2 );
    const int b = static_cast<int>( color1.blue()  * factor1 + color2.blue()  * factor2 );

    QColor result;
    result.setRgb( r, g, b );

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS StatisticsDetailedItem
//////////////////////////////////////////////////////////////////////////////////////////

StatisticsDetailedItem::StatisticsDetailedItem( const QString &text, const QString &subtext, StatisticsItem *parent,
                                                StatisticsDetailedItem *after, const char *name )
    : KListViewItem( parent, after, name )
    , m_type( NONE )
    , m_subText( subtext )
{
    setDragEnabled( true );
    setDropEnabled( false );
    setSelectable( true );

    setText( 0, text );
}

void
StatisticsDetailedItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    bool showDetails = !m_subText.isEmpty();

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
#if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );
#else
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor(0) );
#endif

    KListView *lv = static_cast<KListView *>( listView() );

    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );

    int text_x = 0;
    int textHeight;

    if( showDetails )
        textHeight = fm.lineSpacing() + lv->itemMargin() + 1;
    else
        textHeight = height();

    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );

    if( pixmap( column ) )
    {
        int y = (textHeight - pixmap(column)->height())/2;
        if( showDetails ) y++;
        pBuf.drawPixmap( text_x, y, *pixmap(column) );
        text_x += pixmap(column)->width() + 4;
    }

    pBuf.setFont( font );
    QFontMetrics fmName( font );

    QString name = text(column);
    const int _width = width - text_x - lv->itemMargin()*2;
    if( fmName.width( name ) > _width )
    {
        name = KStringHandler::rPixelSqueeze( name, pBuf.fontMetrics(), _width );
    }

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( showDetails )
    {
        const QColorGroup _cg = listView()->palette().disabled();
        text_x = lv->treeStepSize() + 3;
        font.setItalic( true );
        pBuf.setPen( isSelected() ? _cg.highlightedText() : _cg.text().dark() );
        pBuf.drawText( text_x, textHeight, width, fm.lineSpacing(), AlignVCenter, m_subText );
    }

    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}

void
StatisticsDetailedItem::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = fm.lineSpacing();
    if ( h % 2 > 0 )
        h++;
    if( !m_subText.isEmpty() )
        setHeight( h + fm.lineSpacing() + margin );
    else
        setHeight( h + margin );
}

QString
StatisticsDetailedItem::getSQL()
{
    QueryBuilder qb;
    QString query = QString::null;
    QString artist, album, track;   // track is unused here
    Amarok::albumArtistTrackFromUrl( url(), artist, album, track );

    if( itemType() == StatisticsDetailedItem::ALBUM || itemType() == StatisticsDetailedItem::HISTORY )
    {
        qb.initSQLDrag();
        if ( artist != "0" )
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artist );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }

    else if( itemType() == StatisticsDetailedItem::ARTIST )
    {
        const uint artist_id = CollectionDB::instance()->artistID( url() );

        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }

    else if( itemType() == StatisticsDetailedItem::GENRE )
    {
        const uint genre_id = CollectionDB::instance()->genreID( url() );

        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valGenreID, QString::number( genre_id ) );
        qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }
    debug() << "DetailedStatisticsItem: query is: " << qb.query() << endl;

    return qb.query();
}

KURL::List
StatisticsDetailedItem::getURLs()
{
    if( itemType() == StatisticsDetailedItem::TRACK )
        return KURL::List( KURL::fromPathOrURL(url()) );

    QueryBuilder qb;
    QString query = QString::null;
    QString artist, album, track;   // track is unused here
    Amarok::albumArtistTrackFromUrl( m_url, artist, album, track );

    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

    if( itemType() == StatisticsDetailedItem::ALBUM || itemType() == StatisticsDetailedItem::HISTORY )
    {
        if ( artist != "0" )
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artist );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }

    else if( itemType() == StatisticsDetailedItem::ARTIST )
    {
        const uint artist_id = CollectionDB::instance()->artistID( url() );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }

    else if( itemType() == StatisticsDetailedItem::GENRE )
    {
        const uint genre_id = CollectionDB::instance()->genreID( url() );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valGenreID, QString::number( genre_id ) );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    }

    QStringList values = qb.run();
    KURL::List urls;
    foreach( values )
        urls += KURL::fromPathOrURL( *it );
    return urls;
}

#include "statistics.moc"

