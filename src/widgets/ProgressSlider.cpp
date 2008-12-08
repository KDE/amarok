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

#include "ProgressSlider.h"
#include "SliderWidget.h"

#include "amarokconfig.h"
#include "Debug.h"
#include "EngineController.h"
#include "meta/MetaUtility.h"
#include "TimeLabel.h"

#include <QHBoxLayout>

#include <KLocale>

//Class ProgressWidget
ProgressWidget *ProgressWidget::s_instance = 0;
ProgressWidget::ProgressWidget( QWidget *parent )
    : QWidget( parent )
    , EngineObserver( The::engineController() )
    , m_timeLength( 0 )
{
    s_instance = this;

    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new Amarok::TimeSlider( /*Qt::Horizontal,*/ this );
    m_slider->setMouseTracking( true );
    m_slider->setToolTip( i18n( "Track Progress" ) );
    m_slider->setMaximumSize( 600000, 20 );

    m_timeLabelLeft = new TimeLabel( this );
    m_timeLabelLeft->setToolTip( i18n( "The amount of time elapsed in current song" ) );

    m_timeLabelRight = new TimeLabel( this );
    m_timeLabelRight->setToolTip( i18n( "The amount of time remaining in current song" ) );

    m_timeLabelLeft->setShowTime( false);
    m_timeLabelLeft->setAlignment(Qt::AlignRight);
    m_timeLabelRight->setShowTime( false );
    m_timeLabelLeft->show();
    m_timeLabelRight->show();

    box->addSpacing( 3 );
    box->addWidget( m_timeLabelLeft );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabelRight );
#ifdef Q_WS_MAC
    // don't overlap the resize handle with the time display
    box->addSpacing( 12 );
#endif

    engineStateChanged( Phonon::StoppedState );

    connect( m_slider, SIGNAL(sliderReleased( int )), The::engineController(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

    setBackgroundRole( QPalette::BrightText );
}

void
ProgressWidget::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label
    const uint trackLength = The::engineController()->trackLength();

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

    //put Utility functions somewhere
    QString s1 = Meta::secToPrettyTime( seconds );
    QString s2 = Meta::secToPrettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 ) {
        s1.prepend( '-' );
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        s2.prepend( '-' );
    }

    if( m_timeLength > s1.length() )
        s1.prepend( QString( m_timeLength - s1.length(), ' ' ) );

    if( m_timeLength > s2.length() )
        s2.prepend( QString( m_timeLength - s2.length(), ' ' ) );

    s1 += ' ';
    s2 += ' ';

    m_timeLabelLeft->setText( s1 );
    m_timeLabelRight->setText( s2 );

    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabelLeft->setEnabled( false );
        m_timeLabelRight->setEnabled( true );
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabelLeft->setEnabled( true );
        m_timeLabelRight->setEnabled( false );
    } else
    {
        m_timeLabelLeft->setEnabled( true );
        m_timeLabelRight->setEnabled( true );
    }
}

void
ProgressWidget::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
//     debug() << "POSITION: " << position;
    m_slider->setSliderValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
ProgressWidget::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    switch ( state ) {
        case Phonon::StoppedState:
        case Phonon::LoadingState:
            m_slider->setEnabled( false );
            m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
            m_slider->setMaximum( 0 );
            m_timeLabelLeft->setEnabled( false );
            m_timeLabelLeft->setEnabled( false );
            m_timeLabelLeft->setShowTime( false );
            m_timeLabelRight->setShowTime( false );
            break;

        case Phonon::PlayingState:
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelLeft->setShowTime( true );
            m_timeLabelRight->setShowTime( true );
            //fallthrough
            break;

        case Phonon::BufferingState:
            break;

        case Phonon::PausedState:
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelRight->setEnabled( true );
            break;

        case Phonon::ErrorState:
            ;
    }
}

void
ProgressWidget::engineTrackLengthChanged( long seconds )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( seconds * 1000 );
    m_slider->setEnabled( seconds > 0 );
    m_timeLength = Meta::secToPrettyTime( seconds ).length()+1; // account for - in remaining time
}

void
ProgressWidget::engineNewTrackPlaying()
{
    engineTrackLengthChanged( The::engineController()->trackLength() );
}

QSize ProgressWidget::sizeHint() const
{
    //int height = fontMetrics().boundingRect( "123456789:-" ).height();
    return QSize( width(), 12 );
}

#include "ProgressSlider.moc"

