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

#include "amarok.h"
#include "amarokconfig.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlistloader.h"
#include "sliderwidget.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qapplication.h> //startProgress() etc.
#include <qcolor.h>
#include <qcursor.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qtooltip.h> //toggle labels

#include <kactionclasses.h>
#include <kcursor.h> //setOverrideCursor()
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <ksqueezedtextlabel.h>

using namespace amaroK;


class TimeLabel : public QLabel
{
public:
    TimeLabel( QWidget *parent ) : QLabel( " 0:00:00 ", parent )
    {
        setFont( KGlobalSettings::fixedFont() );
        setFixedSize( sizeHint() );
    }

    virtual void mousePressEvent( QMouseEvent* e )
    {
        if ( e->button() == Qt::LeftButton )
            AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );

        if ( e->button() == Qt::RightButton ) {
            enum Items { NORMAL, REMAIN, LENGTH };

            KPopupMenu menu( this );
            menu.setCheckable( true );
            menu.insertTitle( i18n( "Time Display" ) );
            menu.insertItem( i18n( "Normal" ), NORMAL );
            menu.insertItem( i18n( "Remaining" ), REMAIN );
            menu.insertItem( i18n( "Length" ), LENGTH );
            menu.setItemChecked( NORMAL, !AmarokConfig::timeDisplayRemaining() );
            menu.setItemChecked( REMAIN, AmarokConfig::timeDisplayRemaining() );
            menu.setItemEnabled( LENGTH, false );
            int result = menu.exec( QCursor::pos() );

            switch ( result ) {
                case NORMAL:
                    AmarokConfig::setTimeDisplayRemaining( false );
                    break;
                case REMAIN:
                    AmarokConfig::setTimeDisplayRemaining( true );
                    break;
                case LENGTH:
                    break;
            }
        }
    }
};


StatusBar* StatusBar::s_instance = 0;

StatusBar::StatusBar( QWidget *parent, const char *name )
    : KStatusBar( parent, name )
    , m_sliderPressed( false )
    , m_pPauseTimer( new QTimer( this ) )
{
    //NOTE we don't use KStatusBar::insertItem() mainly because we have
    //no control over the heights of the labels

    s_instance = this; //static member

    // attach
    EngineController::instance()->attach( this );

    // title label
    addWidget( m_pTitle = new KSqueezedTextLabel( this ), 4 ); //TODO may look nicer without the gray border

    // progress
    addWidget( m_pProgressBox = new QHBox( this ), 0, true );
    QObject *pb = new QPushButton( SmallIcon( "cancel" ), QString::null, m_pProgressBox );
    m_pProgress = new KProgress( m_pProgressBox );
    connect( pb, SIGNAL(clicked()), SLOT(stopPlaylistLoader()) );
    m_pProgressBox->hide();

    // total songs count
    addWidget( m_pTotal = new QLabel( this ), 0, true );

    // toggle buttons
    const KActionCollection* const ac =amaroK::actionCollection();
    QWidget *w1 = new ToggleLabel( i18n( "RAND" ), this, (KToggleAction*)ac->action( "random_mode" ) );
    QWidget *w2 = new ToggleLabel( i18n( "REP" ),  this, (KToggleAction*)ac->action( "repeat_playlist" ) );
    QToolTip::add( w1, i18n("Double-click to toggle Random Mode") );
    QToolTip::add( w2, i18n("Double-click to toggle Repeat Playlist Mode") );

    // position slider (stretches at 1/4 the rate of the squeezedTextKLabel)
    addWidget( m_pSlider = new amaroK::Slider( Qt::Horizontal, this ), 1, true );
    connect( m_pSlider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_pSlider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

    // time display
    addWidget( m_pTimeLabel = new TimeLabel( this ), 0, true );

    // make all widgets as high as the time display
    const int h = m_pTimeLabel->height();
    m_pTitle->setFixedHeight( h );
    m_pProgressBox->setFixedHeight( h );
    m_pTotal->setFixedHeight( h );
    w1->setFixedHeight( h );
    w2->setFixedHeight( h );
    m_pSlider->setFixedHeight( h );
    m_pSlider->setMaximumWidth( 200 ); //hack to force statusbar to stay at reasonable width when showing tmp messages

    // set up us the bomb
    engineStateChanged( Engine::Empty );
    slotItemCountChanged( 0 );

    // for great justice!
    connect( m_pPauseTimer, SIGNAL(timeout()), SLOT(slotPauseTimer()) );
    connect( EngineController::instance(), SIGNAL(statusText( const QString& )), SLOT(engineMessage( const QString& )) );
}


StatusBar::~StatusBar()
{
    EngineController::instance()->detach( this );
}


void StatusBar::message( const QString& message ) //SLOT
{
    m_pTitle->setText( message );
    m_oldMessage = message;
}


void StatusBar::message( const QString& message, int ms ) //SLOT
{
    // TODO Show statusbar for messages in case it is hidden

    m_pTitle->setText( message );

    // Remove possible old timer
    disconnect( this, SLOT( restoreMessage() ) );

    QTimer::singleShot( ms, this, SLOT( restoreMessage() ) );
}


void StatusBar::restoreMessage() //SLOT
{
    m_pTitle->setText( m_oldMessage );
}


void StatusBar::clearMessage() //SLOT
{
    m_pTitle->clear();
    m_oldMessage = "";
}


void StatusBar::engineStateChanged( Engine::State state )
{
    switch( state )
    {
        case Engine::Empty:
            clearMessage();
            m_pSlider->setEnabled( false );
            m_pSlider->setMaxValue( 0 );
            m_pTimeLabel->clear(); //must be done after the setValue() above, due to a signal connection
            break;

        case Engine::Paused:
            m_pTitle->setText( i18n( "amaroK is paused" ) ); // display TEMPORARY message
            m_pPauseTimer->start( 300 );
            break;

        case Engine::Playing:
            restoreMessage();
            m_pPauseTimer->stop();
            break;

        case Engine::Idle:
            ; //just do nothing, idle is temporary and a limbo state
    }
}


void StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    message( QString( "%1  (%2)" ).arg( bundle.prettyTitle(), bundle.prettyLength() ) );
    m_pSlider->setMaxValue( bundle.length() * 1000 );
    m_pSlider->setEnabled( bundle.length() > 0 );
}

