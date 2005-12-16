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
#include <kwin.h>

#include <qcolor.h>
#include <qheader.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsimplerichtext.h>
#include <qtimer.h>
#include <qvbox.h>

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
    setCaption( kapp->makeStdCaption( i18n("Statistics") ) );
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
StatisticsList::initDisplay()
{
    //ensure cleanliness - this function is not just called from the ctor, but also when returning to the initial display
    while( firstChild() )
        delete firstChild();

    QueryBuilder qb;
    QStringList a;
//     qb.addReturnFunctionValue( QueryBuilder::funcSum, QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
//     a = qb.run();
//     QString playcount = a[0];

    m_titleItem  = new StatisticsItem( i18n("Your collection statistics"), this );
    m_titleItem->setTitleItem( true );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_trackItem  = new StatisticsItem( i18n("1 Track","%n Tracks", a[0].toInt()), this, m_titleItem );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_artistItem = new StatisticsItem( i18n("1 Artist","%n Artists", a[0].toInt()), this, m_trackItem );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_albumItem  = new StatisticsItem( i18n("1 Album","%n Albums", a[0].toInt()), this, m_artistItem );

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    m_genreItem  = new StatisticsItem( i18n("1 Genre","%n Genres", a[0].toInt()), this, m_albumItem );

    m_titleItem ->setPixmap( QString("amarok") );
    m_trackItem ->setPixmap( QString("sound") );
    m_artistItem->setPixmap( QString("personal") );
    m_albumItem ->setPixmap( QString("cdrom_unmount") );
    m_genreItem ->setPixmap( QString("kfm") );
}

void
StatisticsList::itemClicked( QListViewItem *item ) //SLOT
{
    if( !item )
        return;

    if( item->depth() != 0 ) //not very flexible, *shrug*
    {
        #define item static_cast<StatisticsDetailedItem*>(item)
        if( item->itemType() == StatisticsDetailedItem::SHOW_MORE )
            expandInformation( item );

        else if( item->itemType() == StatisticsDetailedItem::SHOW_LESS )
            initDisplay();

        #undef item
        return;
    }

    #define item static_cast<StatisticsItem*>(item)

    if( item->isOn() )
    {
        while( item->firstChild() )
            delete item->firstChild();

        item->setOn( false );
        item->setOpen( true );
        return;
    }

    item->setOn( true );

    QueryBuilder qb;

    if( item == m_trackItem )
    {
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 10 );
        QStringList fave = qb.run();

        StatisticsDetailedItem *m_last = 0;
        uint c = 1;
        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3 (Score: %4)").arg( QString::number(c), fave[i], fave[i+1], fave[i+2] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            c++;
        }
        QString name = i18n("More tracks...");
        m_last = new StatisticsDetailedItem( name, item, m_last );
        m_last->setItemType( StatisticsDetailedItem::SHOW_MORE );
    }
    else if( item == m_artistItem )
    {
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
        qb.setLimit( 0, 10 );
        QStringList fave = qb.run();

        StatisticsDetailedItem *m_last = 0;
        uint c = 1;
        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 (Score: %3)").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            c++;
        }
        QString name = i18n("More artists...");
        m_last = new StatisticsDetailedItem( name, item, m_last );
        m_last->setItemType( StatisticsDetailedItem::SHOW_MORE );
    }
    else if( item == m_albumItem )
    {
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
        qb.setLimit( 0, 10 );
        QStringList fave = qb.run();

        StatisticsDetailedItem *m_last = 0;
        uint c = 1;
        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3 (Score: %4)").arg( QString::number(c), fave[i], fave[i+1], fave[i+2] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            c++;
        }
        QString name = i18n("More albums...");
        m_last = new StatisticsDetailedItem( name, item, m_last );
        m_last->setItemType( StatisticsDetailedItem::SHOW_MORE );
    }
    else if( item == m_genreItem )
    {
        qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName);
        qb.setLimit( 0, 10 );
        QStringList fave = qb.run();

        StatisticsDetailedItem *m_last = 0;
        uint c = 1;
        for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 (Score: %3)").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, item, m_last );
            c++;
        }
        QString name = i18n("More genres...");
        m_last = new StatisticsDetailedItem( name, item, m_last );
        m_last->setItemType( StatisticsDetailedItem::SHOW_MORE );
    }
    item->setOpen( true );

    #undef item
}

