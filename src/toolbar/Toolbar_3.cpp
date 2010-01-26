
#include "Toolbar_3.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"

#include "browsers/collectionbrowser/CollectionWidget.h"

#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/TimecodeLoadCapability.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/SliderWidget.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <QtDebug>

static const int sliderStretch = 18;
static const QString promoString = i18n( "Amarok your Music" );
static const int prevOpacity = 128;
static const int nextOpacity = 160;

Toolbar_3::Toolbar_3( QWidget *parent )
    : QToolBar( i18n( "Toolbar 3G" ), parent )
    , EngineObserver( The::engineController() )
    , m_lastTime( -1 )
{
    setObjectName( "Toolbar_3G" );
    
    EngineController *engine = The::engineController();
    setIconSize( QSize( 48, 48 ) );

    QWidget *spacerWidget = new QWidget(this);
    spacerWidget->setFixedWidth( 9 );
    addWidget( spacerWidget );

    m_playPause = new PlayPauseButton;
    m_playPause->setPlaying( engine->state() == Phonon::PlayingState );
    m_playPause->setFixedSize( 48, 48 );
    addWidget( m_playPause );
    connect ( m_playPause, SIGNAL( toggled(bool) ), this, SLOT( setPlaying(bool) ) );

    QWidget *info = new QWidget(this);
    QVBoxLayout *vl = new QVBoxLayout( info );
    
    m_prev.label = new AnimatedLabelStack(QStringList(), info);
    m_prev.label->setAnimated( false );
    m_prev.label->setOpacity( prevOpacity );
    m_prev.label->installEventFilter( this );
    m_prev.label->setAlign( Qt::AlignCenter );
    connect ( m_prev.label, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( back() ) );

    m_current.label = new AnimatedLabelStack( QStringList( promoString ), info );
    m_current.label->setBold( true );
    m_current.label->installEventFilter( this );
    connect ( m_current.label, SIGNAL( clicked(const QString&) ), this, SLOT( filter(const QString&) ) );

    m_next.label = new AnimatedLabelStack(QStringList(), info);
    m_next.label->setAnimated( false );
    m_next.label->setOpacity( nextOpacity );
    m_next.label->installEventFilter( this );
    m_next.label->setAlign( Qt::AlignCenter );
    connect ( m_next.label, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( next() ) );

    m_dummy.label = new AnimatedLabelStack(QStringList(), info);
    m_dummy.label->hide();

    vl->addItem( m_trackBarSpacer = new QSpacerItem(0, m_current.label->minimumHeight(), QSizePolicy::MinimumExpanding, QSizePolicy::Fixed ) );

    connect ( m_prev.label, SIGNAL( pulsing(bool) ), m_current.label, SLOT( setStill(bool) ) );
    connect ( m_next.label, SIGNAL( pulsing(bool) ), m_current.label, SLOT( setStill(bool) ) );

    
    m_progressLayout = new QHBoxLayout;
    m_progressLayout->addStretch( 3 );
    m_progressLayout->addWidget( m_timeLabel = new QLabel( this ) );
    m_progressLayout->setAlignment( m_timeLabel, Qt::AlignVCenter | Qt::AlignRight );
    m_timeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    m_progressLayout->addWidget( m_slider = new Amarok::TimeSlider( this ) );
    m_progressLayout->setStretchFactor( m_slider , sliderStretch );
    connect( m_slider, SIGNAL( sliderReleased( int ) ), The::engineController(), SLOT( seek( int ) ) );
    connect( m_slider, SIGNAL( valueChanged( int ) ), SLOT( setLabelTime( int ) ) );

    m_progressLayout->addWidget( m_trackActionBar = new QToolBar( this ) );
    m_trackActionBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_trackActionBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    m_trackActionBar->setIconSize( QSize( 16,16 ) );

    m_progressLayout->addStretch( 3 );
    vl->addLayout( m_progressLayout );

    
    addWidget( info );

    m_volume = new VolumeDial( this );
    m_volume->setRange( 0, 100);
    m_volume->setValue( engine->volume() );
    m_volume->setMute( engine->isMuted() );
    m_volume->setFixedSize( 48, 48 );
    addWidget( m_volume );
    connect ( m_volume, SIGNAL( valueChanged(int) ), engine, SLOT( setVolume(int) ) );
    connect ( m_volume, SIGNAL( muteToggled(bool) ), engine, SLOT( setMuted(bool) ) );

    spacerWidget = new QWidget(this);
    spacerWidget->setFixedWidth( 9 );
    addWidget( spacerWidget );

    connect ( The::playlistController(), SIGNAL( changed()), this, SLOT( updatePrevAndNext() ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodesUpdated(const QString*) ),
              this, SLOT( updateBookmarks(const QString*) ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodeAdded(const QString&, int) ),
              this, SLOT( addBookmark(const QString&, int) ) );
}

