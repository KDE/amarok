#include "statusbar.h"
#include "amarokconfig.h"
#include "metabundle.h"
#include "playerapp.h"
#include "threadweaver.h"

#include <qapplication.h>
#include <qcolor.h>

#include <kactionclasses.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kprogress.h>

#include <enginecontroller.h>


amaroK::StatusBar* amaroK::StatusBar::m_self;

amaroK::StatusBar::StatusBar( QWidget *parent, const char *name ) : KStatusBar( parent, name )
{
    m_self = this;
    
    EngineController::instance()->attach( this );
    // message
    insertItem( QString::null, ID_STATUS, 10 );

    KProgress* progress = new KProgress( this );
    addWidget( progress, 0, true );
//     progress->hide();
    
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
    // make the time label show itself.
    engineTrackPositionChanged( 0 );
}


amaroK::StatusBar::~StatusBar()
{
    EngineController::instance()->detach( this );
}


void amaroK::StatusBar::engineStateChanged( EngineBase::EngineState state )
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


void amaroK::StatusBar::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    changeItem( QString( "%1  (%2)" ).arg( bundle.prettyTitle(), bundle.prettyLength() ), ID_STATUS );
}

void amaroK::StatusBar::engineTrackPositionChanged( long position )
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

void amaroK::StatusBar::slotToggleTime()
{
    AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
}

void amaroK::StatusBar::customEvent( QCustomEvent *e )
{
    kdDebug() << k_funcinfo << endl;

    if ( e->type() == (QEvent::Type) CollectionReader::ProgressEventType ) {
        kdDebug() << k_funcinfo << "Received ProgressEvent\n";
    }
}
    

/********** ToggleLabel ****************/

amaroK::ToggleLabel::ToggleLabel( const QString &text, QWidget *parent, const char *name ) :
    QLabel( text, parent, name )
    , m_State( false )
    , m_ColorToggle( true )
{
}

amaroK::ToggleLabel::~ToggleLabel()
{
}

void amaroK::ToggleLabel::setColorToggle( bool on )
{
    m_ColorToggle = on;
    QColorGroup group = palette().active();
    setPaletteForegroundColor( group.text() );
}

void amaroK::ToggleLabel::mouseDoubleClickEvent ( QMouseEvent */*e*/ )
{
    m_State = !m_State;
    if( m_ColorToggle )
    {
        QColorGroup group = palette().active();
        setPaletteForegroundColor( m_State ? group.text() : group.mid() );
    }
    emit toggled( m_State );
}

void amaroK::ToggleLabel::setOn( bool on )
{
    if( m_ColorToggle )
    {
        QColorGroup group = palette().active();
        setPaletteForegroundColor( on ? group.text() : group.mid() );
    }

    m_State = on;
}

#include "statusbar.moc"
