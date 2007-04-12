/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "debug.h"
#include "enginecontroller.h"
#include "progressslider.h"
#include "timeLabel.h"

#include <QHBoxLayout>
#include <QLabel>

TimeSlider *TimeSlider::s_instance = 0;
TimeSlider::TimeSlider( QWidget *parent )
    : QWidget( parent ),
      EngineObserver( EngineController::instance() )
{
    s_instance = this;

    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new Amarok::PrettySlider( Qt::Horizontal, Amarok::PrettySlider::Normal, this );

    // the two time labels. m_timeLabel is the left one,
    // m_timeLabel2 the right one.
    m_timeLabel = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time elapsed in current song" ) );

    m_timeLabel2 = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time remaining in current song" ) );

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

    engineStateChanged( Engine::Empty );

    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

}

void TimeSlider::drawTimeDisplay( int ms )  //SLOT
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

void TimeSlider::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
TimeSlider::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{

    switch ( state ) {
        case Engine::Empty:
            m_slider->setEnabled( false );
            m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
            m_slider->setMaximum( 0 );
            m_slider->newBundle( MetaBundle() ); // Set an empty bundle
            m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
            m_timeLabel2->setEnabled( false );
            break;

        case Engine::Playing:
            DEBUG_LINE_INFO
            m_timeLabel->setEnabled( true );
            m_timeLabel2->setEnabled( true );
            break;

        case Engine::Idle:
            ; //just do nothing, idle is temporary and a limbo state
    }
}
void TimeSlider::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_slider->newBundle( bundle );
    engineTrackLengthChanged( bundle.length() );
}

void TimeSlider::engineTrackLengthChanged( long length )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( length * 1000 );
    m_slider->setEnabled( length > 0 );
    m_timeLength = MetaBundle::prettyTime( length ).length()+1; // account for - in remaining time
}

#include "progressslider.moc"
