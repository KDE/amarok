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

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "SliderWidget.h"
#include "TimeLabel.h"
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QMouseEvent>

ProgressWidget::ProgressWidget( QWidget *parent )
        : QWidget( parent )
{
    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 0 );
    box->setSpacing( 4 );

    m_slider = new Amarok::TimeSlider( this );
    m_slider->setToolTip( i18n( "Track Progress" ) );
    m_slider->setMaximumSize( 600000, 20 );

    m_timeLabelLeft = new TimeLabel( this );

    m_timeLabelRight = new TimeLabel( this );
    m_timeLabelRight->setAlignment( Qt::AlignRight );

    updateTimeLabelTooltips();

    m_timeLabelLeft->setShowTime( false );
    m_timeLabelLeft->setAlignment( Qt::AlignRight );
    m_timeLabelRight->setShowTime( false );
    m_timeLabelRight->setAlignment( Qt::AlignLeft );
    m_timeLabelLeft->show();
    m_timeLabelRight->show();

    box->addSpacing( 3 );
    box->addWidget( m_timeLabelLeft );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabelRight );

    EngineController *engine = The::engineController();

    if( engine->isPaused() )
        paused();
    else if( engine->isPlaying() )
        trackPlaying();
    else
        stopped();

    connect( engine, &EngineController::stopped,
             this, &ProgressWidget::stopped );
    connect( engine, &EngineController::paused,
             this, &ProgressWidget::paused );
    connect( engine, &EngineController::trackPlaying,
             this, &ProgressWidget::trackPlaying );
    connect( engine, &EngineController::trackLengthChanged,
             this, &ProgressWidget::trackLengthChanged );
    connect( engine, &EngineController::trackPositionChanged,
             this, &ProgressWidget::trackPositionChanged );

    connect( m_slider, &Amarok::TimeSlider::sliderReleased,
             engine, &EngineController::seekTo );

    connect( m_slider, &Amarok::TimeSlider::valueChanged,
             this, &ProgressWidget::drawTimeDisplay );

    setBackgroundRole( QPalette::BrightText );

    connect ( The::amarokUrlHandler(), &AmarokUrlHandler::timecodesUpdated,
              this, &ProgressWidget::redrawBookmarks );
    connect ( The::amarokUrlHandler(), &AmarokUrlHandler::timecodeAdded,
              this, &ProgressWidget::addBookmarkNoPopup );
}

void
ProgressWidget::addBookmarkNoPopup( const QString &name, int milliSeconds )
{
    addBookmark( name, milliSeconds, false );
}

void
ProgressWidget::addBookmark( const QString &name, int milliSeconds, bool showPopup )
{
    DEBUG_BLOCK
    if ( m_slider )
        m_slider->drawTriangle( name, milliSeconds, showPopup );
}

void
ProgressWidget::updateTimeLabelTooltips()
{
    TimeLabel *elapsedLabel = AmarokConfig::leftTimeDisplayRemaining() ? m_timeLabelRight : m_timeLabelLeft;
    TimeLabel *remainingLabel = AmarokConfig::leftTimeDisplayRemaining() ? m_timeLabelLeft : m_timeLabelRight;

    elapsedLabel->setToolTip( i18n( "The amount of time elapsed in current track" ) );
    remainingLabel->setToolTip( i18n( "The amount of time remaining in current track" ) );
}

void
ProgressWidget::drawTimeDisplay( int ms )  //SLOT
{
    if ( !isVisible() )
        return;

    const qint64 trackLength = The::engineController()->trackLength();

    //sometimes the engine gives negative position and track length values for streams
    //which causes the time sliders to show 'interesting' values like -322:0-35:0-59
    int seconds = qMax(0, ms / 1000);
    int remainingSeconds = qMax(0, int((trackLength - ms) / 1000));

    QString sSeconds = Meta::secToPrettyTime( seconds );
    QString sRemainingSeconds = '-' + Meta::secToPrettyTime( remainingSeconds );

    if( AmarokConfig::leftTimeDisplayRemaining() )
    {
        m_timeLabelLeft->setText( sRemainingSeconds );
        m_timeLabelLeft->setEnabled( remainingSeconds > 0 );

        m_timeLabelRight->setText( sSeconds );
        m_timeLabelRight->setEnabled( seconds > 0 );
    }
    else
    {
        m_timeLabelRight->setText( sRemainingSeconds );
        m_timeLabelRight->setEnabled( remainingSeconds > 0 );

        m_timeLabelLeft->setText( sSeconds );
        m_timeLabelLeft->setEnabled( seconds > 0 );
    }
}

