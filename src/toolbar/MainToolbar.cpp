/****************************************************************************************
 * Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
 * Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "MainToolbar.h"

#include "amarokconfig.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "MainWindow.h"
#include "SvgHandler.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"

#include "browsers/collectionbrowser/CollectionWidget.h"

#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/TimecodeLoadCapability.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/SliderWidget.h"
#include "widgets/TrackActionButton.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

static const QString promoString = i18n( "Rediscover Your Music" );

// #define prev_next_role QPalette::Link
#define prev_next_role foregroundRole()
static const int prevOpacity = 128;
static const int nextOpacity = 160;

static const int icnSize = 48;

static const int leftRightSpacer = 15;
static const int timeLabelMargin = 6;
static const int skipPadding = 6;
static const int skipMargin = 6;
static const int constant_progress_ratio_minimum_width = 640;
static const int space_between_tracks_and_slider = 2;
static const float track_fontsize_factor = 1.1f;


MainToolbar::MainToolbar( QWidget *parent )
    : QToolBar( i18n( "Main Toolbar" ), parent )
    , EngineObserver( The::engineController() )
    , m_lastTime( -1 )
{
    setObjectName( "MainToolbar" );

    // control padding between buttons and labels, it's style controlled by default
    layout()->setSpacing( 0 );

    EngineController *engine = The::engineController();
    m_currentEngineState = The::engineController()->state();
    setIconSize( QSize( icnSize, icnSize ) );

    QWidget *spacerWidget = new QWidget(this);
    spacerWidget->setFixedWidth( leftRightSpacer );
    addWidget( spacerWidget );

    m_playPause = new PlayPauseButton;
    m_playPause->setPlaying( engine->state() == Phonon::PlayingState );
    m_playPause->setFixedSize( icnSize, icnSize );
    addWidget( m_playPause );
    connect ( m_playPause, SIGNAL( toggled(bool) ), this, SLOT( setPlaying(bool) ) );

    QWidget *info = new QWidget(this);
    QVBoxLayout *vl = new QVBoxLayout( info );

    QFont fnt = QApplication::font(); // don't use the toolbar font. Often small to support icons only.
    if ( fnt.pointSize() > 0 )
        fnt.setPointSize( qRound(fnt.pointSize() * track_fontsize_factor) );
    const int fntH = QFontMetrics( QApplication::font() ).height();
    
    m_skip_left = The::svgHandler()->renderSvg( "tiny_skip_left", 96*fntH/128, fntH, "tiny_skip_left" );
    m_skip_right = The::svgHandler()->renderSvg( "tiny_skip_right", 96*fntH/128, fntH, "tiny_skip_right" );

    m_prev.key = 0;
    m_prev.label = new AnimatedLabelStack(QStringList(), info);
    m_prev.label->setFont( fnt );
    if ( layoutDirection() == Qt::LeftToRight )
        m_prev.label->setPadding( m_skip_right.width() + skipPadding + skipMargin, 0 );
    else
        m_prev.label->setPadding( 0, m_skip_right.width() + skipPadding + skipMargin );
    m_prev.label->setAlign( Qt::AlignLeft );
    m_prev.label->setAnimated( false );
    m_prev.label->setOpacity( prevOpacity );
    m_prev.label->installEventFilter( this );
    m_prev.label->setForegroundRole( prev_next_role );
    connect ( m_prev.label, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( back() ) );

    m_current.label = new AnimatedLabelStack( QStringList( promoString ), info );
    m_current.label->setFont( fnt );
    m_current.label->setPadding( 24, 24 );
    m_current.label->setBold( true );
    m_current.label->setLayout( new QHBoxLayout );
    m_current.label->installEventFilter( this );
    connect ( m_current.label, SIGNAL( clicked(const QString&) ), Amarok::actionCollection()->action("show_active_track"), SLOT( trigger() ) );

    m_next.key = 0;
    m_next.label = new AnimatedLabelStack(QStringList(), info);
    m_next.label->setFont( fnt );
    if ( layoutDirection() == Qt::LeftToRight )
        m_next.label->setPadding( 0, m_skip_left.width() + skipPadding + skipMargin );
    else
        m_next.label->setPadding( m_skip_left.width() + skipPadding + skipMargin, 0 );
    m_next.label->setAlign( Qt::AlignRight );
    m_next.label->setAnimated( false );
    m_next.label->setOpacity( nextOpacity );
    m_next.label->installEventFilter( this );
    m_next.label->setForegroundRole( prev_next_role );
    connect ( m_next.label, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( next() ) );

    m_dummy.label = new AnimatedLabelStack(QStringList(), info);
    m_next.label->setFont( fnt );
    m_dummy.label->hide();

    vl->addItem( m_trackBarSpacer = new QSpacerItem(0, m_current.label->minimumHeight(), QSizePolicy::MinimumExpanding, QSizePolicy::Fixed ) );

    vl->addSpacing( space_between_tracks_and_slider );

    connect ( m_prev.label, SIGNAL( pulsing(bool) ), m_current.label, SLOT( setStill(bool) ) );
    connect ( m_next.label, SIGNAL( pulsing(bool) ), m_current.label, SLOT( setStill(bool) ) );

    m_timeLabel = new QLabel( info );
    m_timeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    
    m_slider = new Amarok::TimeSlider( info );
    connect( m_slider, SIGNAL( sliderReleased( int ) ), The::engineController(), SLOT( seek( int ) ) );
    connect( m_slider, SIGNAL( valueChanged( int ) ), SLOT( setLabelTime( int ) ) );
    connect( m_slider, SIGNAL( moodbarUsageChanged(bool) ), this, SLOT( layoutProgressBar() ) );

    m_remainingTimeLabel = new QLabel( info );
    m_remainingTimeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );

    const int pbsH = qMax( m_timeLabel->sizeHint().height(), m_slider->sizeHint().height() );
    vl->addItem( m_progressBarSpacer = new QSpacerItem(0, pbsH, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed ) );
    
    addWidget( info );

    m_volume = new VolumeDial( this );
    m_volume->setRange( 0, 100);
    m_volume->setValue( engine->volume() );
    m_volume->setMuted( engine->isMuted() );
    m_volume->setFixedSize( icnSize, icnSize );
    m_volume->addWheelProxies( QList<QWidget*>() << this << info
                                                 << m_prev.label << m_current.label << m_next.label
                                                 << m_timeLabel << m_remainingTimeLabel );
    addWidget( m_volume );
    connect ( m_volume, SIGNAL( valueChanged(int) ), engine, SLOT( setVolume(int) ) );
    connect ( m_volume, SIGNAL( muteToggled(bool) ), engine, SLOT( setMuted(bool) ) );

    spacerWidget = new QWidget(this);
    spacerWidget->setFixedWidth( leftRightSpacer );
    addWidget( spacerWidget );
}

void
MainToolbar::addBookmark( const QString &name, int milliSeconds )
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
        r.translate( qMin( qAbs(d), r.width()/6 ) * (d > 0 ? 1 : -1), 0 );
        label->setGeometry( r );
    }
}

void
MainToolbar::animateTrackLabels()
{
    bool done = true;

    int off = -m_current.label->parentWidget()->geometry().x();
    adjustLabelPos( m_prev.label, m_prev.rect.x() + off );
    m_prev.label->setOpacity( prevOpacity );
    if (done)
        done = m_prev.label->geometry().x() == m_prev.rect.x() + off;
    
    adjustLabelPos( m_current.label, m_current.rect.x() + off );
    if (done)
        done = m_current.label->geometry().x() == m_current.rect.x() + off;
    
    adjustLabelPos( m_next.label, m_next.rect.x() + off );
    m_next.label->setOpacity( nextOpacity );
    if (done)
        done = m_next.label->geometry().x() == m_next.rect.x() + off;

    adjustLabelPos( m_dummy.label, m_dummy.targetX );
    if ( m_dummy.label->geometry().x() == m_dummy.targetX )
        m_dummy.label->hide();
    else
        done = false;
    
    if ( done )
    {
        killTimer( m_trackBarAnimationTimer );
        setCurrentTrackActionsVisible( true );
        m_trackBarAnimationTimer = 0;
    }
}

void
MainToolbar::checkEngineState()
{
    Phonon::State newState = The::engineController()->state();
    if ( m_currentEngineState == newState )
        return;

    m_currentEngineState = newState;
    switch ( m_currentEngineState )
    {
        case Phonon::PlayingState:
            m_playPause->setPlaying( true );
            break;
        case Phonon::StoppedState:
            m_slider->setValue( m_slider->minimum() );
            m_slider->update(); // necessary to clean the moodbar...
            setLabelTime( -1 );
            // fall through
        case Phonon::PausedState:
            m_playPause->setPlaying( false );
            break;
        case Phonon::LoadingState:
        {
            Meta::TrackPtr track = The::engineController()->currentTrack();
            if ( !( track && m_current.uidUrl == track->uidUrl() ) )
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
MainToolbar::engineVolumeChanged( int percent )
{
    m_volume->setValue( percent );
}

void
MainToolbar::engineMuteStateChanged( bool mute )
{
    m_volume->setMuted( mute );
}

void
MainToolbar::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    if ( !isVisible() || currentState == oldState )
        return;
    // when changing a track, we get an interm pause what leads to stupid flicker
    // therefore we wait a few ms before we actually check the _then_ current state
    QTimer::singleShot( 100, this, SLOT( checkEngineState() ) );
}

void
MainToolbar::filter( const QString &string )
{
    if ( CollectionWidget::instance() )
        CollectionWidget::instance()->setFilter( string );
}

void
MainToolbar::layoutProgressBar()
{
    const int limit = constant_progress_ratio_minimum_width;
    const QRect r = m_progressBarSpacer->geometry();

    const int bw = AmarokConfig::showMoodbarInSlider() ? 10 : 6;
    int w = bw;
    if ( size().width() < limit )
    {
        w = (limit<<7)/size().width();
        w = w*w*w*bw;
        w /= (1<<21);
    }

    w = r.width() / w;
    int tlW = m_timeLabel->width();
    if ( tlW + timeLabelMargin > w )
        w = tlW;
    int rtlW = m_remainingTimeLabel->width();
    if ( rtlW + timeLabelMargin > w )
        w = rtlW;

    QRect pb = r.adjusted( w, 0, -w, 0 );
    m_slider->setGeometry( pb );

    QRect tlR( 0, 0, tlW, r.height() );
    QRect rtlR( 0, 0, rtlW, r.height() );

    if ( layoutDirection() == Qt::LeftToRight )
    {
        tlR.moveTopRight( pb.topLeft() - QPoint( timeLabelMargin, 0 ) );
        rtlR.moveTopLeft( pb.topRight() + QPoint( timeLabelMargin, 0 )  );
    }
    else
    {
        rtlR.moveTopRight( pb.topLeft() - QPoint( timeLabelMargin, 0 ) );
        tlR.moveTopLeft( pb.topRight() + QPoint( timeLabelMargin, 0 )  );
    }

    m_timeLabel->setGeometry( tlR );
    m_remainingTimeLabel->setGeometry( rtlR );
}

void
MainToolbar::layoutTrackBar()
{
    m_dummy.label->hide();
    // this is the label parenting widge ("info") offset
    const QPoint off = m_current.label->parentWidget()->geometry().topLeft();
    QRect r = m_trackBarSpacer->geometry();
    r.setWidth( r.width() / 3);
    int d = r.width();

    if ( layoutDirection() == Qt::RightToLeft )
    {
        d = -d;
        r.moveRight( m_trackBarSpacer->geometry().right() );
    }

    m_prev.rect = r.translated( off );
    m_prev.label->setGeometry( r );
    m_prev.label->setOpacity( prevOpacity );

    r.translate( d, 0 );
    m_current.rect = r.translated( off );
    m_current.label->setGeometry( r );

    r.translate( d, 0 );
    m_next.rect = r.translated( off );
    m_next.label->setGeometry( r );
    m_next.label->setOpacity( nextOpacity );

    setCurrentTrackActionsVisible( true );
}

void MainToolbar::updateLabels()
{
    engineTrackChanged( The::engineController()->currentTrack() );
}

void
MainToolbar::updateCurrentTrackActions()
{
    // wipe layout ================
    QLayoutItem *item;
    while ( (item = m_current.label->layout()->takeAt(0)) )
    {
        delete item->widget();
        delete item;
    }

    // collect actions ================
    QList<QAction*> actions;

    foreach( QAction* action, The::globalCurrentTrackActions()->actions() )
        actions << action;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track && track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = track->create<Meta::CurrentTrackActionsCapability>();
        if ( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                actions << action;
        }
        delete cac;
    }

    QHBoxLayout *hbl = static_cast<QHBoxLayout*>( m_current.label->layout() );
    hbl->setContentsMargins( 0, 0, 0, 0 );
    hbl->setSpacing( 3 );

    TrackActionButton *btn;
    const int n = actions.count() / 2;
    for ( int i = 0; i < actions.count(); ++i )
    {
        if ( i == n )
            hbl->addStretch( 10 );
        btn = new TrackActionButton( m_current.label, actions.at(i) );
        btn->installEventFilter( this );
        hbl->addWidget( btn );
    }
}

#define HAS_TAG(_TAG_) track->_TAG_() && !track->_TAG_()->name().isEmpty()
#define TAG(_TAG_) track->_TAG_()->prettyName()
#define CONTAINS_TAG(_TAG_) contains( TAG(_TAG_), Qt::CaseInsensitive )

static QStringList metadata( Meta::TrackPtr track )
{
    QStringList list;
    if ( track )
    {
        bool noTags = false;
        QString title = track->prettyName();
        if (( noTags = title.isEmpty() )) // should not happen
            title = track->prettyUrl();
        if (( noTags = title.isEmpty() )) // should never happen
            title = track->playableUrl().url();
        if (( noTags = title.isEmpty() )) // sth's MEGA wrong ;-)
            title = "???";

        // no tags -> probably filename. try to strip the suffix
        if ( noTags || track->name().isEmpty() )
        {
            noTags = true;
            int dot = title.lastIndexOf('.');
            if ( dot > 0 && title.length() - dot < 6 )
                title = title.left( dot );
        }

        // the track has no tags, is long or contains tags other than the titlebar
        // ==> separate
        if ( noTags || title.length() > 50 ||
             (HAS_TAG(artist) && title.CONTAINS_TAG(artist)) ||
             (HAS_TAG(composer) && title.CONTAINS_TAG(composer)) ||
             (HAS_TAG(album) && title.CONTAINS_TAG(album)) )
        {
            // this will split "all-in-one" filename tags
            QRegExp rx("(\\s+-\\s+|\\s*;\\s*|\\s*:\\s*)");
            list << title.split( rx, QString::SkipEmptyParts );
            QList<QString>::iterator i = list.begin();
            bool ok;
            while ( i != list.end() )
            {
                // check whether this entry is only a number, i.e. probably year or track #
                i->toInt( &ok );
                if ( ok )
                    i = list.erase( i );
                else
                    ++i;
            }
        }
        else // plain title
        {
            list << title;
        }

        if ( HAS_TAG(artist) && !list.CONTAINS_TAG(artist) )
            list << TAG(artist);
        else if ( HAS_TAG(composer) && !list.CONTAINS_TAG(composer) )
            list << TAG(composer);
        if ( HAS_TAG(album) && !list.CONTAINS_TAG(album) )
            list << TAG(album);

        /* other tags
        string year
        string genre
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
#undef CONTAINS_TAG

void
MainToolbar::updatePrevAndNext()
{
    if ( !The::engineController()->currentTrack() )
    {
        m_prev.key = 0L;
        m_prev.label->setForegroundRole( foregroundRole() );
        m_prev.label->setOpacity( 96 );
        m_prev.label->setData( QStringList() ); // << "[ " + i18n("Previous") + " ]" );
        m_prev.label->setCursor( Qt::ArrowCursor );
        m_next.key = 0L;
        m_next.label->setForegroundRole( foregroundRole() );
        m_next.label->setOpacity( 96 );
        m_next.label->setData( QStringList() ); // << "[ " + i18n("Next") + " ]"  );
        m_next.label->setCursor( Qt::ArrowCursor );
        m_current.label->setUpdatesEnabled( true );
        return;
    }

    // NOTICE: i don't like this, but the order is important.
    // Reason is the (current) behaviour of the RandomTrackNavigator
    // when the playlist is completed it will clear the history and reshuffle
    // to be able to sneakpeak the next track, it's necessary to trigger this with this query
    // if we'd query the previous track first, we'd get a track that's actually no more present after
    // the next track query. by this order we'll get a 0L track, what's also the navigators opinion
    // about its queue :-\ //
    bool needUpdate = false;
    bool hadKey = bool(m_next.key);
    Meta::TrackPtr track = The::playlistActions()->likelyNextTrack();
    m_next.key = track ? track.data() : 0L;
    m_next.label->setForegroundRole( prev_next_role );
    m_next.label->setOpacity( nextOpacity );
    m_next.label->setData( metadata( track ) );
    m_next.label->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    if ( hadKey != bool(m_next.key) )
        needUpdate = true;

    hadKey = bool(m_prev.key);
    track = The::playlistActions()->likelyPrevTrack();
    m_prev.key = track ? track.data() : 0L;
    m_prev.label->setForegroundRole( prev_next_role );
    m_next.label->setOpacity( prevOpacity );
    m_prev.label->setData( metadata( track ) );
    m_prev.label->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    if ( hadKey != bool(m_prev.key) )
        needUpdate = true;

    // we may have disbaled it as otherwise the current label gets updated one eventcycle before prev & next
    // see ::engineTrackChanged()
    m_current.label->setUpdatesEnabled( true );

    if ( needUpdate )
        update();

    // unanimated change, probably by sliding the bar - fix label positions
    if ( !m_trackBarAnimationTimer )
        layoutTrackBar();
}

void
MainToolbar::updateBookmarks( const QString *BookmarkName )
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
                    int pos = url->args().value( "pos" ).toDouble() * 1000;
                    debug() << "showing timecode: " << url->name() << " at " << pos ;
                    m_slider->drawTriangle( url->name(), pos, ( BookmarkName && BookmarkName == url->name() ) );
                }
            }
            delete tcl;
        }
    }
}

void
MainToolbar::engineTrackChanged( Meta::TrackPtr track )
{
    if ( !isVisible() )
        return;
    if ( m_trackBarAnimationTimer )
    {
        killTimer( m_trackBarAnimationTimer );
        m_trackBarAnimationTimer = 0;
    }

    if ( track )
    {
        m_current.key = track.data();
        m_current.uidUrl = track->uidUrl();
        m_current.label->setUpdatesEnabled( false );
        m_current.label->setData( metadata( track ) );
        m_current.label->setCursor( Qt::PointingHandCursor );
        updateCurrentTrackActions();

        // If all labels are in position and this is a single step for or back, we perform a slide
        // on the other two labels, i.e. e.g. move the prev to current label position and current
        // to the next and the animate the move into their target positions
        QRect r = m_trackBarSpacer->geometry();
        r.setWidth( r.width() / 3 );
        int d = r.width();

        if ( layoutDirection() == Qt::RightToLeft )
        {
            d = -d;
            r.moveRight( m_trackBarSpacer->geometry().right() );
        }

        if ( isVisible() &&  m_current.label->geometry().x() == r.x() + d )
        {
            if ( m_current.key == m_next.key && m_current.key != m_prev.key )
            {
                setCurrentTrackActionsVisible( false );
                
                // left
                m_dummy.targetX = r.x() - d;
//                 if ( d < 0 ) // rtl
//                     m_dummy.targetX -= d;
                m_dummy.label->setGeometry( r );
                m_dummy.label->setData( m_prev.label->data() );
                m_dummy.label->show();
                // center
                r.translate( d, 0 );
                m_prev.label->setGeometry( r );
                // right
                r.translate( d, 0 );
                m_current.label->setGeometry( r );
                m_next.label->setGeometry( r );
                m_next.label->setOpacity( 0 );
                m_next.label->raise();

                animateTrackLabels();
                m_trackBarAnimationTimer = startTimer( 40 );
            }
            else if ( m_current.key == m_prev.key )
            {
                setCurrentTrackActionsVisible( false );
                
                // left
                m_prev.label->setGeometry( r );
                m_current.label->setGeometry( r );
                // center
                r.translate( d, 0 );
                m_next.label->setGeometry( r );

                // right
                r.translate( d, 0 );
                m_dummy.targetX = r.x() + d;
                m_dummy.label->setGeometry( r );
                m_dummy.label->setData( m_next.label->data() );
                m_dummy.label->show();

                m_prev.label->setOpacity( 0 );
                m_prev.label->raise();

                animateTrackLabels();
                m_trackBarAnimationTimer = startTimer( 40 );
            }
        }
    }
    else
    {
        setLabelTime( -1 );
        m_slider->setValue( m_slider->minimum() );
        m_current.key = 0L;
        m_current.uidUrl.clear();
        m_current.label->setData( QStringList( promoString ) );
        m_current.label->setCursor( Qt::ArrowCursor );
    }

    m_trackBarSpacer->changeSize(0, m_current.label->minimumHeight(), QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    const int pbsH = qMax( m_timeLabel->sizeHint().height(), m_slider->sizeHint().height() );
    m_progressBarSpacer->changeSize(0, pbsH, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    QTimer::singleShot( 0, this, SLOT( updatePrevAndNext() ) );
}

void
MainToolbar::engineTrackLengthChanged( qint64 ms )
{
    m_slider->setRange( 0, ms );
    m_slider->setEnabled( ms > 0 );

    // get the urlid of the current track as the engine might stop and start several times
    // when skipping last.fm tracks, so we need to know if we are still on the same track...
    if ( Meta::TrackPtr track = The::engineController()->currentTrack() )
        m_current.uidUrl = track->uidUrl();

    updateBookmarks( 0 );
}

void
MainToolbar::engineTrackPositionChanged( qint64 position, bool /*userSeek*/ )
{
    if ( m_slider->isEnabled() )
        m_slider->setSliderValue( position );
    else
        setLabelTime( position );
}

