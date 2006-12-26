/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "amarok.h"
#include "amarokcore/amarokconfig.h"
#include "debug.h"
#include "enginecontroller.h"
#include "queueLabel.h"
#include "metabundle.h"
#include "sliderwidget.h"
#include "statusbar.h"

#include <kiconloader.h>
#include <klocale.h>

#include <qhbox.h>
#include <qlayout.h>
#include <qtimer.h>

// stuff that must be included last
//#include "startupTips.h"
#include "timeLabel.h"
#include "toggleLabel.h"
#include "toggleLabel.moc"
#include "selectLabel.h"
#include "selectLabel.moc"


namespace Amarok {


KAction *action( const char *name ) { return Amarok::actionCollection()->action( name ); }

//TODO disable hide statusbar? or show when required? that sucks though.


StatusBar* StatusBar::s_instance = 0;


StatusBar::StatusBar( QWidget *parent, const char *name )
        : KDE::StatusBar( parent, name )
        , EngineObserver( EngineController::instance() )
        , m_timeLength( 9 )
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

    m_slider = new Amarok::PrettySlider( Qt::Horizontal, Amarok::PrettySlider::Normal,
					 positionBox );

    // the two time labels. m_timeLable is the left one,
    // m_timeLabel2 the right one.
    m_timeLabel = new TimeLabel( positionBox );
    m_slider->setMinimumWidth( m_timeLabel->width() );

    m_timeLabel2 = new TimeLabel( positionBox );
    m_slider->setMinimumWidth( m_timeLabel2->width() );

    // TODO Both labels need tooltips (string freeze?)
    QWidget *hbox = new QWidget( this );
    QBoxLayout *layout = new QHBoxLayout( hbox, 0, 2 );
    layout->addSpacing( 3 );
    layout->addWidget( m_queueLabel = new QueueLabel( hbox ) );
    layout->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::action( "repeat" ) ), hbox ) );
    layout->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::action( "random_mode" ) ), hbox ) );
    layout->addSpacing( 3 );

    //TODO reimplement insertChild() instead
    addWidget( m_itemCountLabel );
    addWidget( hbox );
    addWidget( positionBox );

    box->addSpacing( 3 );
    box->addWidget( m_timeLabel );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabel2 );
#ifdef Q_WS_MAC
    // don't overlap the resize handle with the time display
    box->addSpacing( 12 );
#endif

    if( !AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->hide();
 
    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

    // set us up the bomb
    engineStateChanged( Engine::Empty );
    //slotItemCountChanged( 0 );

    // for great justice!
    connect( m_pauseTimer, SIGNAL(timeout()), SLOT(slotPauseTimer()) );

    slotItemCountChanged( 0, 0, 0, 0, 0, 0 );

    //see statupTips.h
    //KDE::showNextTip( this );

    //session stuff
    //setShown( AmarokConfig::showStatusBar() );
}

void
StatusBar::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    m_pauseTimer->stop();

    switch ( state ) {
    case Engine::Empty:
        m_slider->setEnabled( false );
        m_slider->setMinValue( 0 ); //needed because setMaxValue() calls with bogus values can change minValue
        m_slider->setMaxValue( 0 );
	m_slider->newBundle( MetaBundle() ); // Set an empty bundle
        m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
        m_timeLabel2->setEnabled( false );
        setMainText( QString::null );
        break;

    case Engine::Paused:
        m_mainTextLabel->setText( i18n( "Amarok is paused" ) ); // display TEMPORARY message
        m_pauseTimer->start( 300 );
        break;

    case Engine::Playing:
        DEBUG_LINE_INFO
        resetMainText(); // if we were paused, this is necessary
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
        break;

    case Engine::Idle:
        ; //just do nothing, idle is temporary and a limbo state
    }
}

