/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#define DEBUG_PREFIX "RecentlyPlayedListWidget"

#include "RecentlyPlayedListWidget.h"

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistController.h"

#include <KSqueezedTextLabel>
#include <Plasma/IconWidget>

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QLabel>
#include <QStringList>
#include <QVariant>
#include <QTimer>

ClickableGraphicsWidget::ClickableGraphicsWidget( const QString &url,
                                                  QGraphicsItem *parent,
                                                  Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , m_url( url )
{
    setAcceptHoverEvents( true );
    setCursor( Qt::PointingHandCursor );
}

ClickableGraphicsWidget::~ClickableGraphicsWidget()
{
}

void
ClickableGraphicsWidget::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    setOpacity( 0.5 );
    update();
}

void
ClickableGraphicsWidget::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    setOpacity( 1 );
    update();
}

void
ClickableGraphicsWidget::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
}

void
ClickableGraphicsWidget::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    if( !m_url.isEmpty() )
    {
        if( event->button() == Qt::LeftButton )
            emit leftClicked( m_url );
        else if( event->button() == Qt::MidButton )
            emit middleClicked( m_url );
    }
}

TimeDifferenceLabel::TimeDifferenceLabel( const QDateTime &eventTime , QWidget *parent,
                                          Qt::WindowFlags wFlags )
    : QLabel( parent, wFlags )
    , m_eventTime( eventTime )
{
    update();
}

TimeDifferenceLabel::~TimeDifferenceLabel()
{
}

void
TimeDifferenceLabel::update()
{
    setText( Amarok::verboseTimeSince( m_eventTime ) );
}

RecentlyPlayedListWidget::RecentlyPlayedListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_layout( new QGraphicsLinearLayout( Qt::Vertical ) )
    , m_trackIcon( QIcon::fromTheme( "media-album-track") )
{
    QGraphicsWidget *content = new QGraphicsWidget;
    content->setLayout( m_layout );
    setWidget( content );

    connect( EngineController::instance(), SIGNAL(trackChanged(Meta::TrackPtr)),
             SLOT(trackChanged(Meta::TrackPtr)) );

    m_updateTimer = new QTimer( this );
    m_updateTimer->start( 20 * 1000 );

    // Load saved data
    const KConfigGroup group = Amarok::config( "Recently Played" );
    const QVariantList recentlyPlayed = group.readEntry( "Last Played Dates", QVariantList() );
    const QStringList displayNames = group.readEntry( "Display Names", QStringList() );
    const QStringList trackUrls = group.readEntry( "Urls", QStringList() );
    for( int i = 0; i < trackUrls.size(); ++i )
        addTrack( recentlyPlayed[i].toDateTime(), displayNames[i], trackUrls[i] );
}

RecentlyPlayedListWidget::~RecentlyPlayedListWidget()
{
    QVariantList recentlyPlayed;
    QStringList displayNames;
    QStringList trackUrls;
    foreach( const RecentlyPlayedTrackData &data, m_recentTracks )
    {
        recentlyPlayed.append( data.recentlyPlayed );
        displayNames.append( data.displayName );
        trackUrls.append( data.trackUrl );
    }

    KConfigGroup group = Amarok::config( "Recently Played" );
    group.writeEntry( "Last Played Dates", recentlyPlayed );
    group.writeEntry( "Display Names", displayNames );
    group.writeEntry( "Urls", trackUrls );
    group.sync();
}

QGraphicsWidget*
RecentlyPlayedListWidget::addWidgetItem( const RecentlyPlayedTrackData &data )
{
    KSqueezedTextLabel *squeezer = new KSqueezedTextLabel( data.displayName );
    squeezer->setTextElideMode( Qt::ElideRight );
    squeezer->setAttribute( Qt::WA_NoSystemBackground );
    squeezer->setCursor( Qt::PointingHandCursor );

    QGraphicsProxyWidget *labelWidget = new QGraphicsProxyWidget;
    labelWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    labelWidget->setWidget( squeezer );

    TimeDifferenceLabel *lastPlayed = new TimeDifferenceLabel( data.recentlyPlayed );
    lastPlayed->setAttribute( Qt::WA_NoSystemBackground );
    lastPlayed->setAlignment( Qt::AlignRight );
    lastPlayed->setWordWrap( false );
    lastPlayed->setCursor( Qt::PointingHandCursor );
    connect( m_updateTimer, SIGNAL(timeout()), lastPlayed, SLOT(update()) );

    QGraphicsProxyWidget *lastPlayedWidget = new QGraphicsProxyWidget;
    lastPlayedWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    lastPlayedWidget->setWidget( lastPlayed );

    Plasma::IconWidget *icon = new Plasma::IconWidget;
    QSizeF iconSize = icon->sizeFromIconSize( QFontMetricsF(QFont()).height() );
    icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    icon->setMinimumSize( iconSize );
    icon->setMaximumSize( iconSize );
    icon->setIcon( m_trackIcon );

    QGraphicsLinearLayout *itemLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    itemLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    itemLayout->setContentsMargins( 0, 0, 0, 0 );
    itemLayout->addItem( icon );
    itemLayout->addItem( labelWidget );
    itemLayout->addItem( lastPlayedWidget );

    ClickableGraphicsWidget *itemWidget = new ClickableGraphicsWidget( data.trackUrl );
    itemWidget->setLayout( itemLayout );
    connect( itemWidget, SIGNAL(leftClicked(QString)), SLOT(itemLeftClicked(QString)) );
    connect( itemWidget, SIGNAL(middleClicked(QString)), SLOT(itemMiddleClicked(QString)) );

    m_layout->insertItem( 0, itemWidget );

    return itemWidget;
}

void
RecentlyPlayedListWidget::itemLeftClicked( const QString &url )
{
    Playlist::Controller::instance()->insertOptioned( QUrl( url ),
                                                Playlist::OnDoubleClickOnSelectedItems );
}

void
RecentlyPlayedListWidget::itemMiddleClicked( const QString &url )
{
    Playlist::Controller::instance()->insertOptioned( QUrl( url ),
                                                Playlist::OnMiddleClickOnSelectedItems );
}

void
RecentlyPlayedListWidget::addTrack( const Meta::TrackPtr &track )
{
    const Meta::ArtistPtr artist = track->artist();
    const QString displayName = !artist || artist->prettyName().isEmpty()
            ? track->prettyName()
            : i18nc( "%1 is artist, %2 is title", "%1 - %2",
                     artist->prettyName(), track->prettyName() );

    addTrack( QDateTime::currentDateTime(), displayName, track->uidUrl() );
}

void RecentlyPlayedListWidget::addTrack( const QDateTime &recentlyPlayed,
                                         const QString &displayName,
                                         const QString &trackUrl )
{
    while( m_recentTracks.size() >= 10 )
    {
        // Get rid of the least recent entry
        RecentlyPlayedTrackData data = m_recentTracks.dequeue();
        delete data.widget;
    }

    RecentlyPlayedTrackData data;
    data.recentlyPlayed = recentlyPlayed;
    data.displayName = displayName;
    data.trackUrl = trackUrl;
    data.widget = addWidgetItem( data );
    m_recentTracks.enqueue( data );
}

void
RecentlyPlayedListWidget::trackChanged( const Meta::TrackPtr &track )
{
    // Nothing has changed
    if( m_currentTrack == track )
        return;

    // lastTrack will be null if we're resuming from a stopped state
    Meta::TrackPtr lastTrack = m_currentTrack;
    m_currentTrack = track;

    if( lastTrack )
        addTrack( lastTrack );
}