void
MainToolbar::hideEvent( QHideEvent *ev )
{
    QToolBar::hideEvent( ev );
    disconnect ( The::playlistController(), SIGNAL( changed()), this, SLOT( updatePrevAndNext() ) );
    disconnect ( Playlist::ModelStack::instance()->source(), SIGNAL( queueChanged() ), this, SLOT( updatePrevAndNext() ) );
    disconnect ( Playlist::ModelStack::instance()->source(), SIGNAL( metadataUpdated() ), this, SLOT( updateLabels() ) );
    disconnect ( The::playlistActions(), SIGNAL( navigatorChanged()), this, SLOT( updatePrevAndNext() ) );
    disconnect ( The::amarokUrlHandler(), SIGNAL( timecodesUpdated(const QString*) ),
                 this, SLOT( updateBookmarks(const QString*) ) );
    disconnect ( The::amarokUrlHandler(), SIGNAL( timecodeAdded(const QString&, int) ),
                 this, SLOT( addBookmark(const QString&, int) ) );
    layoutTrackBar();
    layoutProgressBar();
}

void
MainToolbar::paintEvent( QPaintEvent *ev )
{
    // let the style paint draghandles and in doubt override the shadow from above
    QToolBar::paintEvent( ev );

    // skip icons
    QPainter p( this );
    Skip *left = &m_prev;
    Skip *right = &m_next;
    
    if ( layoutDirection() == Qt::RightToLeft )
    {
        left = &m_next;
        right = &m_prev;
    }

    if ( left->key )
        p.drawPixmap( left->rect.left() + skipMargin,
                      left->rect.y() + ( left->rect.height() - m_skip_left.height() ) /2,
                      m_skip_left );
    if ( right->key )
        p.drawPixmap( right->rect.right() - ( m_skip_right.width() + skipMargin ),
                      right->rect.y() + ( right->rect.height() - m_skip_right.height() ) /2,
                      m_skip_right );
    p.end();
}


