/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"         //foreach macro
#include "collectiondb.h"
#include "debug.h"
#include "statistics.h"

#include <kapplication.h>
#include <kdeversion.h>        //KDE_VERSION ifndefs.  Remove this once we reach a kde 4 dep
#include <kiconloader.h>
#include <klocale.h>
#include <kmultipledrag.h>     //startDrag()
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
#include <qvbox.h>


static inline
void albumArtistTrackFromUrl( QString url, QString &artist, QString &album )
{
    if ( !url.contains("@@@") ) return;

    const QStringList list = QStringList::split( " @@@ ", url, true );

    Q_ASSERT( !list.isEmpty() );

    artist = list[0];
    album  = list[1];
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS Statistics
//////////////////////////////////////////////////////////////////////////////////////////

Statistics *Statistics::s_instance = 0;

Statistics::Statistics( QWidget *parent, const char *name )
    : KDialogBase( KDialogBase::Swallow, 0, parent, name, false, 0, Close )
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

    QHBox *box = new QHBox( mainWidget() );
    box->setSpacing( 5 );

    m_listview = new StatisticsList( box );
}

Statistics::~Statistics()
{
    s_instance = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS StatisticsList
//////////////////////////////////////////////////////////////////////////////////////////

StatisticsList::StatisticsList( QWidget *parent, const char *name )
    : KListView( parent, name )
    , m_currentItem( 0 )
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

    if( CollectionDB::instance()->isEmpty() )
        return;

    initDisplay();
}

void
StatisticsList::startDrag()
{
    DEBUG_FUNC_INFO

    KURL::List list;
    QueryBuilder qb;

    QString artist, album;

    KMultipleDrag *drag = new KMultipleDrag( this );

    QListViewItemIterator it( this, QListViewItemIterator::Selected );

    for( ; it.current(); ++it )
    {
        StatisticsDetailedItem *item = static_cast<StatisticsDetailedItem*>(*it);

        debug() << "url: " << item->url() << endl;
        albumArtistTrackFromUrl( item->url(), artist, album );

        if( item->itemType() == StatisticsDetailedItem::TRACK )
        {
            list += KURL::fromPathOrURL( item->url() );
        }

        else if( item->itemType() == StatisticsDetailedItem::ALBUM )
        {
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artist );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            QStringList values = qb.run();

            QTextDrag *textdrag = new QTextDrag( qb.query(), 0 );
            textdrag->setSubtype( "amarok-sql" );
            drag->addDragObject( textdrag );
        }

        else if( item->itemType() == StatisticsDetailedItem::ARTIST )
        {
            const uint artist_id = CollectionDB::instance()->artistID( item->url() );

            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            QStringList values = qb.run();

            QTextDrag *textdrag = new QTextDrag( qb.query(), 0 );
            textdrag->setSubtype( "amarok-sql" );
            drag->addDragObject( textdrag );
        }

        else if( item->itemType() == StatisticsDetailedItem::GENRE )
        {
            const uint genre_id = CollectionDB::instance()->genreID( item->url() );

            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valGenreID, QString::number( genre_id ) );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            QStringList values = qb.run();

            QTextDrag *textdrag = new QTextDrag( qb.query(), 0 );
            textdrag->setSubtype( "amarok-sql" );
            drag->addDragObject( textdrag );
        }
    }

    clearSelection();

    drag->addDragObject( new KURLDrag( list, viewport() ) );
    drag->setPixmap(CollectionDB::createDragPixmap(list), QPoint(CollectionDB::DRAGPIXMAP_OFFSET_X,CollectionDB::DRAGPIXMAP_OFFSET_Y));
    drag->dragCopy();
}