void
StatisticsList::expandInformation( StatisticsDetailedItem *item )
{
    StatisticsItem *parent = static_cast<StatisticsItem*>(item->parent());
    QueryBuilder qb;

    StatisticsDetailedItem *m_last = static_cast<StatisticsDetailedItem*>(item->itemAbove());
    uint a = 10; // the number of existing items
    uint c = a+1; // we be sneaky and just add to the other items

    if( parent == m_trackItem )
    {
        delete m_titleItem;
        delete m_artistItem;
        delete m_albumItem;
        delete m_genreItem;

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=a*qb.countReturnValues(); i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3 (Score: %4)").arg( QString::number(c), fave[i], fave[i+1], fave[i+2] );
            m_last = new StatisticsDetailedItem( name, parent, m_last );
            c++;
        }
    }

    else if( parent == m_albumItem )
    {
        delete m_titleItem;
        delete m_artistItem;
        delete m_genreItem;
        delete m_trackItem;

        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);

        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=a*qb.countReturnValues(); i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 - %3 (Score: %4)").arg( QString::number(c), fave[i], fave[i+1], fave[i+2] );
            m_last = new StatisticsDetailedItem( name, parent, m_last );
            c++;
        }
    }

    else if( parent == m_artistItem )
    {
        delete m_titleItem;
        delete m_trackItem;
        delete m_albumItem;
        delete m_genreItem;

        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=a*qb.countReturnValues(); i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 (Score: %3)").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, parent, m_last );
            c++;
        }
    }

    else if( parent == m_genreItem )
    {
        delete m_titleItem;
        delete m_trackItem;
        delete m_albumItem;
        delete m_artistItem;

        qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
        qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName);
        qb.setLimit( 0, 50 );
        QStringList fave = qb.run();

        for( uint i=a*qb.countReturnValues(); i < fave.count(); i += qb.countReturnValues() )
        {
            QString name = i18n("%1. %2 (Score: %3)").arg( QString::number(c), fave[i], fave[i+1] );
            m_last = new StatisticsDetailedItem( name, parent, m_last );
            c++;
        }
    }

    item->setText( 0, i18n("Back") );
    item->setItemType( StatisticsDetailedItem::SHOW_LESS );
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
    , m_isTitleItem( false )
    , m_on( false )
{
    setDragEnabled( false );
    setDropEnabled( false );
    setSelectable( false );

    setText( 0, text );
    setOn( false );

    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
}

void
StatisticsItem::setPixmap( const QString &pix )
{
    KIconLoader iconloader;
    QPixmap icon = iconloader.loadIcon( pix, KIcon::Toolbar, KIcon::SizeHuge );
    KListViewItem::setPixmap( 0, icon );
}

void
StatisticsItem::enterHover()
{
    if( m_isTitleItem )
        return;

    m_animEnter = true;
    m_animCount = 0;
    m_isActive = true;
    m_animTimer->start( ANIM_INTERVAL );
}

void
StatisticsItem::leaveHover()
{
    if( m_isTitleItem )
        return;

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
    if( isTitleItem() )
    {
        QFont font( p->font() );
        font.setBold( true );
        font.setPointSize( font.pointSize() + 1 );
        p->setFont( font );

        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    QColor fillColor, textColor;

    if ( isOn() )
    {
        fillColor = blendColors( cg.highlight(), cg.background(), static_cast<int>( m_animCount * 3.5 ) );
        textColor = blendColors( cg.highlightedText(), cg.text(), static_cast<int>( m_animCount * 4.5 ) );
    }
    else if( m_isActive ) //glowing animation
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
    QFontMetrics fm( p->fontMetrics() );

    int textHeight;
    int text_x = 0;

    textHeight = height();

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
    setSelectable( false );

    setText( 0, text );
}

void
StatisticsDetailedItem::setItemType( const ItemType t )
{
    m_type = t;

    if( t == SHOW_MORE )
    {
        KIconLoader iconloader;
        QPixmap icon = iconloader.loadIcon( "1rightarrow", KIcon::Small );
        setPixmap( 0, icon );
    }
    else if( t== SHOW_LESS )
    {
        KIconLoader iconloader;
        QPixmap icon = iconloader.loadIcon( "1leftarrow", KIcon::Small );
        setPixmap( 0, icon );
    }
    else
        setPixmap( 0, QPixmap::QPixmap() ); //clear it
}

void
StatisticsDetailedItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    KListViewItem::paintCell( p, cg, column, width, align );
}

#include "statistics.moc"