void StatusBar::slotItemCountChanged(int newCount)
{
    m_pTotal->setText( i18n( "1 Track", "%n Tracks", newCount ) );
}

void StatusBar::engineTrackPositionChanged( long position )
{
    m_pSlider->setValue( position );

    if( !m_pSlider->isEnabled() ) drawTimeDisplay( position );
}

void StatusBar::drawTimeDisplay( int ms ) //SLOT
{
    int seconds = ms / 1000;
    const uint trackLength = EngineController::instance()->bundle().length();
    if( AmarokConfig::timeDisplayRemaining() && trackLength > 0 ) seconds = trackLength - seconds;

    QString s;
    s  = ' ';
    s += MetaBundle::prettyTime( seconds );
    s += ' ';

    m_pTimeLabel->setText( s );
}

void StatusBar::stopPlaylistLoader() //SLOT
{
    PlaylistLoader::stop();
}

//these three are static functions
void StatusBar::startProgress() { QApplication::postEvent( s_instance, new ProgressEvent( -1 ) ); }
void StatusBar::showProgress( uint p ) { QApplication::postEvent( s_instance, new ProgressEvent( p ) ); }
void StatusBar::stopProgress() { QApplication::postEvent( s_instance, new ProgressEvent( 101 ) ); }

void StatusBar::customEvent( QCustomEvent *e )
{
    if( e->type() == ProgressEvent::Type )
    {
        #define e static_cast<ProgressEvent*>(e)
        switch( e->progress() )
        {
        case -1:
            m_pProgress->setProgress( 0 );
            m_pProgressBox->show();
            if( isHidden() ) show();
            QApplication::setOverrideCursor( KCursor::workingCursor() );
            break;

        case 101:
            m_pProgress->setProgress( 100 );
            QTimer::singleShot( 1000, m_pProgressBox, SLOT(hide()) );
            if( !AmarokConfig::showStatusBar() ) QTimer::singleShot( 2000, this, SLOT(hide()) );
            QApplication::restoreOverrideCursor();
            break;

        default:
            m_pProgress->setProgress( e->progress() );
        }
        #undef e
    }
}

inline void StatusBar::slotPauseTimer() //slot
{
    static uint counter = 0;

    if( counter == 0 )
    {
        m_pTimeLabel->erase();

    } else {

        m_pTimeLabel->update();
    }

    ++counter &= 3;
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
