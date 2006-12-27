/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "browserbar.h"
#include "debug.h"
#include "enginecontroller.h"
#include "multitabbar.h"   //m_tabBar

#include <kapplication.h>  //kapp
#include <kconfig.h>
#include <kiconloader.h>   //multiTabBar icons
#include <klocale.h>

#include <qcursor.h>       //for resize cursor
#include <qpainter.h>
#include <qsignalmapper.h> //m_mapper
#include <qstyle.h>        //Amarok::Splitter
#include <qtooltip.h>


// we emulate a qsplitter, mostly for historic reasons, but there are still a few advantages
// mostly we can stop the browserbar getting resized too small so that switching browser looks wrong


namespace Amarok
{
    class Splitter : public QWidget {
    public:
        Splitter( BrowserBar *w ) : QWidget( w, "divider" )
        {
            setCursor( QCursor(SplitHCursor) );
            styleChange( style() );
        }

        virtual void paintEvent( QPaintEvent* )
        {
            QPainter p( this );
            parentWidget()->style().drawPrimitive( QStyle::PE_Splitter, &p, rect(), colorGroup(), QStyle::Style_Horizontal );
        }

        virtual void styleChange( QStyle& )
        {
            setFixedWidth( style().pixelMetric( QStyle::PM_SplitterWidth, this ) );
        }

        virtual void mouseMoveEvent( QMouseEvent *e )
        {
            static_cast<BrowserBar*>(parent())->mouseMovedOverSplitter( e );
        }
    };
}

BrowserBar* BrowserBar::s_instance = 0;

BrowserBar::BrowserBar( QWidget *parent )
        : QWidget( parent, "BrowserBar" )
        , EngineObserver( EngineController::instance() )
        , m_playlistBox( new QVBox( this ) )
        , m_divider( new Amarok::Splitter( this ) )
        , m_browserBox( new QVBox( this ) )
        , m_currentIndex( -1 )
        , m_lastIndex( -1 )
        , m_mapper( new QSignalMapper( this ) )
{
    m_tabManagementButton = new QPushButton( SmallIconSet(Amarok::icon( "configure" )), 0, this, "tab_managment_button" );
    connect (m_tabManagementButton, SIGNAL(clicked()), SLOT(showBrowserSelectionMenu()));
    m_tabManagementButton->setIsMenuButton ( true ); //deprecated, but since I cannot add menu directly to button it is needed.

    QToolTip::add (m_tabManagementButton, i18n("Manage tabs")); 


    m_tabBar = new MultiTabBar( MultiTabBar::Vertical, this );


    m_tabManagementButton->setFixedWidth(m_tabBar->sizeHint().width());
    m_tabManagementButton->setFixedHeight(m_tabBar->sizeHint().width());


    s_instance = this;
    m_pos = m_tabBar->sizeHint().width() + 5; //5 = esthetic spacing

    m_tabBar->setStyle( MultiTabBar::AMAROK );
    m_tabBar->setPosition( MultiTabBar::Left );
    m_tabBar->showActiveTabTexts( true );
    m_tabBar->setFixedWidth( m_pos );
    m_tabBar->move( 0, 25 );

    QVBoxLayout *layout = new QVBoxLayout( m_browserBox );
    layout->addSpacing( 3 ); // aesthetics
    layout->setAutoAdd( true );

    m_browserBox->move( m_pos, 0 );
    m_browserBox->hide();
    m_divider->hide();
    m_playlistBox->setSpacing( 1 );

    connect( m_mapper, SIGNAL(mapped( int )), SLOT(showHideBrowser( int )) );

 


   
    //m_tabBar->appendButton( Amarok::icon( "configure" ), 1, 0, QString::null );

}

BrowserBar::~BrowserBar()
{
    KConfig* const config = Amarok::config( "BrowserBar" );
    config->writeEntry( "CurrentPane", m_currentIndex != -1 ? QString(currentBrowser()->name()) : QString::null );
    config->writeEntry( "Width", m_browserBox->width() );
}