void
MainToolbar::resizeEvent( QResizeEvent *ev )
{
    if ( ev->size().width() > 0 && ev->size().width() != ev->oldSize().width() )
    {
        layoutProgressBar();
        layoutTrackBar();
    }
}

void
MainToolbar::setCurrentTrackActionsVisible( bool vis )
{
    if ( m_current.actionsVisible == vis )
        return;
    m_current.actionsVisible = vis;
    QLayoutItem *item;
    for ( int i = 0; i < m_current.label->layout()->count(); ++i )
    {
        item = m_current.label->layout()->itemAt( i );
        if ( item->widget() )
            item->widget()->setVisible( vis );
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

void
MainToolbar::setLabelTime( int ms )
{
    bool relayout = false;
    if ( ms < 0 ) // clear
    {
        m_timeLabel->hide();
        m_remainingTimeLabel->hide();
        m_lastTime = -1;
        m_lastRemainingTime = -1;
        relayout = true;
    }
    else if ( isVisible() ) // no need to do expensive stuff - it's updated every second anyway
    {
        
        const int secs = ms/1000;
        const int remainingSecs =  m_slider->maximum() > 0 ? (m_slider->maximum() - ms) / 1000 : 0;
        
        if ( secs == m_lastTime && remainingSecs == m_lastRemainingTime )
            return;

        m_timeLabel->setText( Meta::secToPrettyTime( secs ) );

        int tf = timeFrame( secs );
        if ( m_lastTime < 0 || tf != timeFrame( m_lastTime ) )
        {
            const int w = QFontMetrics( m_timeLabel->font() ).width( timeString[tf] );
            m_timeLabel->setFixedWidth( w );
            relayout = true;
        }
        m_timeLabel->show();

        if ( remainingSecs > 0 )
        {
            m_remainingTimeLabel->setText( '-' + Meta::secToPrettyTime( remainingSecs ) );
            tf = timeFrame( remainingSecs );
            if ( m_lastRemainingTime < 0 || tf != timeFrame( m_lastRemainingTime ) )
            {
                const int w = QFontMetrics( m_remainingTimeLabel->font() ).width( QString("-") + timeString[tf] );
                m_remainingTimeLabel->setFixedWidth( w );
                relayout = true;
            }
            m_remainingTimeLabel->show();
        }
        else
            m_remainingTimeLabel->hide();

        m_lastTime = secs;
        m_lastRemainingTime = remainingSecs;

    }
    
    if (relayout)
        layoutProgressBar();
}

void
MainToolbar::setPlaying( bool on )
{
    if ( on )
        The::engineController()->play();
    else
        The::engineController()->pause();
}

void
MainToolbar::showEvent( QShowEvent *ev )
{
    connect ( The::playlistController(), SIGNAL( changed()), this, SLOT( updatePrevAndNext() ) );
    connect ( Playlist::ModelStack::instance()->source(), SIGNAL( queueChanged() ), this, SLOT( updatePrevAndNext() ) );
    connect ( Playlist::ModelStack::instance()->source(), SIGNAL( metadataUpdated() ), this, SLOT( updateLabels() ) );
    connect ( The::playlistActions(), SIGNAL( navigatorChanged()), this, SLOT( updatePrevAndNext() ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodesUpdated(const QString*) ),
              this, SLOT( updateBookmarks(const QString*) ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodeAdded(const QString&, int) ),
              this, SLOT( addBookmark(const QString&, int) ) );
    QToolBar::showEvent( ev );
    engineTrackChanged( The::engineController()->currentTrack() );
    checkEngineState();
    layoutTrackBar();
    layoutProgressBar();
}

void
MainToolbar::timerEvent( QTimerEvent *ev )
{
    if ( ev->timerId() == m_trackBarAnimationTimer )
        animateTrackLabels();
    else
        QToolBar::timerEvent( ev );
}

bool
MainToolbar::eventFilter( QObject *o, QEvent *ev )
{
    if ( ev->type() == QEvent::MouseMove )
    {
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if ( mev->buttons() & Qt::LeftButton )
        if ( o == m_current.label || o == m_prev.label || o == m_next.label )
        {
            setCurrentTrackActionsVisible( false );
            const int x = mev->globalPos().x();
            int d = x - m_drag.lastX;
            m_drag.lastX = x;
            const int globalDist = qAbs( x - m_drag.startX );
            if ( globalDist > m_drag.max )
                m_drag.max = globalDist;
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
            m_drag.max = 0;
            m_drag.lastX = m_drag.startX = mev->globalPos().x();
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
            const int d = m_drag.startX - x;
            QRect r = m_trackBarSpacer->geometry();
            const int limit = r.width()/5; // 1/3 is too much, 1/6 to few

            // reset cursor
            AnimatedLabelStack *l = static_cast<AnimatedLabelStack*>(o);
            l->setCursor( l->data().isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor );

            // if this was a _real_ drag, silently release the mouse
            const bool silentRelease = m_drag.max > 25;
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
        if (o == m_next.label && m_next.key)
            m_next.label->setOpacity( 255 );
        else if (o == m_prev.label && m_prev.key)
            m_prev.label->setOpacity( 255 );
        else if ( o->parent() == m_current.label ) // trackaction
        {
            QEvent e( QEvent::Leave );
            QCoreApplication::sendEvent( m_current.label, &e );
        }
        return false;
    }
    if ( ev->type() == QEvent::Leave )
    {
        if (o == m_next.label && m_next.key)
            m_next.label->setOpacity( nextOpacity );
        else if (o == m_prev.label && m_prev.key)
            m_prev.label->setOpacity( prevOpacity );
        else if ( o->parent() == m_current.label &&  // trackaction
                  m_current.label->rect().contains( m_current.label->mapFromGlobal(QCursor::pos()) ) )
        {
            QEvent e( QEvent::Enter );
            QCoreApplication::sendEvent( m_current.label, &e );
        }
        return false;
    }
    return false;
}


#include "MainToolbar.moc"
