/***************************************************************************
		statusbar.h  -  amaroK browserwin statusbar
			   -------------------
  begin                : Fre Apr 24 2002
  copyright            : (C) 2002 by Frederik Holljen
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

#include "statusbar.h"
#include "amarokconfig.h"
#include "metabundle.h"
#include "playerapp.h"
#include "threadweaver.h"

#include <qapplication.h>
#include <qcolor.h>
#include <qevent.h>

#include <kactionclasses.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kprogress.h>

#include <enginecontroller.h>

using namespace amaroK;

StatusBar* StatusBar::m_self;

StatusBar::StatusBar( QWidget *parent, const char *name ) : KStatusBar( parent, name )
{
    m_self = this;
    EngineController::instance()->attach( this );
    // message
    insertItem( QString::null, ID_STATUS, 10 );

    // progress
    m_progress = new KProgress( this );
    m_progress->setMaximumHeight( fontMetrics().height() );
    m_progress->hide();
    addWidget( m_progress, 0, true );

    // total songs count
    insertItem( QString::null, ID_TOTAL);

    // random
    ToggleLabel *rand = new ToggleLabel( i18n( "RAND" ), this );
    addWidget( rand, 0, true );
    KToggleAction *tAction = static_cast<KToggleAction *>(pApp->actionCollection()->action( "random_mode" ));
    connect( rand, SIGNAL( toggled( bool ) ), tAction, SLOT( setChecked( bool ) ) );
    connect( tAction, SIGNAL( toggled(bool) ), rand, SLOT( setOn(bool) ) );
    rand->setOn( tAction->isChecked() );

    // repeat playlist
    ToggleLabel *repeat = new ToggleLabel( i18n( "REP" ), this );
    addWidget( repeat, 0, true );
    tAction = static_cast<KToggleAction *>(pApp->actionCollection()->action( "repeat_playlist" ));
    connect( repeat, SIGNAL( toggled( bool ) ), tAction, SLOT( setChecked( bool ) ) );
    connect( tAction, SIGNAL( toggled(bool) ), repeat, SLOT( setOn(bool) ) );
    repeat->setOn( tAction->isChecked() );

    addWidget( (m_pTimeLabel = new ToggleLabel( "", this )), 0, true );
    m_pTimeLabel->setColorToggle( false );
    m_pTimeLabel->setFont( KGlobalSettings::fixedFont() );
    connect( m_pTimeLabel, SIGNAL( toggled( bool ) ), this, SLOT( slotToggleTime() ) );

    setItemAlignment( ID_STATUS, AlignLeft|AlignVCenter );
    setItemAlignment( ID_TOTAL, AlignCenter );
    // make the time label show itself.
    engineTrackPositionChanged( 0 );
}


StatusBar::~StatusBar()
{
    EngineController::instance()->detach( this );
}


void StatusBar::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
        case EngineBase::Idle:
        case EngineBase::Empty:
            engineTrackPositionChanged( 0 );
            changeItem( QString::null, ID_STATUS );
            break;
        case EngineBase::Playing: // gcc silence
        case EngineBase::Paused:
            break;
    }
}


void StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    changeItem( QString( "%1  (%2)" ).arg( bundle.prettyTitle(), bundle.prettyLength() ), ID_STATUS );
}

void StatusBar::slotItemCountChanged(int newCount)
{
    changeItem( newCount != 1 ? i18n( "%1 tracks" ).arg( newCount )
                              : i18n( "1 track" ), ID_TOTAL );
}

void StatusBar::engineTrackPositionChanged( long position )
{
    // TODO: Don't duplicate code
    int seconds = position / 1000;
    const uint songLength = EngineController::instance()->trackLength() / 1000;
    const bool remaining = AmarokConfig::timeDisplayRemaining() && songLength > 0;

    if( remaining ) seconds = songLength - seconds;

    QString str( ":" );
    str += zeroPad( seconds % 60 );
    str += ' ';
    str.prepend( zeroPad( seconds /= 60 ) );
    str.prepend( ':' );
    str.prepend( zeroPad( seconds / 60 ) );
    str.prepend( ' ' );

    m_pTimeLabel->setText( str );
}

void StatusBar::slotToggleTime()
{
    AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
}

void StatusBar::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) CollectionReader::ProgressEventType ) {
//         kdDebug() << k_funcinfo << "Received ProgressEvent\n";
        CollectionReader::ProgressEvent* p =
            static_cast<CollectionReader::ProgressEvent*>( e );

        switch ( p->state() ) {
            case CollectionReader::ProgressEvent::Start:
                m_progress->setProgress( 0 );
                m_progress->show();
                break;
            case CollectionReader::ProgressEvent::Stop:
                m_progress->hide();
                break;
            case CollectionReader::ProgressEvent::Total:
                m_progress->setTotalSteps( p->value() );
                break;
            case CollectionReader::ProgressEvent::Progress:
                m_progress->setProgress( p->value() );
        }
    }
}


/********** ToggleLabel ****************/

ToggleLabel::ToggleLabel( const QString &text, QWidget *parent, const char *name ) :
    QLabel( text, parent, name )
    , m_State( false )
    , m_ColorToggle( true )
{
}

ToggleLabel::~ToggleLabel()
{
}

void ToggleLabel::setColorToggle( bool on )
{
    m_ColorToggle = on;
    QColorGroup group = palette().active();
    setPaletteForegroundColor( group.text() );
}

void ToggleLabel::mouseDoubleClickEvent ( QMouseEvent */*e*/ )
{
    m_State = !m_State;
    if( m_ColorToggle )
    {
        QColorGroup group = palette().active();
        setPaletteForegroundColor( m_State ? group.text() : group.mid() );
    }
    emit toggled( m_State );
}

void ToggleLabel::setOn( bool on )
{
    if( m_ColorToggle )
    {
        QColorGroup group = palette().active();
        setPaletteForegroundColor( on ? group.text() : group.mid() );
    }

    m_State = on;
}

#include "statusbar.moc"