void
BrowserBar::makeDropProxy( const QString &name, DropProxyTarget *finalTarget )
{
    int id = m_browserIds[name];
    MultiTabBarButton *button = m_tabBar->tab( id );
    if( button )
        button->proxyDrops( finalTarget );
}

int
BrowserBar::restoreWidth()
{
    const int index = indexForName( Amarok::config( "BrowserBar" )->readEntry( "CurrentPane" ) );
    const int width = Amarok::config( "BrowserBar" )->readNumEntry( "Width", browser( index )->sizeHint().width() );

    m_browserBox->resize( width, height() );
    m_pos = m_browserBox->width() + m_tabBar->width();

    return index;
}

void
BrowserBar::polish()
{
    DEBUG_FUNC_INFO

    QWidget::polish();

    uint M = 0;
    foreachType( BrowserList, m_browsers ) {
        const uint m = (*it)->minimumWidth();
        if (m > M)
            M = m;
        if (m > 250) {
            warning() << "Browser is too large, mxcl says castrate the developer: " << (*it)->name() << ", " << M << endl;
            M = 250;
        }
    }

    m_browserBox->setMinimumWidth( M );
    const int index = restoreWidth();

    if (index != -1)
        // if we did it for -1 it ruins the browserBox size
        showHideBrowser( index );
}

void
BrowserBar::adjustWidgetSizes()
{
    //TODO set the geometry of the PlaylistWindow before
    // the browsers are loaded so this isn't called twice

    const uint w    = width();
    const uint h    = height();
    const uint maxW = maxBrowserWidth();
    const uint p    = (m_pos < maxW) ? m_pos : maxW;
    const uint ppw  = p + m_divider->width();
    const uint tbw  = m_tabBar->width();

    m_divider->move( p, 0 );

    const uint offset = !m_divider->isHidden() ? ppw : tbw;

    m_browserBox->resize( p - tbw, h );
    m_playlistBox->setGeometry( offset, 0, w - offset, h );
}

void
BrowserBar::mouseMovedOverSplitter( QMouseEvent *e )
{
    const uint oldPos   = m_pos;
    const uint newPos   = mapFromGlobal( e->globalPos() ).x();
    const uint minWidth = m_tabBar->width() + m_browserBox->minimumWidth();
    const uint maxWidth = maxBrowserWidth();

    if( newPos < minWidth )
        m_pos = minWidth;

    else if( newPos > maxWidth )
        m_pos = maxWidth;

    else
        m_pos = newPos;

    if( m_pos != oldPos )
        adjustWidgetSizes();
}

bool
BrowserBar::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::LayoutHint:
        //FIXME include browserholder width
        setMinimumWidth(
                m_tabBar->minimumWidth() +
                m_divider->minimumWidth() +
                m_browserBox->width() +
                m_playlistBox->minimumWidth() );
        break;

    case QEvent::Resize:
//         DEBUG_LINE_INFO

        m_divider->resize( 0, height() ); //Qt will set width
        m_tabBar->resize( 0, height() ); //Qt will set width

        adjustWidgetSizes();

        return true;

    default:
        ;
    }

    return QWidget::event( e );
}

void
BrowserBar::addBrowser( const QString &identifier, QWidget *widget, const QString &title, const QString& icon )
{
    const int id = m_tabBar->tabs()->count(); // the next available id
    const QString name( widget->name() );
    m_browserIds[name] = id;
    QWidget *tab;

    widget->reparent( m_browserBox, QPoint() );
    widget->hide();

    m_tabBar->appendTab( SmallIcon( icon ), id, title, identifier );
    tab = m_tabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME you can focus on the tab, but they respond to no input!

    //we use a SignalMapper to show/hide the corresponding browser when tabs are clicked
    connect( tab, SIGNAL(clicked()), m_mapper, SLOT(map()) );
    m_mapper->setMapping( tab, id );
    connect( tab, SIGNAL(initiateDrag ( int ) ), this, SLOT( showBrowser( int )) );

    m_browsers.push_back( widget );
}

