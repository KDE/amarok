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
#include "statusbar.h"
#include "threadweaver.h"

#include <qapplication.h>
#include <qcolor.h>
#include <qevent.h>
#include <qslider.h>
#include <qtimer.h>
#include <qtooltip.h> //toggle labels

#include <kactionclasses.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kprogress.h>
#include <ksqueezedtextlabel.h>


using namespace amaroK;

StatusBar* StatusBar::m_self = 0;


class TimeLabel : public QLabel
{
public:
    TimeLabel( const QString &text, QWidget *parent ) : QLabel( text, parent )
    {
        setFont( KGlobalSettings::fixedFont() );
        setFixedSize( sizeHint() );
    }

    virtual void mouseDoubleClickEvent( QMouseEvent* )
    {
        AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
    }
};


//#include <qlayout.h>
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
    addWidget( m_pTitle = new KSqueezedTextLabel( this ), 2 ); //TODO may look nicer without the gray border

    // progress
    addWidget( m_pProgress = new KProgress( this ), 0, true );
    m_pProgress->setMaximumHeight( fontMetrics().height() );
    m_pProgress->hide();

    // total songs count
    addWidget( m_pTotal = new QLabel( this ), 0, true );
    m_pTotal->setFixedHeight( fontMetrics().height() );

    // toggle buttons
    const KActionCollection* const ac = pApp->actionCollection();
    QWidget *w1 = new ToggleLabel( i18n( "RAND" ), this, (KToggleAction*)ac->action( "random_mode" ) );
    QWidget *w2 = new ToggleLabel( i18n( "REP" ),  this, (KToggleAction*)ac->action( "repeat_playlist" ) );

    QToolTip::add( w1, i18n("Double-click to toggle Random Mode") );
    QToolTip::add( w2, i18n("Double-click to toggle Repeat Playlist Mode") );

    // position slider
    //TODO make this stretchy?
    addWidget( m_pSlider = new QSlider( Qt::Horizontal, this ), 0, true );
    m_pSlider->setTracking( false );
    m_pSlider->setFixedWidth( 70 );
    m_pSlider->setFixedHeight( fontMetrics().height() );
    connect( m_pSlider, SIGNAL( sliderPressed() ),     SLOT( sliderPressed() ) );
    connect( m_pSlider, SIGNAL( sliderReleased() ),    SLOT( sliderReleased() ) );
    connect( m_pSlider, SIGNAL( sliderMoved( int ) ),  SLOT( sliderMoved( int ) ) );

    // time display
    addWidget( m_pTimeLabel = new TimeLabel( " 00:00:00 ", this ), 0, true );

    connect( m_pPauseTimer, SIGNAL(timeout()), SLOT(slotPauseTimer()) );

    // set us up the bomb
    engineStateChanged( EngineBase::Empty );
    slotItemCountChanged( 0 );
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
            break;

        case EngineBase::Paused:
            // display TEMPORARY message
            message( "amaroK is paused" );
            m_pPauseTimer->start( 300 );
            break;

        case EngineBase::Playing:
            m_pPauseTimer->stop();
            // clear TEMPORARY message
            clear();
            enable = true;
            break;
    }

    m_pSlider->setEnabled( enable );
}


void StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_pTitle->setText( QString( "%1  (%2)" ).arg( bundle.prettyTitle(), bundle.prettyLength() ) );
    m_pSlider->setMaxValue( bundle.length() * 1000 );
}

void StatusBar::slotItemCountChanged(int newCount)
{
    m_pTotal->setText( i18n( "1 Track", "%n Tracks", newCount ) );
}

void StatusBar::engineTrackPositionChanged( long position )
{
    if ( !m_sliderPressed ) {
        drawTimeDisplay( position );
        // adjust position slider
        m_pSlider->setValue( position );
    }
}

void StatusBar::drawTimeDisplay( long position )
{
    const uint trackLength = EngineController::instance()->trackLength();
    const bool remaining = AmarokConfig::timeDisplayRemaining() && trackLength > 0;
    uint seconds = remaining ? (trackLength - position)/1000 : position/1000;

    // TODO: Don't duplicate code
    // TODO: instead make a static Metabundle prettyLength( int ) function
    QString str( " " );
    str.prepend( zeroPad( seconds % 60 ) );
    str.prepend( ':' );
    seconds /= 60;
    str.prepend( zeroPad( seconds % 60 ) );
    str.prepend( ':' );
    str.prepend( zeroPad( seconds / 60 ) );
    str.prepend( ' ' );

    m_pTimeLabel->setText( str );
}

void StatusBar::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) CollectionReader::ProgressEventType ) {
//         kdDebug() << k_funcinfo << "Received ProgressEvent\n";
        CollectionReader::ProgressEvent* p =
            static_cast<CollectionReader::ProgressEvent*>( e );

        switch ( p->state() ) {
            case CollectionReader::ProgressEvent::Start:
                m_pProgress->setProgress( 0 );
                m_pProgress->show();
                break;
            case CollectionReader::ProgressEvent::Stop:
                m_pProgress->hide();
                break;
            case CollectionReader::ProgressEvent::Total:
                m_pProgress->setTotalSteps( p->value() );
                break;
            case CollectionReader::ProgressEvent::Progress:
                m_pProgress->setProgress( p->value() );
        }
    }
}

inline void StatusBar::sliderPressed()
{
    m_sliderPressed = true;
}

inline void StatusBar::sliderReleased()
{
    m_sliderPressed = false;

    EngineController::engine()->seek( m_pSlider->value() );
}

inline void StatusBar::sliderMoved( int value )
{
    drawTimeDisplay( static_cast<long>( value ) );
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

ToggleLabel::ToggleLabel( const QString &text, KStatusBar* const bar, const KToggleAction* const action ) :
    QLabel( text, bar )
    , m_State( false )
{
    setFixedSize( sizeHint() );

    bar->addWidget( this, 0, true );
    connect( this,   SIGNAL(toggled( bool )), action, SLOT(setChecked( bool )) );
    connect( action, SIGNAL(toggled( bool )), this,   SLOT(setChecked( bool )) );

    setChecked( action->isChecked() );
}

void ToggleLabel::mouseDoubleClickEvent( QMouseEvent */*e*/ )
{
    setChecked( !m_State );

    emit toggled( m_State );
}

void ToggleLabel::setChecked( bool on )
{
    //FIXME setting palette is non-ideal as it means when the colors are changed in the control center
    //      these toggle buttons aren't updated. *shrug* it's not fatal.

    if( on )
        unsetPalette();
    else
        setPaletteForegroundColor( colorGroup().mid() );

    m_State = on;
}

#include "statusbar.moc"