void
StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    #define escapeHTML(s) QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
    QString title       = escapeHTML( bundle.title() );
    QString prettyTitle = escapeHTML( bundle.prettyTitle() );
    QString artist      = escapeHTML( bundle.artist() );
    QString album       = escapeHTML( bundle.album() );
    QString length      = escapeHTML( bundle.prettyLength() );
    #undef escapeHTML

    if ( bundle.artist() == "Mike Oldfield" && bundle.title() == "Amarok" ) {
        longMessage( i18n(
                "<p>One of Mike Oldfield's best pieces of work, Amarok, inspired the name behind "
                "the audio-player you are currently using. Thanks for choosing Amarok!</p>"
                "<p align=right>Mark Kretschmann<br>Max Howell<br>Chris Muehlhaeuser<br>"
                "The many other people who have helped make Amarok what it is</p>" ), KDE::StatusBar::Information );
    }

    // ugly because of translation requirements
    if( !title.isEmpty() && !artist.isEmpty() && !album.isEmpty() )
        title = i18n( "track by artist on album", "<b>%1</b> by <b>%2</b> on <b>%3</b>" )
                .arg( title, artist, album );

    else if( !title.isEmpty() && !artist.isEmpty() )
        title = i18n( "track by artist", "<b>%1</b> by <b>%2</b>" )
                .arg( title, artist );

    else if( !album.isEmpty() )
        // we try for pretty title as it may come out better
        title = i18n( "track on album", "<b>%1</b> on <b>%2</b>" )
               .arg( prettyTitle, album );
    else
        title = "<b>" + prettyTitle + "</b>";

    if( title.isEmpty() )
        title = i18n( "Unknown track" );

    // don't show '-' or '?'
    if( length.length() > 1 ) {
        title += " (";
        title += length;
        title += ')';
    }

    setMainText( i18n( "Playing: %1" ).arg( title ) );

    m_slider->newBundle( bundle );
    engineTrackLengthChanged( bundle.length() );
}

void
StatusBar::slotItemCountChanged( int newCount, int newLength,  //total
                                 int visCount, int visLength,  //visible
                                 int selCount, int selLength ) //selected
{
    const bool hasSel = ( selCount > 1 ), hasVis = ( visCount != newCount );

    QString text = ( hasSel && hasVis ) ? i18n( "%1 selected of %2 visible tracks" )
                                          .arg( selCount ).arg( visCount )
                 : ( hasVis && newCount == 1 ) ? i18n( "0 visible of 1 track" )
                 : ( hasVis ) ? i18n( "%1 visible of %2 tracks" ).arg( visCount).arg( newCount )
                 : ( hasSel ) ? i18n( "%1 selected of %2 tracks" ).arg( selCount ).arg( newCount )
                 : i18n( "1 track", "%n tracks", newCount );

    int getValue = 0;

    if( hasSel )
        getValue = selLength;

    else if( hasVis )
        getValue = visLength;

    else
        getValue = newLength;

    if( getValue )
        m_itemCountLabel->setText( i18n( "X visible/selected tracks (time) ", "%1 (%2)" ).arg( text, MetaBundle::fuzzyTime( getValue ) ) );
    else
        m_itemCountLabel->setText( text );

    QToolTip::add( m_itemCountLabel,  i18n( "Play-time: %1" ).arg( MetaBundle::veryPrettyTime( getValue ) ) );
}

void
StatusBar::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
StatusBar::engineTrackLengthChanged( long length )
{
    m_slider->setMinValue( 0 );
    m_slider->setMaxValue( length * 1000 );
    m_slider->setEnabled( length > 0 );
    m_timeLength = MetaBundle::prettyTime( length ).length()+1; // account for - in remaining time
}

void
StatusBar::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label.
    const uint trackLength = EngineController::instance()->bundle().length();

    if( AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->show();
    else
        m_timeLabel->hide();

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = seconds;
        seconds = trackLength - seconds;
    // when the left label shows the remaining time and it's a stream
    } else if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = seconds;
        seconds = 0; // for streams
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = trackLength - seconds;
    // when the right label shows the remaining time and it's a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = 0;
    }

    QString s1 = MetaBundle::prettyTime( seconds );
    QString s2 = MetaBundle::prettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 ) {
        s1.prepend( '-' );
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        s2.prepend( '-' );
    }

    while( (int)s1.length() < m_timeLength )
        s1.prepend( ' ' );

    while( (int)s2.length() < m_timeLength )
        s2.prepend( ' ' );

    s1 += ' ';
    s2 += ' ';

    m_timeLabel->setText( s1 );
    m_timeLabel2->setText( s2 );

    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( false );
        m_timeLabel2->setEnabled( true );
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( false );
    } else
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
    }
}

void
StatusBar::slotPauseTimer()  //slot
{
    static uint counter = 0;

    if ( counter == 0 )
    {
        m_timeLabel->erase();
        m_timeLabel2->erase();
    } else
    {
        m_timeLabel->update();
        m_timeLabel2->update();
    }

    ++counter &= 3;
}

///////////////////
//MessageQueue
///////////////////

MessageQueue::MessageQueue()
    : m_queueMessages(true)
{}
MessageQueue*
MessageQueue::instance()
{
    static MessageQueue mq;
    return &mq;
}

void
MessageQueue::addMessage(const QString& message)
{
    if(m_queueMessages)
        m_messages.push(message);
    else
        StatusBar::instance()->longMessage(message);
}

void
MessageQueue::sendMessages()
{
     m_queueMessages = false;
     while(! m_messages.isEmpty())
     {
        StatusBar::instance()->longMessage(m_messages.pop());
     }
}

} //namespace Amarok

#include "statusbar.moc"