void
BrowserBar::removeMediaBrowser( QWidget *widget )
{
    BrowserList::iterator it = qFind( m_browsers.begin(), m_browsers.end(), widget );
    if( it != m_browsers.end() )
        m_browsers.erase( it );
    QWidget *tab;
    tab = m_tabBar->tab( m_browserIds["MediaBrowser"] );
    m_mapper->removeMappings( tab );
    m_tabBar->removeTab( m_browserIds["MediaBrowser"] );
}

void
BrowserBar::showHideBrowser( int index )
{
    const int prevIndex = m_currentIndex;

    if( m_currentIndex != -1 ) {
        ///first we need to hide the currentBrowser

        m_currentIndex = -1; //to prevent race condition, see CVS history

        m_browsers[prevIndex]->hide();
        m_tabBar->setTab( prevIndex, false );
    }

    if( index == prevIndex ) {
        ///close the BrowserBar

        m_browserBox->hide();
        m_divider->hide();

        adjustWidgetSizes();
    }

    else if( (uint)index < m_browsers.count() ) {
        ///open up target

        QWidget* const target = m_browsers[index];
        m_currentIndex = index;

        m_divider->show();
        target->show();
        target->setFocus();
        m_browserBox->show();
        m_tabBar->setTab( index, true );

        if( prevIndex == -1 ) {
            // we need to show the browserBox
            // m_pos dictates how everything will be sized in adjustWidgetSizes()
            m_pos = m_browserBox->width() + m_tabBar->width();
            adjustWidgetSizes();
        }
    }

    emit browserActivated( index );
}

void
BrowserBar::showHideVisibleBrowser( int index )
{
    int realindex = -1;
    QPtrList<MultiTabBarTab> tabs = *m_tabBar->tabs();
    for( int i = 0, n = tabs.count(); i < n; ++i )
    {
        if( tabs.at( i )->visible() )
            index--;
        if( index < 0 )
        {
            realindex = i;
            break;
        }
    }

    if( realindex >= 0 )
        showHideBrowser( realindex );
}

QWidget*
BrowserBar::browser( const QString &name ) const
{
    foreachType( BrowserList, m_browsers )
        if( name == (*it)->name() )
            return *it;

    return 0;
}

int
BrowserBar::visibleCount() const
{
    int num = 0;
    QPtrList<MultiTabBarTab> tabs = *m_tabBar->tabs();
    for( int i = 0, n = tabs.count(); i < n; ++i )
    {
        if( tabs.at( i )->visible() )
            num++;
    }

    return num;
}

int
BrowserBar::indexForName( const QString &name ) const
{
    for( uint x = 0; x < m_browsers.count(); ++x )
        if( name == m_browsers[x]->name() )
            return x;
    return -1;
}

void
BrowserBar::showBrowserSelectionMenu()
{
      m_tabBar->showTabSelectionMenu(mapToGlobal(QPoint(m_tabManagementButton->pos().x(), m_tabManagementButton->pos().y() +m_tabManagementButton->height() )));
}

void
BrowserBar::engineStateChanged( Engine::State state, Engine::State oldState )
{
    if( !AmarokConfig::autoShowContextBrowser() || m_currentIndex == -1 )
        return;

    switch( state ) {
    case Engine::Playing:

        if( oldState != Engine::Paused && m_currentIndex != -1 ) {
            m_lastIndex = m_currentIndex;
            showBrowser( "ContextBrowser" );
        }
        break;

    case Engine::Empty:

        if( m_lastIndex >= 0 )
            showBrowser( m_lastIndex );

    default:
        ;
    }
}

#include "browserbar.moc"
