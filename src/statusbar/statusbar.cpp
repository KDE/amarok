/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "amarok.h"
#include "amarokcore/amarokconfig.h"
#include "debug.h"
#include "enginecontroller.h"
#include <klocale.h>
#include "metabundle.h"
#include <qhbox.h>
#include <qlayout.h>
#include <qtimer.h>
#include "sliderwidget.h"
#include "statusbar.h"
#include "timeLabel.h"


namespace amaroK {


//TODO disable hide statusbar? or show when required? that sucks though.


StatusBar* StatusBar::s_instance = 0;


StatusBar::StatusBar( QWidget *parent, const char *name )
        : KDE::StatusBar( parent, name )
        , EngineObserver( EngineController::instance() )
        , m_pauseTimer( new QTimer( this ) )
{
    s_instance = this; //static member

    // total songs count
    m_itemCountLabel = new QLabel( this );
    m_itemCountLabel->setAlignment( Qt::AlignCenter );
    m_itemCountLabel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    //positionBox
    QWidget *positionBox = new QWidget( this, "positionBox" );
    QBoxLayout *box = new QHBoxLayout( positionBox, 1, 3 );

    m_slider = new amaroK::Slider( Qt::Horizontal, positionBox );
    m_timeLabel = new TimeLabel( positionBox );
    m_slider->setMinimumWidth( m_timeLabel->width() );

    QHBox *hbox = new QHBox( this );
    QFont font; font.setPointSize( font.pointSize() - 1 );
    hbox->setFont( font );
    new QLabel( " RND", hbox );
    new QLabel( "REP ", hbox );

    //TODO reimplement insertChild() instead
    addWidget( m_itemCountLabel );
    addWidget( hbox );
    addWidget( positionBox );

    box->addSpacing( 3 );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabel );

    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

    // set up us the bomb
    engineStateChanged( Engine::Empty );
    //slotItemCountChanged( 0 );

    // for great justice!
    connect( m_pauseTimer, SIGNAL(timeout()), SLOT(slotPauseTimer()) );

    slotItemCountChanged( 0, 0, 0, 0 );

    //session stuff
    //setShown( AmarokConfig::showStatusBar() );
}

void
StatusBar::engineStateChanged( Engine::State state )
{
    m_pauseTimer->stop();

    switch ( state ) {
    case Engine::Empty:
        m_slider->setEnabled( false );
        m_slider->setMaxValue( 0 );
        m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
        setMainText( QString::null );
        break;

    case Engine::Paused:
        m_mainTextLabel->setText( i18n( "amaroK is paused" ) ); // display TEMPORARY message
        m_pauseTimer->start( 300 );
        break;

    case Engine::Playing:
        DEBUG_LINE_INFO
        resetMainText(); // if we were paused, this is necessary
        m_timeLabel->setEnabled( true );
        break;

    case Engine::Idle:
        ; //just do nothing, idle is temporary and a limbo state
    }
}


void
StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    QString title  = bundle.prettyTitle();
    QString length = bundle.prettyLength();

    if ( bundle.artist() == "Mike Oldfield" && bundle.title() == "Amarok" ) {
        longMessage( i18n(
                "<p>One of Mike Oldfield's best pieces of work, Amarok, inspired the name behind "
                "the audio-player you are currently using. Thanks for choosing amaroK!</p>"
                "<p align=right>Mark Kretschmann<br>Max Howell<br>Chris Muehlhaeuser<br>"
                "The many other people who have helped make amaroK what it is</p>" ) );
    }

    if ( title.isEmpty() )
        title = i18n( "Unknown track" );

    // don't show '-' or '?'
    if ( length.length() > 1 ) {
        title += " (";
        title += length;
        title += ")";
    }

    setMainText( i18n( "Playing: %1" ).arg( title ) );

    m_slider->setMaxValue( bundle.length() * 1000 );
    m_slider->setEnabled( bundle.length() > 0 );
}

void
StatusBar::slotItemCountChanged( int newCount, int newLength, int selCount, int selLength )
{
    QString text;

    if ( selCount > 1 ) {
        text = QString( i18n( "Selected %1 out of %2 Tracks" ) ).arg( selCount ).arg( newCount );
        if ( selCount != 0 )
            text += QString( " - [%1 / %2]" )
                    .arg( MetaBundle::prettyTime( selLength ) )
                    .arg( MetaBundle::prettyTime( newLength ) );
    } else {
        text = i18n( "1 Track", "%n Tracks", newCount );
        if ( newCount != 0 )
            text += QString( " - [%1]" ).arg( MetaBundle::prettyTime( newLength ) );
    }

    m_itemCountLabel->setText( ' ' + text + ' ' );
}

void
StatusBar::engineTrackPositionChanged( long position )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
StatusBar::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    const uint trackLength = EngineController::instance()->bundle().length();

    QString s( " " );

    if( AmarokConfig::timeDisplayRemaining() && trackLength > 0 ) {
        seconds = trackLength - seconds;
        s += '-';
    }

    s += MetaBundle::prettyTime( seconds );
    s += ' ';

    m_timeLabel->setText( s );
}

void
StatusBar::slotPauseTimer()  //slot
{
    static uint counter = 0;

    if ( counter == 0 )
        m_timeLabel->erase();
    else
        m_timeLabel->update();

    ++counter &= 3;
}

} //namespace amaroK

#include "statusbar.moc"
