/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *   Copyright (C)       2005 Mark Kretschmann <markey@web.de>             *
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

#include <kapplication.h>  //kapp
#include <kconfig.h>
#include <kiconloader.h>   //multiTabBar icons
#include <klocale.h>

#include <qcursor.h>       //for resize cursor
#include <qlayout.h>
#include <qsplitter.h>
#include <qtoolbox.h>
#include <qvbox.h>


BrowserBar::BrowserBar( QWidget *parent )
        : QWidget( parent, "BrowserBar" )
        , EngineObserver( EngineController::instance() )
        , m_currentIndex( -1 )
        , m_lastIndex( -1 )
{
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->addSpacing( 3 ); // aesthetics
    layout->setAutoAdd( true );

    m_splitter = new QSplitter( QSplitter::Horizontal, this );
    m_splitter->setOpaqueResize( true );
    m_browserBox = new QVBox( m_splitter );
    m_playlistBox = new QVBox( m_splitter );

    m_toolBox = new QToolBox( m_browserBox );
    m_playlistBox->setSpacing( 1 );
}

BrowserBar::~BrowserBar()
{
    KConfig* const config = amaroK::config( "BrowserBar" );
    config->writeEntry( "CurrentPane", m_currentIndex != -1 ? currentBrowser()->name() : QString::null );
    config->writeEntry( "Width", m_browserBox->width() );
}

void
BrowserBar::polish()
{
    DEBUG_FUNC_INFO

    QWidget::polish();

    uint M = 0;
    foreachType( BrowserList, m_browsers ) {
        const uint m = (*it)->minimumWidth();
        if( m > M ) M = m;
    }

    KConfig* const config = amaroK::config( "BrowserBar" );
    const int index = indexForName( config->readEntry( "CurrentPane" ) );
    const int width = config->readNumEntry( "Width", browser( index )->sizeHint().width() );

    if( M > 250 ) {
        warning() << "Some browsers are insisting on a silly minimum size! " << M << endl;
        M = 250;
    }

    m_browserBox->setMinimumWidth( M );
    m_browserBox->resize( width, height() );

    if( index != -1 )
        // if we did it for -1 it ruins the browserBox size
        showHideBrowser( index );
}

void
BrowserBar::addBrowser( QWidget *widget, const QString &title, const QString& icon )
{
    const QString name( widget->name() );

    widget->reparent( m_browserBox, QPoint() );
    widget->hide();

    m_toolBox->addItem( widget, SmallIconSet( icon ), title );
    m_browsers.push_back( widget );
}

void
BrowserBar::showHideBrowser( int index )
{
    m_toolBox->setCurrentIndex( index );
    m_currentIndex = index;
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
BrowserBar::indexForName( const QString &name ) const
{
    for( uint x = 0; x < m_browsers.count(); ++x )
        if( name == m_browsers[x]->name() )
            return x;

    return -1;
}

void
BrowserBar::engineStateChanged( Engine::State state )
{
    if( !AmarokConfig::autoShowContextBrowser() || m_currentIndex == -1 )
        return;

    switch( state ) {
    case Engine::Playing:

        if( m_currentIndex != -1 ) {
            m_lastIndex = m_currentIndex;
            showBrowser( "ContextBrowser" );
        }

        // we watch for any event, if there's some event we reset the timer
//         currentBrowser()->installEventFilter( this );
//         currentBrowser()->setMouseTracking( true );
//         killTimers();
//         startTimer( 5000 );
        break;

    case Engine::Empty:

        if( m_lastIndex >= 0 )
            showBrowser( m_lastIndex );

    default:
        ;
    }
}


#include "browserbar.moc"