void
Toolbar_3::addBookmark( const QString &name, int milliSeconds )
{
    if ( m_slider )
        m_slider->drawTriangle( name, milliSeconds, false );
}

// Moves the label towards its target position by 2/3rds of the remaining distance
static void adjustLabelPos( QWidget *label, int targetX )
{
    QRect r = label->geometry();
    int d = targetX - r.x();
    if ( d )
    {
        d += (d>0) ? 1 : -1; // increase the distance abs value by one to force a ceil of the below fraction
        d = (2*d)/3;
        r.translate( d, 0 );
        label->setGeometry( r );
    }
}

void
Toolbar_3::animateTrackLabels()
{
    bool done = true;
    int x = m_trackBarSpacer->geometry().x();
    const int dx = m_trackBarSpacer->geometry().width() / 3;
    adjustLabelPos( m_prev.label, x );
    m_prev.label->setOpacity( prevOpacity );
    if (done)
        done = m_prev.label->geometry().x() == x;
    
    x += dx;
    adjustLabelPos( m_current.label, x );
    if (done)
        done = m_current.label->geometry().x() == x;
    
    x += dx;
    adjustLabelPos( m_next.label, x );
    m_next.label->setOpacity( nextOpacity );
    if (done)
        done = m_next.label->geometry().x() == x;

    adjustLabelPos( m_dummy.label, m_dummy.targetX );
    if ( m_dummy.label->geometry().x() == m_dummy.targetX )
        m_dummy.label->hide();
    else
        done = false;
    
    if ( done )
    {
        killTimer( m_trackBarAnimationTimer );
        m_trackBarAnimationTimer = 0;
    }
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

    switch ( currentState )
    {
    case Phonon::PlayingState:
        m_playPause->setPlaying( true );
        break;
    case Phonon::StoppedState:
        setLabelTime( -1 );
        // fall through
    case Phonon::PausedState:
        m_playPause->setPlaying( false );
        break;
    case Phonon::LoadingState:
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( !( track && m_current.key == track->uidUrl() ) )
        {
            setLabelTime( -1 );
            m_slider->setEnabled( false );
        }
        break;
    }
    default:
        break;
    }
}

void
Toolbar_3::filter( const QString &string )
{
    if ( CollectionWidget::instance() )
        CollectionWidget::instance()->setFilter( string );
}

void
Toolbar_3::layoutTrackBar()
{
    QRect r = m_trackBarSpacer->geometry();
    r.setWidth( r.width() / 3);
    m_prev.label->setGeometry( r );
    m_prev.label->setOpacity( prevOpacity );
    r.moveLeft( r.right() + 1 );
    m_current.label->setGeometry( r );
    r.moveLeft( r.right() + 1 );
    m_next.label->setGeometry( r );
    m_next.label->setOpacity( nextOpacity );
}

#define HAS_TAG(_TAG_) !track->_TAG_()->name().isEmpty()
#define TAG(_TAG_) track->_TAG_()->prettyName()
#define CONTAINS_TAG(_TAG_) contains( TAG(_TAG_), Qt::CaseInsensitive )

