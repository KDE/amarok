// Maintainer:  Max Howell (C) Copyright 2004
// Copyright:   See COPYING file that comes with this distribution
// Description: The SideBar/MultTabBar/BrowserBar all-in-one spectacular!
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


//USAGE
// 1. create a widget, NAME THE WIDGET!
// 2. use addBrowser() to append it to the bar
// 3. you can retrieve pointers to the widgets you inserted using browser( "name" )

//<mxcl>
//This is much tighter code than KDockWidget and co.
//I think I should look into submitting patches for that class. It's quite messy.
//But it is also more flexible in a docking perspective.

//TODO make browserholder a layout
//TODO perhaps you can add browsers and holder to multitabbar, and then just size that accordingly?
//NOTE the widget widths are saved in their baseSize() property


#define BROWSER_BAR_LEFT


namespace amaroK {

class Divider : public QWidget
{
public:
    Divider( QWidget *w ) : QWidget( w ) { styleChange( style() ); }

    virtual void paintEvent( QPaintEvent* )
    {
        QPainter p( this );
        parentWidget()->style().drawPrimitive( QStyle::PE_Splitter, &p, rect(), colorGroup(), 0 );
    }

    virtual void styleChange( QStyle& )
    {
        setFixedWidth( style().pixelMetric( QStyle::PM_SplitterWidth, this ) );
    }
};

class Drawer : public QFrame
{
public:
    Drawer( QWidget *parent ) : QFrame( parent, 0/*,  WType_TopLevel | WX11BypassWM | WStyle_NoBorder*/ )
    {
        setFrameShape( QFrame::Box );
    }

    void setExternal() { setLineWidth( 1 ); setWFlags( WType_TopLevel | WX11BypassWM | WStyle_NoBorder ); }
    void setInternal() { setLineWidth( 0 ); setWFlags( 0 ); }

    bool isExternal() const { return !testWFlags( 0 ); }
};

extern KConfig *config( const QString& );

}


