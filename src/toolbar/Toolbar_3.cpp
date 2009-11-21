
#include "Toolbar_3.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/ProgressWidget.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>

Toolbar_3::Toolbar_3( QWidget *parent ) : QToolBar( parent )
{
    EngineController *engine = The::engineController();
    setIconSize( QSize( 48, 48 ) );
    setContentsMargins( 4, 0, 4, 0 );
    PlayPauseButton *pp = new PlayPauseButton;
    pp->setPlaying(!engine->isPaused());
    pp->setFixedSize( 48, 48 );
    addWidget( pp );
    connect (pp, SIGNAL( toggled(bool) ), this, SLOT( setPlaying(bool) ) );

    QWidget *info = new QWidget(this);

    QHBoxLayout *hl = new QHBoxLayout;
    QVBoxLayout *vl = new QVBoxLayout( info );
    
    prev = new AnimatedLabelStack(QStringList(), info);
    prev->setAnimated( false );
    prev->setOpacity( 128 );
    prev->setCursor(Qt::PointingHandCursor);
    prev->installEventFilter( this );
//     prev->setAlign( Qt::AlignLeft );
    connect (prev, SIGNAL(clicked()), engine, SLOT(playPause()) );

    AnimatedLabelStack *current = new AnimatedLabelStack(QStringList() << "AmaroK 2.3", info);
    current->setBold( true );
    current->setCursor(Qt::PointingHandCursor);

    next = new AnimatedLabelStack(QStringList(), info);
    next->setAnimated( false );
    next->setOpacity( 160 );
    next->setCursor(Qt::PointingHandCursor);
    next->installEventFilter( this );
//     next->setAlign( Qt::AlignRight );
    connect (next, SIGNAL(clicked()), Amarok::actionCollection()->action( "next" ), SLOT(trigger()) );

    hl->addWidget( prev );
    hl->addWidget( current );
    hl->addWidget( next );
    vl->addLayout( hl );

    hl = new QHBoxLayout;
    ProgressWidget *progressWidget = new ProgressWidget( 0 );
    hl->addStretch( 3 );
    hl->addWidget( progressWidget );
    hl->setStretchFactor( progressWidget, 10 );
    hl->addStretch( 3 );
    vl->addLayout( hl );
    

    addWidget( info );

    
    connect ( prev, SIGNAL( pulsing(bool) ), current, SLOT( setStill(bool) ) );
    connect ( next, SIGNAL( pulsing(bool) ), current, SLOT( setStill(bool) ) );
    
    VolumeDial *vd = new VolumeDial( this );
    vd->setRange( 0, 100);
    vd->setValue( engine->volume() );
    vd->setFixedSize( 48, 48 );
    addWidget( vd );
    connect( vd, SIGNAL( valueChanged(int) ), engine, SLOT( setVolume(int) ) );
    connect( vd, SIGNAL( clicked() ), engine, SLOT( toggleMute(int) ) );
}

void
Toolbar_3::engineVolumeChanged( int percent )
{
}

void
Toolbar_3::engineMuteStateChanged( bool mute )
{
}

void
Toolbar_3::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    if ( currentState == oldState )
        return;
    if ( currentState == Phonon::PlayingState )
        m_playPause->setPlaying( true );
    else if ( currentState == Phonon::StoppedState || currentState == Phonon::PausedState )
        m_playPause->setPlaying( false );
}

void
Toolbar_3::engineTrackChanged( Meta::TrackPtr track );

void Toolbar_3::setPlaying( bool on )
{
    if ( on )
        The::EngineController()->play();
    else
        The::EngineController()->pause();
}


bool
Toolbar_3::eventFilter( QObject *o, QEvent *ev )
{
    if ( ev->type() == QEvent::Enter )
    {
        if (o == next)
            next->setOpacity( 255 );
        else if (o == prev)
            prev->setOpacity( 255 );
        return false;
    }
    if ( ev->type() == QEvent::Leave )
    {
        if (o == next)
            next->setOpacity( 160 );
        else if (o == prev)
            prev->setOpacity( 128 );
        return false;
    }
    return false;
}

