
#include "Toolbar_3.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"

#include "playlist/PlaylistActions.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/ProgressWidget.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>

Toolbar_3::Toolbar_3( QWidget *parent )
    : QToolBar( i18n( "Toolbar 3G" ), parent )
    , EngineObserver( The::engineController() )
{
    setObjectName( "Toolbar_3G" );
    
    EngineController *engine = The::engineController();
    setIconSize( QSize( 48, 48 ) );
    setContentsMargins( 4, 0, 4, 0 );

    m_playPause = new PlayPauseButton;
    m_playPause->setPlaying( !engine->isPaused() );
    m_playPause->setFixedSize( 48, 48 );
    addWidget( m_playPause );
    connect ( m_playPause, SIGNAL( toggled(bool) ), this, SLOT( setPlaying(bool) ) );

    QWidget *info = new QWidget(this);
    QHBoxLayout *hl = new QHBoxLayout;
    QVBoxLayout *vl = new QVBoxLayout( info );
    
    m_prev = new AnimatedLabelStack(QStringList(), info);
    m_prev->setAnimated( false );
    m_prev->setOpacity( 128 );
    m_prev->installEventFilter( this );
//     m_prev->setAlign( Qt::AlignLeft );
    connect ( m_prev, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( back() ) );

    m_current = new AnimatedLabelStack(QStringList() << "AmaroK 2.3", info);
    m_current->setBold( true );
//     connect ( m_prev, SIGNAL( clicked(const QString&), The::playlistActions(), SLOT( back() ) );

    m_next = new AnimatedLabelStack(QStringList(), info);
    m_next->setAnimated( false );
    m_next->setOpacity( 160 );
    m_next->installEventFilter( this );
//     m_next->setAlign( Qt::AlignRight );
    connect ( m_next, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( next() ) );

    hl->addWidget( m_prev );
    hl->addWidget( m_current );
    hl->addWidget( m_next );
    vl->addLayout( hl );

    connect ( m_prev, SIGNAL( pulsing(bool) ), m_current, SLOT( setStill(bool) ) );
    connect ( m_next, SIGNAL( pulsing(bool) ), m_current, SLOT( setStill(bool) ) );

    hl = new QHBoxLayout;
    ProgressWidget *progressWidget = new ProgressWidget( 0 );
    hl->addStretch( 3 );
    hl->addWidget( progressWidget );
    hl->setStretchFactor( progressWidget, 10 );
    hl->addStretch( 3 );
    vl->addLayout( hl );
    

    addWidget( info );

    m_volume = new VolumeDial( this );
    m_volume->setRange( 0, 100);
    m_volume->setValue( engine->volume() );
    m_volume->setMute( engine->isMuted() );
    m_volume->setFixedSize( 48, 48 );
    addWidget( m_volume );
    connect( m_volume, SIGNAL( valueChanged(int) ), engine, SLOT( setVolume(int) ) );
    connect( m_volume, SIGNAL( muteToggled(bool) ), engine, SLOT( setMuted(bool) ) );
}

void
Toolbar_3::engineVolumeChanged( int percent )
{
    m_volume->setValue( percent );
}

void
Toolbar_3::engineMuteStateChanged( bool mute )
{
    m_volume->setMute( mute );
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

static QStringList data( Meta::TrackPtr track )
{
    QStringList list;
    if ( track )
    {
        if ( !track->name().isEmpty() )
            list << track->prettyName();
        if ( !track->composer()->name().isEmpty() )
            list << track->composer()->prettyName();
        if ( !track->album()->name().isEmpty() )
            list << track->album()->prettyName();
        if ( !track->artist()->name().isEmpty() )
            list << track->artist()->prettyName();
        if ( !track->year()->name().isEmpty() )
            list << track->year()->prettyName();
        if ( !track->genre()->name().isEmpty() )
            list << track->genre()->prettyName();
        #if 0
        virtual double score() const = 0;
        virtual int rating() const = 0;
        /** Returns the length of this track in milliseconds, or 0 if unknown */
        virtual qint64 length() const = 0;
        virtual int sampleRate() const = 0;
        virtual int bitrate() const = 0;
        virtual int trackNumber() const = 0;
        virtual int discNumber() const = 0;
        virtual uint lastPlayed() const = 0;
        virtual uint firstPlayed() const;
        virtual int playCount() const = 0;
        virtual QString type() const = 0;
        virtual bool inCollection() const;
        #endif
    }
    return list;
}

void
Toolbar_3::engineTrackChanged( Meta::TrackPtr track )
{
    m_current->setData( data( track ) );
    m_current->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    
    track = The::PlaylistActions()->prevTrack();
    m_prev->setData( data( track ) );
    m_prev->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    
    track = The::PlaylistActions()->nextTrack();
    m_next->setData( data( The::PlaylistActions()->nextTrack() ) );
    m_next->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
}

void
Toolbar_3::setPlaying( bool on )
{
    if ( on )
        The::engineController()->play();
    else
        The::engineController()->pause();
}

bool
Toolbar_3::eventFilter( QObject *o, QEvent *ev )
{
    if ( ev->type() == QEvent::Enter )
    {
        if (o == m_next)
            m_next->setOpacity( 255 );
        else if (o == m_prev)
            m_prev->setOpacity( 255 );
        return false;
    }
    if ( ev->type() == QEvent::Leave )
    {
        if (o == m_next)
            m_next->setOpacity( 160 );
        else if (o == m_prev)
            m_prev->setOpacity( 128 );
        return false;
    }
    return false;
}