BrowserBar::BrowserBar( QWidget *parent )
  : QWidget( parent, "BrowserBar" )
  , m_playlist( new QVBox( this ) )
  , m_divider( new amaroK::Divider( parent ) ) //FIXME why parent?
  , m_tabBar( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
  , m_browserHolder( new amaroK::Drawer( this ) )
  , m_currentIndex( -1 )
  , m_mapper( new QSignalMapper( this ) )
{
    m_pos = m_tabBar->sizeHint().width();

    m_tabBar->setStyle( KMultiTabBar::VSNET );
    m_tabBar->setPosition( KMultiTabBar::Left );
    m_tabBar->showActiveTabTexts( true );
    m_tabBar->setFixedWidth( m_pos );

#ifdef BROWSER_BAR_LEFT
    m_browserHolder->move( m_pos, 0 );
#else
    m_playlist->move( 0, 0 );
    //m_pos will be adjusted before we are shown (Resize event() or/and showHideBrowser())
#endif

    m_overlapButton = new TinyButton( m_browserHolder, const_cast< const char** >(not_close_xpm), i18n( "Overlap" ) );
    m_overlapButton->setToggleButton( true );
    connect( m_overlapButton, SIGNAL( toggled( bool ) ), SLOT( toggleOverlap( bool ) ) );

    QPushButton *closeButton = new TinyButton( m_browserHolder, style().stylePixmap( QStyle::SP_TitleBarCloseButton ), i18n( "Close" ) );
    connect( closeButton, SIGNAL( clicked() ), SLOT( closeCurrentBrowser() ) );

    const QSize buttonSize = closeButton->pixmap()->size();
    closeButton->setFixedSize( buttonSize );
    m_overlapButton->setFixedSize( buttonSize );

    QVBoxLayout *mainLayout = new QVBoxLayout( m_browserHolder, 0 );
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


    connect( m_mapper, SIGNAL( mapped( int ) ), SLOT( showHideBrowser( int ) ) );

    m_divider->installEventFilter( this );
    m_divider->setCursor( QCursor(SizeHorCursor) );

    //set the browserbar to an initial state of closed();
    m_browserHolder->hide();
    m_divider->hide();
    //ensure these widgets are at the front of the stack
    m_browserHolder->raise();
    m_divider->raise();


    //DRAWER CODE!!

    //m_browserHolder->setExternal();
    m_browserHolder->setInternal();

    //-------------
}

BrowserBar::~BrowserBar()
{
    KConfig* const config = amaroK::config( "PlaylistSideBar" );

    config->writeEntry( "Stay", m_overlapButton->isOn() );
    config->writeEntry( "CurrentPane", m_currentIndex != -1 ? currentBrowser()->name() : QString::null );

    for( BrowserIterator it = m_browsers.constBegin(), end = m_browsers.constEnd(); it != end; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}


void
BrowserBar::adjustWidgetSizes()
{
    //TODO set the geometry of the BrowserWin before it the browsers are loaded so this isn't called twice

    const uint w   = width();
    const uint h   = height();
    const uint mxW = uint(w*0.85);
    const uint p   = (m_pos < mxW) ? m_pos : mxW;
    const uint ppw = p + m_divider->width();
    const uint tbw = m_tabBar->width();

    //this bool indicates whether or not to draw the playlist offset
    //due to an open tab in overlap mode
    const bool b = m_overlapButton->isOn() && !m_divider->isHidden();

    m_divider->move( p, 0 );

#ifdef BROWSER_BAR_LEFT

    const uint offset = b ? ppw : tbw + 4; //the 4 is just for aesthetics

    m_browserHolder->resize( p - tbw, h );
    m_playlist->setGeometry( offset, 0, w - offset, h );

#else

    const uint reducedWidth = w - tbw;

    m_browserHolder->setGeometry( ppw, 0, reducedWidth - ppw, h );
    m_playlist->resize( b ? p : reducedWidth, h );

#endif
}

bool
BrowserBar::eventFilter( QObject*, QEvent *e )
{
    //if( !currentBrowser() ) return false; //we hide the divider now, so this should be impossible

    switch( e->type() )
    {
    case QEvent::MouseButtonRelease:

        currentBrowser()->setBaseSize( currentBrowser()->size() ); //only necessary to do on mouse release

        //FALL THROUGH

    case QEvent::MouseMove:
    {
        #define e static_cast<QMouseEvent*>(e)

        const uint currentPos = m_pos;
        const uint newPos     = mapFromGlobal( e->globalPos() ).x();

    #ifdef BROWSER_BAR_LEFT
        const uint minWidth   = m_tabBar->width() + currentBrowser()->minimumWidth();

        if( newPos < minWidth ) m_pos = minWidth;
        else /*if( newPos < maxWidth )*/ m_pos = newPos; //TODO allow for widget maximumWidth
    #else
        m_pos = newPos;
    #endif

        //TODO minimum playlist width must be greater than 10/9 of tabBar width or will be strange behaviour

        if( m_pos != currentPos ) adjustWidgetSizes();

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
      setMinimumWidth( m_tabBar->minimumWidth() + m_divider->minimumWidth() + m_playlist->minimumWidth() );
      break;

  case QEvent::Resize:

      m_divider->resize( 0, height() ); //Qt will set width

  #ifdef BROWSER_BAR_LEFT
      m_tabBar->resize( 0, height() ); //Qt will set width
  #else
  {
      const uint x = width()-m_tabBar->width();
      if( !m_browserHolder->isVisible() ) m_pos = x-4;
      else m_pos += width() - ((QResizeEvent*)e)->oldSize().width();
      m_tabBar->setGeometry( x, 0, 0, height() );
  }
  #endif
      adjustWidgetSizes();
      return true;

  default:
      break;
  }

  return QWidget::event( e );
}

void
BrowserBar::addBrowser( QWidget *widget, const QString &title, const QString& icon )
{
    //hi, this function is ugly - blame the monstrosity that is KMultiTabBar

    //determine next available id
    const int id = m_tabBar->tabs()->count();
    const QString name( widget->name() );

    widget->reparent( m_browserHolder, QPoint() ); //we need to own this widget for it to layout properly
    m_browserHolder->layout()->add( widget );
    widget->hide();
    if( widget->minimumWidth() < 30 ) widget->setMinimumWidth( 30 );

    //this is a signal the browser should emit when a single KURL is activated by the user
    connect( widget, SIGNAL(activated( const KURL& )), SIGNAL(activated( const KURL& )) );

    m_tabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, title );
    QWidget *tab = m_tabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME you can focus on the tab, but they respond to no input!

    //we use a SignalMapper to show/hide the corresponding browser when tabs are clicked
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );

    m_browsers.push_back( widget );

    KConfig* const config = amaroK::config( "PlaylistSideBar" );
    widget->setBaseSize( config->readNumEntry( name, widget->sizeHint().width() ), DEFAULT_HEIGHT );
    m_overlapButton->setOn( config->readBoolEntry( "Stay", true ) );
    if( config->readEntry( "CurrentPane" ) == name ) showHideBrowser( id );
}

void
BrowserBar::removeBrowser( const QCString &name )
{
    for( int x = 0; x < m_browsers.count(); ++x )
        if( m_browsers[x]->name() == name )
        {
            if( m_currentIndex == x ) closeCurrentBrowser();

            //NOTE we do not delete the browser currently
            //because we don't need this functionality yet
            //if you need to delete the browser you must implement:
            // 1. saving of size, see dtor
            // 2. either reorder tab ids or ensure iterations over m_browsers check for 0

            m_tabBar->removeTab( x );
            return;
        }
}

void
BrowserBar::showHideBrowser( int index )
{
    //this function is safe for any values of index ( even < 1 )

    //we use this tmpIndex due to a strange race condition caused by the following hide()
    //hide() seems to cause some events to be processed by BrowserWin::eventFilter()
    //the issue was autoClosePages() being called halfway during the call to hide()
    //that was crashing us as it was totally unexpected!(!)
    const int prevIndex = m_currentIndex;

    if( m_currentIndex != -1 )
    {
        //we need to hide the currentBrowser in all circumstances

        m_currentIndex = -1; //to prevent race condition crash described above

        m_browsers[prevIndex]->hide();
        m_tabBar->setTab( prevIndex, false );
    }

    if( index == prevIndex )
    {
        //then we are closing the BrowserBar
        //set to a closed state

        //m_currentIndex = -1; //done above now, will always happen

        m_browserHolder->hide();
        m_divider->hide();

        //we only need to adjust widget sizes if the overlap button is on
        //as otherwise playlist is right size already (see adjustWidgetSizes())
        if( m_overlapButton->isOn() ) adjustWidgetSizes();

    } else if( QWidget* const target = m_browsers[index] ) {

        //then open up target, adjust sized etc.

        m_currentIndex = index;

        //NOTE it is important that the divider is visible before adjust..() is called
        //NOTE the showEvents are processed after we have adjustedSizes below
        //FIXME show() is immediate, set geometries first!
        m_divider->show();
        target->show();
        target->setFocus();
        m_tabBar->setTab( index, true );
        m_browserHolder->show();

        if( /*m_browserHolder->isExternal()*/ false )
        {
            m_divider->hide(); //FIXME

            QRect r;
            QWidget *w = topLevelWidget();

            r.rTop()   = topLevelWidget()->geometry().y();
            r.rLeft()  = r.rRight() = topLevelWidget()->x();
            r.rLeft() -= target->baseSize().width();

            r.setHeight( m_tabBar->height() );

            m_browserHolder->stackUnder( w ); //doesn't work
            m_browserHolder->setGeometry( r );

        } else if( prevIndex == -1 || !m_overlapButton->isOn() ) {

            //we need to resize the browserHolder

            #ifdef BROWSER_BAR_LEFT
            m_pos = target->baseSize().width() + m_tabBar->width();
            #else
            m_pos = width() - target->baseSize().width() - m_tabBar->width();
            #endif

            adjustWidgetSizes();
        }
    }
}

QWidget*
BrowserBar::browser( const QCString &widgetName ) const
{
    for( BrowserIterator it = m_browsers.constBegin(), end = m_browsers.constEnd(); it != end; ++it )
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

void
BrowserBar::autoCloseBrowsers() //SLOT
{
    if( m_currentIndex != -1 && !m_overlapButton->isOn() )
    {
         closeCurrentBrowser();
    }
}

inline void
BrowserBar::toggleOverlap( bool on ) //SLOT
{
    //cursor for divider should be qsplitter style when overlapped and resize like when not
    m_divider->setCursor( QCursor( on ? SplitHCursor : SizeHorCursor ) );

    adjustWidgetSizes();
}



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
        topLeft     = bottomRight;
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