void
ProgressWidget::stopped()
{
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

void
ProgressWidget::paused()
{
    // I am wondering, is there a way that the track can get paused
    // directly?
    m_timeLabelLeft->setEnabled( true );
    m_timeLabelRight->setEnabled( true );
}

void
ProgressWidget::trackPlaying()
{
    m_timeLabelLeft->setEnabled( true );
    m_timeLabelLeft->setEnabled( true );
    m_timeLabelLeft->setShowTime( true );
    m_timeLabelRight->setShowTime( true );

    //in some cases (for streams mostly), we do not get an event for track length changes once
    //loading is done, causing maximum() to return 0 at when playback starts. In this case we need
    //to make sure that maximum is set correctly or the slider will not move.
    trackLengthChanged( The::engineController()->trackLength() );
}

void
ProgressWidget::trackLengthChanged( qint64 milliseconds )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( milliseconds );

    const int timeLength = Meta::msToPrettyTime( milliseconds ).length() + 1; // account for - in remaining time
    QFontMetrics tFm( m_timeLabelRight->font() );
    const int labelSize = tFm.horizontalAdvance(QChar('0')) * timeLength;

    //set the sizes of the labels to the max needed by the length of the track
    //this way the progressbar will not change size during playback of a track
    m_timeLabelRight->setFixedWidth( labelSize );
    m_timeLabelLeft->setFixedWidth( labelSize );

    //get the urlid of the current track as the engine might stop and start several times
    //when skipping lst.fm tracks, so we need to know if we are still on the same track...
    if ( The::engineController()->currentTrack() )
        m_currentUrlId = The::engineController()->currentTrack()->uidUrl();

    redrawBookmarks();
}

void
ProgressWidget::trackPositionChanged( qint64 position )
{
    m_slider->setSliderValue( position );

    // update the enabled state. Phonon determines isSeekable sometimes too late.
    m_slider->setEnabled( (m_slider->maximum() > 0) && The::engineController()->isSeekable() );
    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}


void
ProgressWidget::redrawBookmarks( const QString *BookmarkName )
{
    DEBUG_BLOCK
    m_slider->clearTriangles();
    if ( The::engineController()->currentTrack() )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( track->has<Capabilities::TimecodeLoadCapability>() )
        {
            Capabilities::TimecodeLoadCapability *tcl = track->create<Capabilities::TimecodeLoadCapability>();
            BookmarkList list = tcl->loadTimecodes();
            debug() << "found " << list.count() << " timecodes on this track";
            for( AmarokUrlPtr url : list )
            {
                if ( url->command() == "play" )
                {

                    if ( url->args().keys().contains( "pos" ) )
                    {
                        int pos = url->args().value( "pos" ).toDouble() * 1000;
                        debug() << "showing timecode: " << url->name() << " at " << pos ;
                        addBookmark( url->name(), pos, ( BookmarkName && BookmarkName == url->name() ));
                    }
                }
            }
            delete tcl;
        }
    }
}

void ProgressWidget::mousePressEvent(QMouseEvent* e)
{
    QWidget* widgetUnderCursor = childAt(e->pos());
    if( widgetUnderCursor == m_timeLabelLeft ||
        widgetUnderCursor == m_timeLabelRight )
    {
        // user clicked on one of the time labels, switch display
        AmarokConfig::setLeftTimeDisplayRemaining( !AmarokConfig::leftTimeDisplayRemaining() );
        drawTimeDisplay( The::engineController()->trackPositionMs() );
        updateTimeLabelTooltips();
    }

    QWidget::mousePressEvent(e);
}

QSize ProgressWidget::sizeHint() const
{
    //int height = fontMetrics().boundingRect( "123456789:-" ).height();
    return QSize( width(), 12 );
}

