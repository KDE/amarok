/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ProgressWidget.h"

#include "Debug.h"
#include "EngineController.h"
#include "SliderWidget.h"
#include "TimeLabel.h"
#include "amarokconfig.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/TimecodeLoadCapability.h"
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"

#include <QHBoxLayout>

#include <KLocale>


ProgressWidget::ProgressWidget( QWidget *parent )
    : QWidget( parent )
    , EngineObserver( The::engineController() )
    , m_timeLength( 0 )
{

    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new Amarok::TimeSlider( this );
    m_slider->setMouseTracking( true );
    m_slider->setToolTip( i18n( "Track Progress" ) );
    m_slider->setMaximumSize( 600000, 20 );

    m_timeLabelLeft = new TimeLabel( this );
    m_timeLabelLeft->setToolTip( i18n( "The amount of time elapsed in current song" ) );

    m_timeLabelRight = new TimeLabel( this );
    m_timeLabelRight->setFixedWidth( 60 );
    m_timeLabelRight->setToolTip( i18n( "The amount of time remaining in current song" ) );
    m_timeLabelRight->setAlignment( Qt::AlignRight );

    m_timeLabelLeft->setShowTime( false);
    m_timeLabelLeft->setAlignment( Qt::AlignRight );
    m_timeLabelRight->setShowTime( false );
    m_timeLabelLeft->show();
    m_timeLabelRight->show();

    box->addSpacing( 3 );
    box->addWidget( m_timeLabelLeft );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabelRight );

    engineStateChanged( Phonon::StoppedState );

    connect( m_slider, SIGNAL( sliderReleased( int ) ), The::engineController(), SLOT( seek( int ) ) );
    connect( m_slider, SIGNAL( valueChanged( int ) ), SLOT( drawTimeDisplay( int ) ) );

    setBackgroundRole( QPalette::BrightText );

    The::amarokUrlHandler()->registerForTimecodes( this );
}

ProgressWidget::~ProgressWidget()
{
    The::amarokUrlHandler()->unregisterForTimecodes( this );
}

void
ProgressWidget::addBookmark( const QString &name, int milliSeconds )
{
    DEBUG_BLOCK
    if( m_slider )
        m_slider->drawTriangle( name, milliSeconds );
}

void
ProgressWidget::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label

    const qint64 trackLength = The::engineController()->trackLength();

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = seconds;
        seconds = ( trackLength / 1000 ) - seconds;
    }

    // when the left label shows the remaining time and it's a stream
    else if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = seconds;
        seconds = 0; // for streams
    }

    // when the right label shows the remaining time and it's not a stream
    else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = ( trackLength / 1000 ) - seconds;
    }

    // when the right label shows the remaining time and it's a stream
    else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = 0;
    }

    //put Utility functions somewhere
    QString s1 = Meta::secToPrettyTime( seconds );
    QString s2 = Meta::secToPrettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
        s1.prepend( '-' );

    // when the right label shows the remaining time and it's not a stream
    else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
        s2.prepend( '-' );

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
    }
    else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabelLeft->setEnabled( true );
        m_timeLabelRight->setEnabled( false );
    }
    else
    {
        m_timeLabelLeft->setEnabled( true );
        m_timeLabelRight->setEnabled( true );
    }
}

void
ProgressWidget::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    //debug() << "POSITION: " << position;
    m_slider->setSliderValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
ProgressWidget::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    DEBUG_BLOCK

    switch ( state )
    {
        case Phonon::LoadingState:
            if ( !The::engineController()->currentTrack() || ( m_currentUrlId != The::engineController()->currentTrack()->uidUrl() ) )
            {
                m_slider->setEnabled( false );
                debug() << "slider disabled!";
                m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
                m_slider->setMaximum( 0 );
                m_timeLabelLeft->setEnabled( false );
                m_timeLabelLeft->setEnabled( false );
                m_timeLabelLeft->setShowTime( false );
                m_timeLabelRight->setShowTime( false );
            }
            break;

        case Phonon::PlayingState:
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelLeft->setShowTime( true );
            m_timeLabelRight->setShowTime( true );
            //fallthrough
            break;

        case Phonon::StoppedState:
        case Phonon::BufferingState:
        case Phonon::ErrorState:
            break;

        case Phonon::PausedState:
            m_timeLabelLeft->setEnabled( true );
            m_timeLabelRight->setEnabled( true );
            break;
    }
}

void
ProgressWidget::engineTrackLengthChanged( qint64 milliseconds )
{
    DEBUG_BLOCK

    debug() << "new length: " << milliseconds;
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( milliseconds );
    m_slider->setEnabled( milliseconds > 0 );
    debug() << "slider enabled!";
    m_timeLength = Meta::msToPrettyTime( milliseconds ).length()+1; // account for - in remaining time

    //get the urlid of the current track as the engine might stop and start several times
    //when skipping lst.fm tracks, so we need to know if we are still on the same track...
    if ( The::engineController()->currentTrack() )
        m_currentUrlId = The::engineController()->currentTrack()->uidUrl();

    redrawBookmarks();
}

void
ProgressWidget::redrawBookmarks()
{
    DEBUG_BLOCK
    m_slider->clearTriangles();

    if( The::engineController()->currentTrack() )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track->hasCapabilityInterface( Meta::Capability::LoadTimecode ) )
        {
            Meta::TimecodeLoadCapability *tcl = track->create<Meta::TimecodeLoadCapability>();
            BookmarkList list = tcl->loadTimecodes();
            debug() << "found " << list.count() << " timecodes on this track";
            foreach( AmarokUrlPtr url, list )
            {
                if( url->command() == "play"  ) {

                    if ( url->args().keys().contains( "pos" ) )
                    {
                        int pos = url->args().value( "pos" ).toInt() * 1000;
                        debug() << "showing timecode: " << url->name() << " at " << pos ;
                        addBookmark( url->name(), pos );
                    }
                }
            }
            delete tcl;
        }
    }
}

void
ProgressWidget::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    m_slider->setEnabled( false );
    engineTrackLengthChanged( The::engineController()->trackLength() );
}

QSize ProgressWidget::sizeHint() const
{
    //int height = fontMetrics().boundingRect( "123456789:-" ).height();
    return QSize( width(), 12 );
}

void ProgressWidget::enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )
    Q_UNUSED( reason )
    DEBUG_BLOCK

    m_slider->setEnabled( false );
    m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
    m_slider->setMaximum( 0 );
    m_timeLabelLeft->setEnabled( false );
    m_timeLabelLeft->setEnabled( false );
    m_timeLabelLeft->setShowTime( false );
    m_timeLabelRight->setShowTime( false );

    m_currentUrlId.clear();
    m_slider->clearTriangles();
}

#include "ProgressWidget.moc"
