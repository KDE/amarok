#include "statusbar.h"
#include "amarokconfig.h"
#include "metabundle.h"

#include <qcolor.h>

#include <enginecontroller.h>

amaroK::StatusBar::StatusBar( QWidget *parent, const char *name ) : KStatusBar( parent, name )
{
    EngineController::instance()->attach( this );
    addWidget( new ToggleLabel( "RANDOM", this ), 0, true );
    addWidget( (m_pTimeLabel = new ToggleLabel( "", this )), 0, true );

    connect( m_pTimeLabel, SIGNAL( toggled( int ) ), this, SLOT( slotToggleTime() ) );
}


amaroK::StatusBar::~StatusBar()
{
    EngineController::instance()->detach( this );
}


void amaroK::StatusBar::engineStateChanged( EngineBase::EngineState state )
{
}


void amaroK::StatusBar::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    message( bundle.prettyTitle() + "  (" + bundle.prettyLength() + ")" );
}

void amaroK::StatusBar::engineTrackPositionChanged( long position )
{
    // TODO: Don't duplicate code
    int seconds = position / 1000;
    int songLength = EngineController::instance()->trackLength() / 1000;
    bool remaining = AmarokConfig::timeDisplayRemaining() && songLength > 0;

    if( remaining ) seconds = songLength - seconds;

    QString
    str  = zeroPad( seconds /60/60%60 );
    str += ':';
    str += zeroPad( seconds /60%60 );
    str += ':';
    str += zeroPad( seconds %60 );
    m_pTimeLabel->setText( str );
}

void amaroK::StatusBar::slotToggleTime()
{
    AmarokConfig::setTimeDisplayRemaining( !AmarokConfig::timeDisplayRemaining() );
}


/********** ToggleLabel ****************/

amaroK::ToggleLabel::ToggleLabel( const QString &text, QWidget *parent, const char *name ) :
    QLabel( text, parent, name )
    , State( false )
{
}

void amaroK::ToggleLabel::mouseDoubleClickEvent ( QMouseEvent */*e*/ )
{
    State = !State;
    if( State )
        setPaletteForegroundColor( "black" );
    else
        setPaletteForegroundColor( "gray" );
    emit toggled( State );
}

void amaroK::ToggleLabel::setOn( bool on )
{
    if( on )
        setPaletteForegroundColor( "black" );
    else
        setPaletteForegroundColor( "gray" );

    if( State != on )
    {
        State = on;
        emit toggled( State );
    }
}

#include "statusbar.moc"