void
StatisticsList::initDisplay()
{
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
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_artistItem = new StatisticsItem( i18n("Favorite Artists"), this, m_mostplayedItem );
    m_artistItem->setSubtext( i18n("%n artist", "%n artists", a[0].toInt()) );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_albumItem = new StatisticsItem( i18n("Favorite Albums"), this, m_artistItem );
    m_albumItem->setSubtext( i18n("%n album", "%n albums", a[0].toInt()) );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_genreItem = new StatisticsItem( i18n("Favorite Genres"), this, m_albumItem );
    m_genreItem->setSubtext( i18n("%n genre", "%n genres", a[0].toInt()) );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcMin, QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();
    QDateTime firstPlay = QDateTime();
    firstPlay.setTime_t( a[0].toUInt() );

    m_newestItem = new StatisticsItem( i18n("Newest Items"), this, m_genreItem );
    m_newestItem->setSubtext( i18n("Listening since %1").arg( amaroK::verboseTimeSince( firstPlay ) ) );

    m_trackItem ->setPixmap( QString("sound") );
    m_mostplayedItem->setPixmap( QString("favorites") );
    m_artistItem->setPixmap( QString("personal") );
    m_albumItem ->setPixmap( QString("cdrom_unmount") );
    m_genreItem ->setPixmap( QString("kfm") );
    m_newestItem->setPixmap( QString("history") );
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
        initDisplay();
        return;
    }

    expandInformation( item );
    item->setOpen( true );

    #undef item
}

void
StatisticsList::expandInformation( StatisticsItem *item )
{
    QueryBuilder qb;

    StatisticsDetailedItem *m_last = 0;
    uint c = 1;

    if( item == m_trackItem )
    {
        delete m_newestItem;
        delete m_genreItem;
        delete m_albumItem;
        delete m_artistItem;
        delete m_mostplayedItem;

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::TRACK );
            m_last->setUrl( fave[i+2] );
            c++;
        }
    }

    else if( item == m_mostplayedItem )
    {
        delete m_newestItem;
        delete m_genreItem;
        delete m_albumItem;
        delete m_artistItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::TRACK );
            m_last->setUrl( fave[i+2] );
            c++;
        }
    }

    else if( item == m_artistItem )
    {
        delete m_newestItem;
        delete m_genreItem;
        delete m_albumItem;
        delete m_mostplayedItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2").arg( QString::number(c), fave[i] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::ARTIST );
            QString url = QString("%1").arg( fave[i] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_albumItem )
    {
        delete m_newestItem;
        delete m_genreItem;
        delete m_artistItem;
        delete m_mostplayedItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);

        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::ALBUM );
            QString url = QString("%1 @@@ %2").arg( fave[i+2], fave[i+3] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_genreItem )
    {
        delete m_newestItem;
        delete m_albumItem;
        delete m_artistItem;
        delete m_mostplayedItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2").arg( QString::number(c), fave[i] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::GENRE );
            QString url = QString("%1").arg( fave[i] );
            m_last->setUrl( url );
            c++;
        }
    }

    else if( item == m_newestItem )
    {
        delete m_genreItem;
        delete m_albumItem;
        delete m_artistItem;
        delete m_mostplayedItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate );
        qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList newest = qb.run();

        for( uint i=0; i < newest.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3").arg( QString::number(c), newest[i], newest[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            m_last->setItemType( StatisticsDetailedItem::HISTORY );
            QString url = QString("%1 @@@ %2").arg( newest[i+2], newest[i+3] );
            m_last->setUrl( url );
            c++;
        }
    }

    item->setExpanded( true );
    repaintItem( item );  // Better than ::repaint(), flickers less
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
StatisticsItem::setPixmap( const QString &pix )
{
    KIconLoader iconloader;
    QPixmap icon = iconloader.loadIcon( pix, KIcon::Desktop, KIcon::SizeHuge );
    KListViewItem::setPixmap( 0, icon );
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

    KListView *lv = (KListView *)listView();

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
        int ellWidth = fmName.width( i18n("...") );
        QString text = QString::fromLatin1("");
        int i = 0;
        int len = name.length();
        while ( i < len && fmName.width( text + name[ i ] ) + ellWidth < width - text_x - lv->itemMargin()*2  ) {
            text += name[ i ];
            i++;
        }
        name = text + i18n("...");
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

StatisticsDetailedItem::StatisticsDetailedItem( QString &text, StatisticsItem *parent,
                                                StatisticsDetailedItem *after, const char *name )
    : KListViewItem( parent, after, name )
    , m_type( NONE )
{
    setDragEnabled( true );
    setDropEnabled( false );
    setSelectable( true );

    setText( 0, text );
}

#include "statistics.moc"

