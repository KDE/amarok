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
#include "contextbar.h"
#include "debug.h"
#include "multitabbar.h"   //m_tabBar

#include <kapplication.h>  //kapp
#include <kconfig.h>
#include <kiconloader.h>   //multiTabBar icons
#include <klocale.h>

#include <qcursor.h>       //for resize cursor
#include <qpainter.h>
#include <qsignalmapper.h> //m_mapper
#include <qstyle.h>        //amaroK::Splitter


// we emulate a qsplitter, mostly for historic reasons, but there are still a few advantages
// mostly we can stop the browserbar getting resized too small so that switching browser looks wrong


namespace amaroK
{
    class Splitter : public QWidget {
    public:
        Splitter( ContextBar *w ) : QWidget( w, "divider" )
        {
            setCursor( QCursor(SplitHCursor) );
            styleChange( style() );
        }

        virtual void paintEvent( QPaintEvent* )
        {
            QPainter p( this );
            parentWidget()->style().drawPrimitive( QStyle::PE_Splitter, &p, rect(), colorGroup(), QStyle::Style_Default );
        }

        virtual void styleChange( QStyle& )
        {
            setFixedHeight( style().pixelMetric( QStyle::PM_SplitterWidth, this ) );
        }

        virtual void mouseMoveEvent( QMouseEvent *e )
        {
            static_cast<ContextBar*>(parent())->mouseMovedOverSplitter( e );
        }
    };
}


ContextBar::ContextBar( QWidget *parent )
        : QWidget( parent, "ContextBar" )
        , m_playlistBox( new QVBox( this ) )
        , m_divider( new amaroK::Splitter( this ) )
        , m_tabBar( new MultiTabBar( MultiTabBar::Horizontal, this ) )
        , m_browserBox( new QWidget( this ) )
        , m_currentIndex( -1 )
        , m_lastIndex( -1 )
        , m_mapper( new QSignalMapper( this ) )
{
    m_pos = m_tabBar->sizeHint().height() + 5; //5 = aesthetic spacing

    m_tabBar->setStyle( MultiTabBar::AMAROK );
    m_tabBar->setPosition( MultiTabBar::Top );
    m_tabBar->showActiveTabTexts( true );
    m_tabBar->setFixedHeight( m_pos );
//    m_tabBar->move( 0, 3 );

    QVBoxLayout *layout = new QVBoxLayout( m_browserBox );
    layout->addSpacing( 3 ); // aesthetics
    layout->setAutoAdd( true );

    m_browserBox->move( 0, m_pos );
    m_browserBox->hide();
    m_divider->hide();
    m_playlistBox->setSpacing( 1 );

    connect( m_mapper, SIGNAL(mapped( int )), SLOT(showHideBrowser( int )) );
}

ContextBar::~ContextBar()
{
    KConfig* const config = amaroK::config( "ContextBar" );
    config->writeEntry( "CurrentPane", m_currentIndex != -1 ? QString(currentBrowser()->name()) : QString::null );
    config->writeEntry( "Height", m_browserBox->height() );
}

int
ContextBar::restoreHeight()
{
    const int index = indexForName( amaroK::config( "ContextBar" )->readEntry( "CurrentPane" ) );
//     const int height = amaroK::config( "ContextBar" )->readNumEntry( "Height", browser( index )->sizeHint().height() );
    const int height = amaroK::config( "ContextBar" )->readNumEntry( "Height", 200 );

    m_browserBox->resize( width(), 300 /*height*/ );
    return index;
}

void
ContextBar::polish()
{
    DEBUG_FUNC_INFO

    QWidget::polish();

    uint M = 0;
    foreachType( BrowserList, m_browsers ) {
        const uint m = (*it)->minimumHeight();
        if (m > M)
            M = m;
        if (m > 250) {
            warning() << "Browser is too large, mxcl says castrate the developer: " << (*it)->name() << ", " << M << endl;
            M = 250;
        }
    }

    m_browserBox->setMinimumHeight( M );
    const int index = restoreHeight();

    if (index != -1)
        // if we did it for -1 it ruins the browserBox size
        showHideBrowser( index );
}

