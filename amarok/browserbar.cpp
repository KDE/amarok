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
#include <kmultitabbar.h>  //m_multiTabBar


//TODO panes don't remember their widths
//     you possibly don't need to use baseSize anymore..
//TODO rename class and files
//TODO update commentry
//TODO change container() name, it is poor
//TODO make pageholder a layout


//<mxcl>
//I avoided using KDockWidget as it is horrendously large and we only wanted a subset
//of its functionality. But this turned out to be more code than I thought.
//Maybe this was a silly decision.. but we have some functionality that KDockWidget doesn't offer
//like variable sized panes.
//I suggest using KDockWindow and KDockWidget if they have been cleaned up and made easier to understand
//with KDE 4

/**
 * The SideBbar/MultTab/Tray all-in-one spectacular!
 *
 * @author Max Howell.
*/

//NOTE I use the "current" property of the QPtrList, m_widgets, to represent
//the currently visible widget. This had a number of advantages, one of
//which is not clarity of source! Apologies.
//NOTE I use the baseSize().width() property to remember page widths as it
//an otherwise unused property and this saves memory

//NOTE you can obtain pointers to the widgets you insert with addPage() using: page( "name" );



BrowserBar::BrowserBar( QWidget *parent )
  : QWidget( parent )
  , m_playlist( new QVBox( this ) )
  , m_divider( new QFrame( parent ) )
  , m_multiTabBar( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
  , m_pageHolder( new QWidget( this ) ) //FIXME making this a layout would save mem
  , m_mapper( new QSignalMapper( this ) )
  , m_pages()
{
    m_pos = m_multiTabBar->sizeHint().width();

    m_multiTabBar->setStyle( KMultiTabBar::VSNET );
    m_multiTabBar->setPosition( KMultiTabBar::Left );
    m_multiTabBar->showActiveTabTexts( true );
    m_multiTabBar->setMinimumWidth( m_pos );

    m_stayButton = new TinyButton( m_pageHolder, const_cast< const char** >(not_close_xpm), i18n( "Overlap" ) );
    m_stayButton->setToggleButton( true );
    connect( m_stayButton, SIGNAL( clicked() ), SLOT( adjustSize() ) );

    QPushButton *closeButton = new TinyButton( m_pageHolder, style().stylePixmap( QStyle::SP_TitleBarCloseButton ), i18n( "Close" ) );
    connect( closeButton, SIGNAL( clicked() ), SLOT( close() ) );

    QSize buttonSize = closeButton->pixmap()->size();
    closeButton->setFixedSize( buttonSize );
    m_stayButton->setFixedSize( buttonSize );

    QVBoxLayout *mainLayout = new QVBoxLayout( m_pageHolder );
    QHBoxLayout *tinyLayout = new QHBoxLayout();
    tinyLayout->setAlignment( Qt::AlignRight );
    tinyLayout->addWidget( m_stayButton );
    tinyLayout->addWidget( closeButton );
    mainLayout->addLayout( tinyLayout );

    //set it to a default state of closed();
    m_pageHolder->hide();

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    m_stayButton->setOn( config->readBoolEntry( "Stay", true ) );

    connect( m_mapper, SIGNAL( mapped( int ) ), SLOT( showHidePage( int ) ) );

    m_divider->setFrameStyle( QFrame::Panel | QFrame::Raised );
    //m_divider->setLineWidth( 1 );
    //m_divider->raise();
    m_divider->setCursor( QCursor(sizeHorCursor) );
    m_divider->installEventFilter( this );
}

BrowserBar::~BrowserBar()
{
    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );

    config->writeEntry( "Stay", m_stayButton->isOn() );
    config->writeEntry( "CurrentPane", m_pages.current() ? m_pages.current()->name() : "" );

    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}


void
BrowserBar::adjustSize() //SLOT
{
    const uint offset = m_multiTabBar->width() + 4;

    if( !m_stayButton->isOn() )
    {
        m_pageHolder->raise();
        m_divider->raise();

        m_pageHolder->setGeometry( offset, 0, position() - offset, height() );
        m_playlist  ->setGeometry( offset, 0, width()-offset, height() );
        m_divider   ->setGeometry( position(), 0, 4, height() );

    } else {

        m_pageHolder->setGeometry( offset, 0, position() - offset, height() );
        m_playlist  ->setGeometry( position()+4, 0, width()-position()-4, height() );
        m_divider   ->setGeometry( position(), 0, 4, height() );
    }
}

