// Maintainer: Max Howell (C) Copyright 2004
// Copyright:  See COPYING file that comes with this distribution
//

#include "browserbar.h"

#include <qcursor.h>       //for resize cursor
#include <qobjectlist.h>   //coloredObjects()
#include <qpainter.h>      //BrowserBar::TinyButton
#include <qpixmap.h>       //TinyButtons
#include <qsignalmapper.h> //m_mapper
#include <qstyle.h>        //BrowserBar::BrowserBar
#include <qtooltip.h>      //QToolTip::add()

#include <kapplication.h>  //kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>   //multiTabBar icons
#include <klocale.h>
#include <kmultitabbar.h>  //m_tabBar


/**
 * The SideBbar/MultTab/Tray all-in-one spectacular!
 *
 * @author Max Howell.
*/

//Important: name the widgets you plan to add to the bar! The name is important and browser settings are
//associated with it. You also can get the widget pointer using page( "name" );


//TODO update commentry
//TODO change container() name, it is poor
//TODO make pageholder a layout
//TODO get the browsers to offer more sensible minimumWidths!
//TODO perhaps you can add pages and holder to multitabbar, and then just size that accordingly?


//<mxcl>
//I avoided using KDockWidget as it is horrendously large and we only wanted a subset
//of its functionality. But this turned out to be more code than I thought.
//Maybe this was a silly decision.. but we have some functionality that KDockWidget doesn't offer
//like variable sized panes.
//I suggest using KDockWindow and KDockWidget if they have been cleaned up and made easier to understand
//with KDE 4


//NOTE I use the baseSize().width() property to remember page widths as it
//an otherwise unused property and this saves memory
//NOTE you can obtain pointers to the widgets you insert with addPage() using: page( "name" );


#define BROWSER_BAR_LEFT


