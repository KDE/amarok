//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#include "playlistsidebar.h"

#include <qpainter.h>      //PlaylistSideBar::TinyButton
#include <qpixmap.h>       //TinyButtons
#include <qsignalmapper.h> //m_mapper
#include <qstyle.h>        //PlaylistSideBar::PlaylistSideBar
#include <qtimer.h>        //autoClose()
#include <qtooltip.h>      //QToolTip::add()

#include <kapplication.h>  //kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>   //multiTabBar icons
#include <klocale.h>
#include <kmultitabbar.h>  //m_multiTabBar


//<mxcl>
//I avoided using KDockWidget as it is horrendously large and we only wanted a subset
//of its functionality. But this turned out to be more code than I thought.
//Maybe this was a silly decision.. change it if you like.

/**
 * A mini-button usually placed in the dockpanel.
 *
 * @author Max Judin, Max Howell.
*/

PlaylistSideBar::TinyButton::TinyButton( QWidget *parent )
  : QPushButton( parent )
  , m_mouseOver( false )
{
    setFocusPolicy( NoFocus );
}

void PlaylistSideBar::TinyButton::drawButton( QPainter* p )
{
    p->fillRect( rect(), colorGroup().brush( QColorGroup::Button ) );
    p->drawPixmap( (width() - pixmap()->width()) / 2, (height() - pixmap()->height()) / 2, *pixmap() );

    //this is from kdockwidget_private.cpp and is now better code
    //TODO commit improvements

    if( isOn() || isDown() ) { //draw buttonPressed state
        p->setPen( colorGroup().dark() );
        p->moveTo( 0, height() - 1 );
        p->lineTo( 0, 0 );
        p->lineTo( width() - 1, 0 );

        p->setPen( colorGroup().light() );
        p->lineTo( width() - 1, height() - 1 );
        p->lineTo( 0, height() - 1 );
    }
    else if( m_mouseOver ) { //draw mouseOver state
        p->setPen( colorGroup().light() );
        p->moveTo( 0, height() - 1 );
        p->lineTo( 0, 0 );
        p->lineTo( width() - 1, 0 );

        p->setPen( colorGroup().dark() );
        p->lineTo( width() - 1, height() - 1 );
        p->lineTo( 0, height() - 1 );
    }
}

void PlaylistSideBar::TinyButton::enterEvent( QEvent * )
{
    m_mouseOver = true;
    repaint();
}

void PlaylistSideBar::TinyButton::leaveEvent( QEvent * )
{
    m_mouseOver = false;
    repaint();
}



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

PlaylistSideBar::PlaylistSideBar( QWidget *parent )
    : QHBox( parent )
    , m_multiTabBar( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
    , m_pageHolder( new QWidget( this ) ) //FIXME making this a layout would save mem
    , m_stayButton( 0 )
    , m_mapper( new QSignalMapper( this ) )
    , m_pages()
{
    m_multiTabBar->setStyle( KMultiTabBar::VSNET );
    m_multiTabBar->setPosition( KMultiTabBar::Left );
    m_multiTabBar->showActiveTabTexts( true );

    m_stayButton = new TinyButton( m_pageHolder );
    m_stayButton->setToggleButton( true );
    m_stayButton->setPixmap( const_cast< const char** >(not_close_xpm) );
    QToolTip::add( m_stayButton, i18n( "The tray stays open", "Stay" ) );

    QPushButton *closeButton = new TinyButton( m_pageHolder );
    closeButton->setPixmap( style().stylePixmap( QStyle::SP_TitleBarCloseButton , this) );
    QToolTip::add( closeButton, i18n( "Close" ) );
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
    setMaximumWidth( m_multiTabBar->sizeHint().width() );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    m_stayButton->setOn( config->readBoolEntry( "Stay", true ) );

    connect( m_mapper, SIGNAL( mapped( int ) ), SLOT( showHidePage( int ) ) );
}

PlaylistSideBar::~PlaylistSideBar()
{
    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    config->writeEntry( "Stay", m_stayButton->isOn() );
    config->writeEntry( "CurrentPane", m_pages.current() ? m_pages.current()->name() : "" );

    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}

void PlaylistSideBar::addPage( QWidget *widget, const QString &title, const QString& icon )
{
    //hi, this function is ugly - blame the monstrosity that is KMultiTabBar

    //determine next available id
    int id = m_multiTabBar->tabs()->count();
    QString name( widget->name() );

    widget->reparent( m_pageHolder, QPoint(), false );
    m_pageHolder->layout()->add( widget );
    widget->hide();

    m_multiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, title );
    QWidget *tab = m_multiTabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME currently if you tab to these widgets, they respond to no input!

    //we use a SignalMapper to show/hide the corresponding page when tabs are clicked
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
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

void PlaylistSideBar::showHidePage( int replaceIndex )
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
        if( isHidden ) { replace->show(); m_pageHolder->show(); setMaximumWidth( 2000 ); }
        else           { m_pages.last(); m_pages.next(); //quickly sets "current" to NULL
                         replace->hide(); m_pageHolder->hide(); setMaximumWidth( m_multiTabBar->width() ); }
    }
}

QSize PlaylistSideBar::sizeHint() const
{
    //return a sizeHint that will make the splitter space our pages as the user expects
    //note you can't just return width() that wouldn't work unfortunately
    //NOTE the widget's baseSize is the whole sidebar width, not just the widget's width
    return ( m_pages.current() ) ? m_pages.current()->baseSize() : m_multiTabBar->size();
}

void PlaylistSideBar::resizeEvent( QResizeEvent *e )
{
    if( m_pages.current() )
    {
        m_pages.current()->setBaseSize( e->size().width(), DefaultHeight );
    }
}

QWidget *PlaylistSideBar::page( const QString &widgetName )
{
    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        if( widgetName == (*it)->name() )
            return *it;

    return 0;
}

void PlaylistSideBar::setFont( const QFont & )
{
    //Hi! are you here because this function doesn't work?
    //Please consider not changing the fonts for the browsers as they require a
    //readable font, and the user already set a readable font - his default font
    //thanks! - mxcl

    //note - untested because font system was broken when I made this
    //FIXME you'll need to iterate over the children or something else..
/*    for( uint i = 0; i < m_pages.size(); ++i )
        if( m_pages[i] )
            m_pages[i]->setFont( font );*/
}

inline void PlaylistSideBar::close() //SLOT
{
    //this works even if "current" is NULL
    showHidePage( m_pages.at() );
}

inline void PlaylistSideBar::autoClosePages() //SLOT
{
    if( !m_stayButton->isOn() )
    {
        QTimer::singleShot( QApplication::doubleClickInterval(), this, SLOT( close() ) );
    }
}


#include "playlistsidebar.moc"