bool
BrowserBar::eventFilter( QObject*, QEvent *e )
{
    #define e static_cast<QMouseEvent*>(e)
    const uint currentPos = m_pos;

    switch( e->type() )
    {
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    {
        if( !m_pages.current() ) break; //no pages open, don't allow resizing //TODO don't show resize cursor

        const uint newPos   = mapFromGlobal( e->globalPos() ).x();
        const uint minWidth = m_multiTabBar->width() + m_pages.current()->minimumWidth();

        if( newPos < minWidth ) m_pos = minWidth;
        else if( newPos < width() * 0.90 ) m_pos = newPos; //TODO allow for widget maximumWidth

        //TODO change widget maximumWidth property so it is appropriate for us, then we don't have to
        //     do maths here

        //TODO minimum playlist width must be greater than 10/9 of tabBar width or will be strange behaviour

        if( m_pos != currentPos ) adjustSize();
        //m_divider->repaint( false );

        return true;
    }
    default:
        break;
    }

    #undef e

    return false;
}

bool
BrowserBar::event( QEvent* e )
{
  switch( e->type() )
  {
  case QEvent::LayoutHint:
      kdDebug() << "LAYOUT_HINT!\n";
      setMinimumWidth( m_multiTabBar->minimumSize().width() + m_playlist->minimumSize().width() + 4 );
      break;

  case QEvent::Resize:
      m_multiTabBar->setGeometry( 0, 0, 0, height() ); //width will be adjusted by Qt
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
    int id = m_multiTabBar->tabs()->count();
    QString name( widget->name() );

    widget->reparent( m_pageHolder, QPoint(), false );
    m_pageHolder->layout()->add( widget );
    widget->hide();
    widget->setMinimumWidth( 20 );

    m_multiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, title );
    QWidget *tab = m_multiTabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME currently if you tab to these widgets, they respond to no input!

    //we use a SignalMapper to show/hide the corresponding page when tabs are clicked
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );

    KConfig *config = kapp->config();
    config->setGroup( "BrowserBar" );
    widget->setBaseSize( config->readNumEntry( name, widget->sizeHint().width() + m_multiTabBar->width() ), DefaultHeight );
    {
        //FIXME what if there's no correlation between QPtrList index and multiTabBar id?
        //we need some kind of sensible behavior that doesn't require much code
        int index = m_pages.at();
        m_pages.append( widget );
        //FIXME this sucks plenty!
        if( index == -1 ) { m_pages.last(); m_pages.next(); }
        else m_pages.at( index ); //restore "current"
    }

    if( config->readEntry( "CurrentPane" ) == name )
        showHidePage( id );
}

void
BrowserBar::showHidePage( int replaceIndex )
{
    int currentIndex = m_pages.at(); //doesn't set "current"
    QWidget *current = m_pages.current();
    QWidget *replace = m_pages.at( replaceIndex ); //sets "current" to replaceIndex

    if( replace != NULL ) //just in case
    {
        //FIXME make me pretty!

        if( current != NULL && current != replace )
        {
            //we need to close the open page
            current->hide();
            m_multiTabBar->tab( currentIndex )->setState( false );
        }

        bool isHidden = replace->isHidden();

        m_multiTabBar->tab( replaceIndex )->setState( isHidden );

        //TODO don't resize if m_stayButton->isOn();

        if( isHidden ) { replace->show(); m_pageHolder->show();
                         m_pos = replace->baseSize().width() + m_multiTabBar->width(); }
        else           { m_pages.last(); m_pages.next(); //sets "current" to NULL
                         replace->hide(); m_pageHolder->hide();
                         m_pos = m_multiTabBar->width(); }

        adjustSize();
    }
}

QWidget*
BrowserBar::page( const QString &widgetName )
{
    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
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
BrowserBar::close() //SLOT
{
    //this works even if "current" is NULL
    showHidePage( m_pages.at() );
}

inline void
BrowserBar::autoClosePages() //SLOT
{
    if( !m_stayButton->isOn() )
    {
        close();
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