BrowserBar::BrowserBar( QWidget *parent )
  : QWidget( parent )
  , m_playlist( new QVBox( this ) )
  , m_divider( new QFrame( parent ) )
  , m_tabBar( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
  , m_pageHolder( new QWidget( this ) ) //FIXME making this a layout would save mem
  , m_mapper( new QSignalMapper( this ) )
  , m_pages()
  , m_currentPage( 0 )
  , m_currentTab( 0 )
{
    m_pos = m_tabBar->sizeHint().width();

    m_tabBar->setStyle( KMultiTabBar::VSNET );
    m_tabBar->setPosition( KMultiTabBar::Left );
    m_tabBar->showActiveTabTexts( true );
    m_tabBar->setFixedWidth( m_pos );

#ifdef BROWSER_BAR_LEFT
    m_pageHolder->move( m_pos, 0 );
#else
    m_playlist->move( 0, 0 );
    //m_pos will be adjusted before we are shown (Resize event() or/and showHidePage())
#endif

    m_overlapButton = new TinyButton( m_pageHolder, const_cast< const char** >(not_close_xpm), i18n( "Overlap" ) );
    m_overlapButton->setToggleButton( true );
    connect( m_overlapButton, SIGNAL( clicked() ), SLOT( adjustSize() ) );

    QPushButton *closeButton = new TinyButton( m_pageHolder, style().stylePixmap( QStyle::SP_TitleBarCloseButton ), i18n( "Close" ) );
    connect( closeButton, SIGNAL( clicked() ), SLOT( close() ) );

    QSize buttonSize = closeButton->pixmap()->size();
    closeButton->setFixedSize( buttonSize );
    m_overlapButton->setFixedSize( buttonSize );

    QVBoxLayout *mainLayout = new QVBoxLayout( m_pageHolder );
    QHBoxLayout *tinyLayout = new QHBoxLayout();
#ifdef BROWSER_BAR_LEFT
    tinyLayout->setAlignment( Qt::AlignRight );
    tinyLayout->addWidget( m_overlapButton );
    tinyLayout->addWidget( closeButton );
#else
    tinyLayout->setAlignment( Qt::AlignLeft );
    tinyLayout->addWidget( closeButton );
    tinyLayout->addWidget( m_overlapButton );
#endif
    mainLayout->addLayout( tinyLayout );

    //set it to a default state of closed();
    m_pageHolder->hide();

    connect( m_mapper, SIGNAL( mapped( int ) ), SLOT( showHidePage( int ) ) );

    m_divider->setFrameStyle( QFrame::Panel | QFrame::Raised );
    m_divider->setCursor( QCursor(sizeHorCursor) );
    m_divider->installEventFilter( this );
}

BrowserBar::~BrowserBar()
{
    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );

    config->writeEntry( "Stay", m_overlapButton->isOn() );
    config->writeEntry( "CurrentPane", m_currentPage ? m_currentPage->name() : QString::null );

    const PageIterator end = m_pages.constEnd();
    for( PageIterator it = m_pages.constBegin(); it != end; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}


void
BrowserBar::adjustSize() //SLOT
{
    const uint w   = width();
    const uint h   = height();
    const uint p   = position();
    const uint pp4 = p+4;

    //TODO in non-overlap mode, only resizes have to adjust the playlist size
    //TODO on right, ensure the divider is below the tabs in the stack and then take 2 pixs instead of 4
    //TODO shouldn't be necessary to raise() the pageHodler and divider every time, there is a widget stack

    m_divider->setGeometry( p, 0, 4, h );

#ifdef BROWSER_BAR_LEFT

    const uint offset = m_tabBar->width();
    const uint op4    = offset+4;

    //m_pageHolder->setGeometry( offset, 0, p-offset, h );
    m_pageHolder->resize( p-offset, h );

    if( m_overlapButton->isOn() )
    {
        m_playlist->setGeometry( pp4, 0, w-pp4, h );

    } else {

        m_pageHolder->raise();
        m_divider->raise();

        m_playlist->setGeometry( op4, 0, w-op4, h );
    }

#else

    const uint offset = w - m_tabBar->width();

    m_pageHolder->setGeometry( pp4, 0, offset-pp4, h );

    if( m_overlapButton->isOn() )
    {
        //m_playlist->setGeometry( 0, 0, p, h );
        m_playlist->resize( p, h );

    } else {

        m_pageHolder->raise();
        m_divider->raise();

        //m_playlist->setGeometry( 0, 0, offset-4, h );
        m_playlist->resize( offset-4, h );
    }

#endif
}

bool
BrowserBar::eventFilter( QObject*, QEvent *e )
{
    if( !m_currentPage ) return false;

    switch( e->type() )
    {
    case QEvent::MouseButtonRelease:

        m_currentPage->setBaseSize( m_currentPage->size() ); //only necessary to do on mouse release

        //FALL THROUGH

    case QEvent::MouseMove:
    {
        #define e static_cast<QMouseEvent*>(e)

        const uint currentPos = m_pos;
        const uint newPos     = mapFromGlobal( e->globalPos() ).x();

    #ifdef BROWSER_BAR_LEFT
        const uint minWidth   = m_tabBar->width() + m_currentPage->minimumWidth();
        const uint maxWidth   = uint(width() * 0.9);

        if( newPos < minWidth ) m_pos = minWidth;
        else if( newPos < maxWidth ) m_pos = newPos; //TODO allow for widget maximumWidth
    #else
        m_pos = newPos;
    #endif

        //TODO minimum playlist width must be greater than 10/9 of tabBar width or will be strange behaviour

        if( m_pos != currentPos ) adjustSize();

        return true;

        #undef e
    }

    default:
        break;
    }

    return false;
}

bool
BrowserBar::event( QEvent *e )
{
  switch( e->type() )
  {
  case QEvent::LayoutHint:
      //kdDebug() << "LAYOUT_HINT!\n";
      setMinimumWidth( m_tabBar->minimumWidth() + m_playlist->minimumWidth() + 4 );
      break;

  case QEvent::Resize:
  #ifdef BROWSER_BAR_LEFT
      //m_tabBar->setGeometry( 0, 0, 0, height() ); //width will be adjusted by Qt
      m_tabBar->resize( 0, height() ); //width will be adjusted by Qt
  #else
  {
      const uint x = width()-m_tabBar->width();
      if( !m_pageHolder->isVisible() ) m_pos = x-4;
      else m_pos += width() - ((QResizeEvent*)e)->oldSize().width();
      m_tabBar->setGeometry( x, 0, 0, height() );
  }
  #endif
      adjustSize();
      return true;

  default:
      break;
  }

  return QWidget::event( e );
}

void
BrowserBar::addPage( QWidget *widget, const QString &title, const QString& icon )
{
    //hi, this function is ugly - blame the monstrosity that is KMultiTabBar

    //determine next available id
    const int id = m_tabBar->tabs()->count();
    const QString name( widget->name() );

    widget->reparent( m_pageHolder, QPoint(), false ); //we need to own this widget for it to layout properly
    m_pageHolder->layout()->add( widget );
    widget->hide();
    if( widget->minimumWidth() < 20 ) widget->setMinimumWidth( 20 );

    m_tabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, title );
    QWidget *tab = m_tabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME currently if you tab to these widgets, they respond to no input!

    //we use a SignalMapper to show/hide the corresponding page when tabs are clicked
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );

    m_pages.push_back( widget ); //TODO check the index of this is the id of the tab, QValueVector?

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    widget->setBaseSize( config->readNumEntry( name, widget->sizeHint().width() ), DEFAULT_HEIGHT );
    m_overlapButton->setOn( config->readBoolEntry( "Stay", true ) );
    if( config->readEntry( "CurrentPane" ) == name ) showHidePage( id );
}