static QStringList metadata( Meta::TrackPtr track )
{
    QStringList list;
    QRegExp rx("(\\s+-\\s+|\\s*;\\s*|\\s*:\\s*)"); // this will split "all-in-one" filename tags
    if ( track )
    {
        if ( !track->name().isEmpty() )
        {
            QString title = track->prettyName();
            
            if ( title.length() > 50 ||
                 (HAS_TAG(artist) && title.CONTAINS_TAG(artist)) ||
                 (HAS_TAG(composer) && title.CONTAINS_TAG(composer)) ||
                 (HAS_TAG(album) && title.CONTAINS_TAG(album)) )
            {
                list << title.split( rx, QString::SkipEmptyParts );
            }
            else
            {
                list << title;
            }
        }

        if ( HAS_TAG(artist) && !list.CONTAINS_TAG(artist) )
            list << TAG(artist);
        else if ( HAS_TAG(composer) && !list.CONTAINS_TAG(composer) )
            list << TAG(composer);
        if ( HAS_TAG(album) && !list.CONTAINS_TAG(album) )
            list << TAG(album);
        if ( HAS_TAG(year) && TAG(year) != "0" ) // "0" years be empty?!
            list << TAG(year);
        if ( HAS_TAG(genre) && !list.CONTAINS_TAG(genre) )
            list << TAG(genre);

        /* other tags
        double score
        int rating
        qint64 length // ms
        int sampleRate
        int bitrate
        int trackNumber
        int discNumber
        uint lastPlayed
        uint firstPlayed
        int playCount
        QString type
        bool inCollection
        */
    }
    return list;
}

#undef HAS_TAG
#undef TAG

void
Toolbar_3::updatePrevAndNext()
{
    if ( !The::engineController()->currentTrack() )
    {
        m_prev.label->setData( QStringList() );
        m_next.label->setData( QStringList() );
        m_current.label->setUpdatesEnabled( true );
        return;
    }
    
    Meta::TrackPtr track = The::playlistActions()->prevTrack();
    m_prev.key = track ? track->uidUrl() : QString();
    m_prev.label->setData( metadata( track ) );
    m_prev.label->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    
    track = The::playlistActions()->nextTrack();
    m_next.key = track ? track->uidUrl() : QString();
    m_next.label->setData( metadata( track ) );
    m_next.label->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );

    // we may have disbaled it as otherwise the current label gets updated one eventcycle before prev & next
    // see ::engineTrackChanged()
    m_current.label->setUpdatesEnabled( true );
}

void
Toolbar_3::updateBookmarks( const QString *BookmarkName )
{
    DEBUG_BLOCK
    m_slider->clearTriangles();
    if ( Meta::TrackPtr track = The::engineController()->currentTrack() )
    {
        if ( track->hasCapabilityInterface( Meta::Capability::LoadTimecode ) )
        {
            Meta::TimecodeLoadCapability *tcl = track->create<Meta::TimecodeLoadCapability>();
            BookmarkList list = tcl->loadTimecodes();
            debug() << "found " << list.count() << " timecodes on this track";
            foreach( AmarokUrlPtr url, list )
            {
                if ( url->command() == "play" && url->args().keys().contains( "pos" ) )
                {
                    int pos = url->args().value( "pos" ).toInt() * 1000;
                    debug() << "showing timecode: " << url->name() << " at " << pos ;
                    m_slider->drawTriangle( url->name(), pos, ( BookmarkName && BookmarkName == url->name() ) );
                }
            }
            delete tcl;
        }
    }
}