void
ContextBar::adjustWidgetSizes()
{
    //TODO set the geometry of the PlaylistWindow before
    // the browsers are loaded so this isn't called twice

    const uint w    = width();
    const uint h    = height();
    const uint maxH = maxBrowserHeight();
    const uint p    = (m_pos < maxH) ? m_pos : maxH;
    const uint pph  = p + 5 /*m_divider->height()*/;
    const uint tbh  = m_tabBar->height();

    m_divider->move( 0, p );

    const uint offset = !m_divider->isHidden() ? pph : tbh;

    m_browserBox->resize( w, p - tbh );
    m_playlistBox->setGeometry( 0, offset, w, h - offset );
}

void
ContextBar::mouseMovedOverSplitter( QMouseEvent *e )
{
    const uint oldPos   = m_pos;
    const uint newPos   = mapFromGlobal( e->globalPos() ).y();
    const uint minHeight = m_tabBar->height() + m_browserBox->minimumHeight();
    const uint maxHeight = maxBrowserHeight();

    if( newPos < minHeight )
        m_pos = minHeight;

    else if( newPos > maxHeight )
        m_pos = maxHeight;

    else
        m_pos = newPos;

    if( m_pos != oldPos )
        adjustWidgetSizes();
}

bool
ContextBar::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::LayoutHint:
        //FIXME include browserholder width
        setMinimumHeight(
                m_tabBar->minimumHeight() +
                m_divider->minimumHeight() +
                m_browserBox->height() +
                m_playlistBox->minimumHeight() );
        break;

    case QEvent::Resize:
//         DEBUG_LINE_INFO

        m_divider->resize( width(), 0 ); //Qt will set width
        m_tabBar->resize( width(), 0 ); //Qt will set width

        adjustWidgetSizes();

        return true;

    default:
        ;
    }

    return QWidget::event( e );
}

void
ContextBar::addBrowser( QWidget *widget, const QString &title, const QString& icon )
{
    const int id = m_tabBar->tabs()->count(); // the next available id
    const QString name( widget->name() );
    QWidget *tab;

    widget->reparent( m_browserBox, QPoint() );
    widget->hide();

    m_tabBar->appendTab( SmallIcon( icon ), id, title );
    tab = m_tabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME you can focus on the tab, but they respond to no input!

    //we use a SignalMapper to show/hide the corresponding browser when tabs are clicked
    connect( tab, SIGNAL(clicked()), m_mapper, SLOT(map()) );
    m_mapper->setMapping( tab, id );
    connect( tab, SIGNAL(initiateDrag ( int ) ), this, SLOT( showBrowser( int )) );

    m_browsers.push_back( widget );
}

void
ContextBar::showHideBrowser( int index )
{
    const int prevIndex = m_currentIndex;

    if( m_currentIndex != -1 ) {
        ///first we need to hide the currentBrowser

        m_currentIndex = -1; //to prevent race condition, see CVS history

        m_browsers[prevIndex]->hide();
        m_tabBar->setTab( prevIndex, false );
    }

    if( index == prevIndex ) {
        ///close the ContextBar

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
            m_pos = m_browserBox->height() + m_tabBar->height();
            adjustWidgetSizes();
        }
    }

    emit browserActivated( index );
}

void
ContextBar::showHideVisibleBrowser( int index )
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
ContextBar::browser( const QString &name ) const
{
    foreachType( BrowserList, m_browsers )
        if( name == (*it)->name() )
            return *it;

    return 0;
}

int
ContextBar::visibleCount() const
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
ContextBar::indexForName( const QString &name ) const
{
    for( uint x = 0; x < m_browsers.count(); ++x )
        if( name == m_browsers[x]->name() )
            return x;

    return -1;
}

#include "contextbar.moc"