void
BrowserBar::showHidePage( int index )
{
    //determine the target page (the widget to show/hide)
    QWidget* const target = index != -1 ? m_pages[index] : m_currentPage;

    if( m_currentPage ) //then we need to hide the currentPage
    {
        m_currentPage->hide();
        m_currentTab->setState( false );
    }

    if( target == m_currentPage ) //then we need to set the bar to the closed state
    {
        m_currentPage = 0;
        m_currentTab  = 0;

        m_pageHolder->hide();

        #ifdef BROWSER_BAR_LEFT
        m_pos = m_tabBar->width();
        #else
        m_pos = width() - m_tabBar->width() - 4;
        #endif

    } else if( target ) { //then open up target

        const bool resizePage = !m_currentPage || !m_overlapButton->isOn();

        m_currentPage = target;
        m_currentTab  = m_tabBar->tab( index );

        //TODO if you always set baseSize to m_pos irrelevant of side you can avoid extra maths
        //     no that won't work if the window size is changed, but you may be able to remove the tabBar->width() factor..
        //TODO if you override width() and make it = width() - m_tabBar->width() would that make the code easier to read?

        //set the page Holder size if appropriate
        #ifdef BROWSER_BAR_LEFT
        if( resizePage ) m_pos = m_currentPage->baseSize().width() + m_tabBar->width();
        #else
        if( resizePage ) m_pos = width() - m_currentPage->baseSize().width() - m_tabBar->width();
        #endif

        m_currentPage->show();
        m_currentTab->setState( true );
        m_pageHolder->show();
    }

    kdDebug() << m_pos << ", " << width() << endl;

    adjustSize(); //m_pos has changed, we need to update geometries
}

QWidget*
BrowserBar::page( const QString &widgetName )
{
    const PageIterator end = m_pages.constEnd();

    for( PageIterator it = m_pages.constBegin(); it != end; ++it )
        if( widgetName == (*it)->name() )
            return *it;

    return 0;
}

void
BrowserBar::setFont( const QFont & )
{
    //Hi! are you here because this function doesn't work?
    //I can't decide, should we use the custom font or not?
}

inline void
BrowserBar::autoClosePages() //SLOT
{
    if( !m_overlapButton->isOn() && m_currentPage )
    {
        showHidePage();
    }
}


//FIXME move back to the top to make the commit more readable
BrowserBar::TinyButton::TinyButton( QWidget *parent, const QPixmap &pix, const QString &toolTipText )
  : QPushButton( parent )
  , m_mouseOver( false )
{
    setFocusPolicy( NoFocus );
    setPixmap( pix );
    QToolTip::add( this, toolTipText );
}

void
BrowserBar::TinyButton::drawButton( QPainter* p )
{
    //this is from kdockwidget_private.cpp and is now better code
    //TODO commit improvements

    p->fillRect( rect(), colorGroup().brush( QColorGroup::Button ) );
    p->drawPixmap( (width() - pixmap()->width()) / 2, (height() - pixmap()->height()) / 2, *pixmap() );

    //default colours are for mouseOver state
    QColor topLeft = colorGroup().light(), bottomRight = colorGroup().dark();

    if( isOn() || isDown() )
    {
        //draw buttonPressed state
        topLeft     = colorGroup().dark();
        bottomRight = colorGroup().light();
    }
    else if( !m_mouseOver ) return; //otherwise we are already drawn correctly

    const uint h = height() -1, w = width() -1;

    p->moveTo( 0, h );

    p->setPen( topLeft );
    p->lineTo( 0, 0 );
    p->lineTo( w, 0 );

    p->setPen( bottomRight );
    p->lineTo( w, h );
    p->lineTo( 0, h );
}

#include "browserbar.moc"
