/***************************************************************************
 statusbar.cpp        : amaroK browserwin statusbar
 copyright            : (C) 2004 by Frederik Holljen
 email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "app.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "sliderwidget.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qcolor.h>
#include <qevent.h>
#include <qtimer.h>
#include <qtooltip.h> //toggle labels

#include <kactionclasses.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kprogress.h>
#include <ksqueezedtextlabel.h>
#include <ksqueezedtextlabel.h>


using namespace amaroK;

StatusBar* StatusBar::m_self = 0;


class TimeLabel : public QLabel
{
public:
    TimeLabel( QWidget *parent ) : QLabel( " 0:00:00 ", parent )
    {
        setFont( KGlobalSettings::fixedFont() );
        setFixedSize( sizeHint() );
    }

    virtual void mouseDoubleClickEvent( QMouseEvent* )
    {
        AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
    }
};


StatusBar::StatusBar( QWidget *parent, const char *name )
    : KStatusBar( parent, name )
    , m_sliderPressed( false )
    , m_pPauseTimer( new QTimer( this ) )
{
    //NOTE we don't use KStatusBar::insertItem() mainly because we have
    //no control over the heights of the labels

    m_self = this; //static member

    // attach
    EngineController::instance()->attach( this );

    // title label
    addWidget( m_pTitle = new KSqueezedTextLabel( this ), 4 ); //TODO may look nicer without the gray border

    // progress
    addWidget( m_pProgress = new KProgress( this ), 0, true );
    m_pProgress->hide();

    // total songs count
    addWidget( m_pTotal = new QLabel( this ), 0, true );

    // toggle buttons
    const KActionCollection* const ac = pApp->actionCollection();
    QWidget *w1 = new ToggleLabel( i18n( "RAND" ), this, (KToggleAction*)ac->action( "random_mode" ) );
    QWidget *w2 = new ToggleLabel( i18n( "REP" ),  this, (KToggleAction*)ac->action( "repeat_playlist" ) );
    QToolTip::add( w1, i18n("Double-click to toggle Random Mode") );
    QToolTip::add( w2, i18n("Double-click to toggle Repeat Playlist Mode") );

    // position slider (stretches at 1/4 the rate of the squeezedTextKLabel)
    addWidget( m_pSlider = new PlaylistSlider( Qt::Horizontal, this ), 1, true );
    m_pSlider->setTracking( false );
    connect( m_pSlider, SIGNAL( sliderPressed() ),     SLOT( sliderPressed() ) );
    connect( m_pSlider, SIGNAL( sliderReleased() ),    SLOT( sliderReleased() ) );
    connect( m_pSlider, SIGNAL( sliderMoved( int ) ),  SLOT( sliderMoved( int ) ) );

    // time display
    addWidget( m_pTimeLabel = new TimeLabel( this ), 0, true );

    // make all widgets as high as the time display
    const int h = m_pTimeLabel->height();
    m_pTitle->setFixedHeight( h );
    m_pProgress->setFixedHeight( h );
    m_pTotal->setFixedHeight( h );
    w1->setFixedHeight( h );
    w2->setFixedHeight( h );
    m_pSlider->setFixedHeight( h );

    // set up us the bomb
    engineStateChanged( EngineBase::Empty );
    slotItemCountChanged( 0 );

    // for great justice!
    connect( m_pPauseTimer, SIGNAL(timeout()), SLOT(slotPauseTimer()) );
}


StatusBar::~StatusBar()
{
    EngineController::instance()->detach( this );
}


void StatusBar::engineStateChanged( EngineBase::EngineState state )
{
    bool enable = false; //for most states we want the slider disabled

    switch( state )
    {
        case EngineBase::Idle:
        case EngineBase::Empty:
            m_pTimeLabel->clear();
            m_pTitle->clear();
            m_pSlider->setMaxValue( 0 );
            break;

        case EngineBase::Paused:
            //message( i18n( "amaroK is paused" ) ); // display TEMPORARY message
            m_pPauseTimer->start( 300 );
            break;

        case EngineBase::Playing:
            m_pPauseTimer->stop();
            //clear(); // clear TEMPORARY message
            enable = true;
            break;
    }

    m_pSlider->setEnabled( enable );
}


void StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_pTitle->setText( QString( "%1  (%2)" ).arg( bundle.prettyTitle(), bundle.prettyLength() ) );
    m_pSlider->setMaxValue( bundle.length() );
}

void StatusBar::slotItemCountChanged(int newCount)
{
    m_pTotal->setText( i18n( "1 Track", "%n Tracks", newCount ) );
}

void StatusBar::engineTrackPositionChanged( long position )
{
    if ( !m_sliderPressed )
    {
        position /= 1000; //we deal in seconds
        drawTimeDisplay( position );
        m_pSlider->setValue( position );
    }
}

void StatusBar::drawTimeDisplay( long seconds )
{
    const uint trackLength = EngineController::instance()->bundle().length();
    if( AmarokConfig::timeDisplayRemaining() && trackLength > 0 ) seconds = trackLength - seconds;

    QString s;
    s  = ' ';
    s += MetaBundle::prettyTime( seconds );
    s += ' ';

    m_pTimeLabel->setText( s );
}

void StatusBar::customEvent( QCustomEvent *e )
{
    CollectionReader::ProgressEvent* p = (CollectionReader::ProgressEvent*)e;

    switch ( p->state() ) {
    case CollectionReader::ProgressEvent::Start:
        m_pProgress->setProgress( 0 );
        m_pProgress->show();
        if( isHidden() ) show();
        break;

    case CollectionReader::ProgressEvent::Stop:
        QTimer::singleShot( 2000, m_pProgress, SLOT(hide()) );
        if( !AmarokConfig::showStatusBar() ) QTimer::singleShot( 2000, this, SLOT(hide()) );
        break;

    case CollectionReader::ProgressEvent::Total:
        m_pProgress->setTotalSteps( p->value() );
        break;

    case CollectionReader::ProgressEvent::Progress:
        m_pProgress->setProgress( p->value() );
    }
}

inline void StatusBar::sliderPressed()
{
    m_sliderPressed = true;
}

inline void StatusBar::sliderReleased()
{
    m_sliderPressed = false;

    //we have a maximum resolution of 1 second, since we can only display that accuracy in the statusbar anyway
    EngineController::engine()->seek( m_pSlider->value() * 1000 );
}

inline void StatusBar::sliderMoved( int value )
{
    if ( m_sliderPressed )
        drawTimeDisplay( static_cast<long>( value ) );
    else
	//that's the case of scrolling with the mouse wheel
	EngineController::engine()->seek( value * 1000 );
}


inline void StatusBar::slotPauseTimer() //slot
{
    static bool quick = true;

    if( quick )
    {
        m_pPauseTimer->changeInterval( 300 );
        m_pTimeLabel->erase();

    } else {

        m_pPauseTimer->changeInterval( 1000 );
        m_pTimeLabel->update();
    }

    quick = !quick;
}

/********** ToggleLabel ****************/

ToggleLabel::ToggleLabel( const QString &text, KStatusBar* const bar, KToggleAction* const action )
    : QLabel( text, bar )
    , m_state( false )
    , m_action( action )
{
    setFixedSize( sizeHint() );

    bar->addWidget( this, 0, true );
    connect( action, SIGNAL(toggled( bool )), SLOT(setChecked( bool )) );

    setChecked( action->isChecked() );
}

void ToggleLabel::mouseDoubleClickEvent( QMouseEvent */*e*/ )
{
    m_action->activate();
}

void ToggleLabel::setChecked( bool on )
{
    //FIXME setting palette is non-ideal as it means when the colors are changed in the control center
    //      these toggle buttons aren't updated. *shrug* it's not fatal.

    if( on )
        unsetPalette();
    else
        setPaletteForegroundColor( colorGroup().mid() );

    m_state = on;
}

#include "statusbar.moc"