void
Toolbar_3::engineTrackChanged( Meta::TrackPtr track )
{
    if ( m_trackBarAnimationTimer )
    {
        killTimer( m_trackBarAnimationTimer );
        m_trackBarAnimationTimer = 0;
    }
    setLabelTime( -1 );
    setActionsFrom( track );
    m_trackBarSpacer->changeSize(0, m_current.label->minimumHeight(), QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    if ( track )
    {
        m_current.key = track->uidUrl();
        m_current.label->setUpdatesEnabled( false );
        m_current.label->setData( metadata( track ) );
        m_current.label->setCursor( Qt::PointingHandCursor );

        // If all labels are in position and this is a single step for or back, we perform a slide
        // on the other two labels, i.e. e.g. move the prev to current label position and current
        // to the next and the animate the move into their target positions
        QRect r = m_trackBarSpacer->geometry();
        r.setWidth( r.width() / 3);
        if ( isVisible() && m_current.label->geometry().x() == r.x() + r.width() )
        {
            if ( m_current.key == m_next.key )
            {
                // left
                m_dummy.targetX = r.x() - r.width()/2;
                m_dummy.label->setGeometry( r );
                m_dummy.label->setData( m_prev.label->data() );
                m_dummy.label->show();
                // center
                r.moveLeft( r.right() + 1 );
                m_prev.label->setGeometry( r );
                // right
                r.moveLeft( r.right() + 1 );
                m_current.label->setGeometry( r );
                m_next.label->setGeometry( r );

                m_next.label->setOpacity( 0 );
                m_next.label->raise();

                animateTrackLabels();
                m_trackBarAnimationTimer = startTimer( 40 );
            }
            else if ( m_current.key == m_prev.key )
            {
                // left
                m_prev.label->setGeometry( r );
                m_current.label->setGeometry( r );
                // center
                r.moveLeft( r.right() + 1 );
                m_next.label->setGeometry( r );

                // right
                r.moveLeft( r.right() + 1 );
                m_dummy.targetX = r.x() + r.width()/2;
                m_dummy.label->setGeometry( r );
                m_dummy.label->setData( m_next.label->data() );
                m_dummy.label->show();

                m_prev.label->setOpacity( 0 );
                m_prev.label->raise();

                animateTrackLabels();
                m_trackBarAnimationTimer = startTimer( 40 );
            }
        }
        else
            layoutTrackBar();
    }
    else
    {
        m_current.key.clear();
        m_current.label->setData( QStringList( promoString ) );
        m_current.label->setCursor( Qt::ArrowCursor );
    }
    QTimer::singleShot( 0, this, SLOT( updatePrevAndNext() ) );
}

void
Toolbar_3::engineTrackLengthChanged( qint64 ms )
{
    m_slider->setRange( 0, ms );
    m_slider->setEnabled( ms > 0 );

    // get the urlid of the current track as the engine might stop and start several times
    // when skipping last.fm tracks, so we need to know if we are still on the same track...
    if ( Meta::TrackPtr track = The::engineController()->currentTrack() )
        m_current.key = track->uidUrl();

    updateBookmarks( 0 );
}

void
Toolbar_3::engineTrackPositionChanged( qint64 position, bool /*userSeek*/ )
{
    m_slider->setSliderValue( position );
//     if ( !m_slider->isEnabled() )
//         setLabelTime( position )
}

void
Toolbar_3::resizeEvent( QResizeEvent *ev )
{
    if ( ev->size().width() > 0 )
    {
        const int limit = 640;
        if ( ev->size().width() > limit )
        {
            m_progressLayout->setStretchFactor( m_slider, sliderStretch );
        }
        else
        {
            int s = qRound(float(limit)/ev->size().width());
            s *= s*s*sliderStretch;
            m_progressLayout->setStretchFactor( m_slider, qMax( sliderStretch, s ) );
        }
        layoutTrackBar();
    }
}

void
Toolbar_3::setActionsFrom( Meta::TrackPtr track )
{
    m_trackActionBar->clear();

    foreach ( QAction* action, The::globalCurrentTrackActions()->actions() )
        m_trackActionBar->addAction( action );
    
    if ( track && track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = track->create<Meta::CurrentTrackActionsCapability>();
        if ( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                m_trackActionBar->addAction( action );
        }
        delete cac;
    }
}

const char * timeString[4] = { "3:33", "33:33", "3:33:33", "33:33:33" };

static inline int
timeFrame( int secs )
{
    if ( secs < 10*60 ) // 9:59
        return 0;
    if ( secs < 60*60 ) // 59:59
        return 1;
    if ( secs < 10*60*60 ) // 9:59:59
        return 2;
    return 3; // 99:59:59
}

void Toolbar_3::setLabelTime( int ms )
{
    if ( ms < 0 ) // clear
    {
        m_timeLabel->setText( QString() );
        m_timeLabel->setMinimumWidth( 0 );
        m_trackActionBar->setMinimumWidth( 0 );
        m_lastTime = -1;
    }
    else
    {
        const int secs = ms/1000;
        if ( secs == m_lastTime )
            return;

        const int tf = timeFrame( secs );
        if ( tf != timeFrame( m_lastTime ) )
        {
            const int w = QFontMetrics( m_timeLabel->font() ).width( timeString[tf] );
            m_timeLabel->setMinimumWidth( w );
            m_trackActionBar->setMinimumWidth( w );
        }
        
        m_lastTime = secs;
        m_timeLabel->setText( Meta::secToPrettyTime( secs ) );
    }
}

void
Toolbar_3::setPlaying( bool on )
{
    if ( on )
        The::engineController()->play();
    else
        The::engineController()->pause();
}

void
Toolbar_3::showEvent( QShowEvent *ev )
{
    QToolBar::showEvent( ev );
    layoutTrackBar();
}

void
Toolbar_3::timerEvent( QTimerEvent *ev )
{
    if ( ev->timerId() == m_trackBarAnimationTimer )
        animateTrackLabels();
    else
        QToolBar::timerEvent( ev );
}

bool
Toolbar_3::eventFilter( QObject *o, QEvent *ev )
{
    if ( ev->type() == QEvent::MouseMove )
    {
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if ( mev->buttons() & Qt::LeftButton )
        if ( o == m_current.label || o == m_prev.label || o == m_next.label )
        {
            const int x = mev->globalPos().x();
            int d = x - m_dragLastX;
            m_dragLastX = x;
            const int globalDist = qAbs( x - m_dragStartX );
            if ( globalDist > m_prev.label->width() )
                return false; // constrain to one item width

            m_current.label->setGeometry( m_current.label->geometry().translated( d, 0 ) );
            m_prev.label->setGeometry( m_prev.label->geometry().translated( d, 0 ) );
            m_next.label->setGeometry( m_next.label->geometry().translated( d, 0 ) );
        }
        return false;
    }
    if ( ev->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if ( mev->button() == Qt::LeftButton )
        if ( o == m_current.label || o == m_prev.label || o == m_next.label )
        {
            static_cast<QWidget*>(o)->setCursor( Qt::SizeHorCursor );
            m_dragLastX = m_dragStartX = mev->globalPos().x();
        }
        return false;
    }
    if ( ev->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if ( mev->button() == Qt::LeftButton )
        if ( o == m_current.label || o == m_prev.label || o == m_next.label )
        {
            const int x = mev->globalPos().x();
            const int d = m_dragStartX - x;
            QRect r = m_trackBarSpacer->geometry();
            const int limit = r.width()/5; // 1/3 is too much, 1/6 to few

            // reset cursor
            AnimatedLabelStack *l = static_cast<AnimatedLabelStack*>(o);
            l->setCursor( l->data().isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor );

            // if this was a _real_ drag, silently release the mouse
            const bool silentRelease = qAbs(d) > 25;
            if ( silentRelease )
            {   // this is a drag, release secretly
                o->blockSignals( true );
                o->removeEventFilter( this );
                QMouseEvent mre( QEvent::MouseButtonRelease, mev->pos(), mev->globalPos(),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier );
                                 QCoreApplication::sendEvent( o, &mre );
                o->installEventFilter( this );
                o->blockSignals( false );
            }

            // if moved "far enough" jump to prev/next track
            // NOTICE the labels shall snap back _after_ the track has changed (in case)
            // as this is not reliable, we force a timered snapback sa well
            if ( d > limit )
            {
                The::playlistActions()->next();
                QTimer::singleShot(500, this, SLOT( layoutTrackBar() ) );
            }
            else if ( d < -limit )
            {
                The::playlistActions()->back();
                QTimer::singleShot(500, this, SLOT( layoutTrackBar() ) );
            }
            else
                layoutTrackBar();

            return silentRelease;
        }
        return false;
    }
    if ( ev->type() == QEvent::Enter )
    {
        if (o == m_next.label)
            m_next.label->setOpacity( 255 );
        else if (o == m_prev.label)
            m_prev.label->setOpacity( 255 );
        return false;
    }
    if ( ev->type() == QEvent::Leave )
    {
        if (o == m_next.label)
            m_next.label->setOpacity( 160 );
        else if (o == m_prev.label)
            m_prev.label->setOpacity( 128 );
        return false;
    }
    return false;
}

