/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "EngineController.h"
#include "RecentlyPlayedListWidget.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KIcon>
#include <KSqueezedTextLabel>
#include <Plasma/IconWidget>

#include <QDateTime>
#include <QFontMetricsF>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QTimer>

RecentlyPlayedListWidget::RecentlyPlayedListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_trackIcon( KIcon( QLatin1String("media-album-track") ) )
{
    QGraphicsWidget *content = new QGraphicsWidget( this );
    m_layout = new QGraphicsLinearLayout( Qt::Vertical, content );
    setWidget( content );

    EngineController *ec = The::engineController();
    connect( ec, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(trackChanged(Meta::TrackPtr)) );
    QTimer::singleShot( 0, this, SLOT(startQuery()) ); // let engine controller have time to initialize
}

RecentlyPlayedListWidget::~RecentlyPlayedListWidget()
{
    clear();
}

void
RecentlyPlayedListWidget::updateWidget()
{
    DEBUG_BLOCK
    QFont font;
    QFontMetricsF fm( font );
    QMap<uint, Meta::TrackPtr> tracks = m_recentTracks;
    m_recentTracks.clear();

    foreach( const Meta::TrackPtr &track, tracks )
    {
        QString name = track->prettyName();

        QString labelText;
        Meta::ArtistPtr artist = track->artist();
        if( artist && !artist->prettyName().isEmpty() )
            labelText = QString( "%1 - %2" ).arg( artist->prettyName(), name );
        else
            labelText = name;

        KSqueezedTextLabel *squeezer = new KSqueezedTextLabel( labelText );
        squeezer->setTextElideMode( Qt::ElideRight );
        squeezer->setAttribute( Qt::WA_NoSystemBackground );

        QGraphicsProxyWidget *labelWidget = new QGraphicsProxyWidget( this );
        labelWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
        labelWidget->setWidget( squeezer );

        QLabel *lastPlayed = new QLabel( Amarok::verboseTimeSince( track->statistics()->lastPlayed() ) );
        lastPlayed->setAttribute( Qt::WA_NoSystemBackground );
        lastPlayed->setAlignment( Qt::AlignRight );
        lastPlayed->setWordWrap( false );

        QGraphicsProxyWidget *lastPlayedWidget = new QGraphicsProxyWidget( this );
        lastPlayedWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        lastPlayedWidget->setWidget( lastPlayed );

        Plasma::IconWidget *icon = new Plasma::IconWidget( this );
        QSizeF iconSize = icon->sizeFromIconSize( fm.height() );
        icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        icon->setMinimumSize( iconSize );
        icon->setMaximumSize( iconSize );
        icon->setIcon( m_trackIcon );

        QGraphicsLinearLayout *itemLayout = new QGraphicsLinearLayout( Qt::Horizontal );
        itemLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
        itemLayout->addItem( icon );
        itemLayout->addItem( labelWidget );
        itemLayout->addItem( lastPlayedWidget );
        m_layout->insertItem( 0, itemLayout );
    }
}

void
RecentlyPlayedListWidget::addTrack( const Meta::TrackPtr &track )
{
    if( !track )
        return;
    if( !track->statistics()->lastPlayed().isValid() )
        return;

    if( m_recentTracks.size() > 12 )
        m_recentTracks.erase( m_recentTracks.begin() );

    m_recentTracks.insert( track->statistics()->lastPlayed().toTime_t(), track );
}

void
RecentlyPlayedListWidget::startQuery()
{
    if( The::engineController()->isPlaying() )
        return;

    DEBUG_BLOCK
    PERF_LOG( "Start query recently played tracks" );
    clear();

    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    connect( qm, SIGNAL(newResultReady( Meta::TrackList)),
             this, SLOT(tracksReturned( Meta::TrackList)) );
    connect( qm, SIGNAL(queryDone()), SLOT(setupTracksData()) );

    qm->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Track )
      ->excludeFilter( Meta::valTitle, QString(), true, true )
      ->orderBy( Meta::valLastPlayed, true )
      ->excludeFilter( Meta::valLastPlayed, "2147483647" )
      ->limitMaxResultSize( 10 )
      ->run();
}

void
RecentlyPlayedListWidget::trackChanged( Meta::TrackPtr track )
{
    // engine controller will give a null track when playback stops
    if( !track )
    {
        addTrack( m_currentTrack );
        updateWidget();
        return;
    }

    // "track" is the new song being played, so it hasn't finished yet.
    if( track == m_currentTrack )
        return;

    // a null m_current_track means playback just started for the first time
    if( !m_currentTrack )
    {
        m_currentTrack = track;
        return;
    }

    addTrack( m_currentTrack );
    m_currentTrack = track;

    if( !The::engineController()->isPlaying() )
        updateWidget();
}

void
RecentlyPlayedListWidget::clear()
{
    prepareGeometryChange();
    int count = m_layout->count();
    while( --count >= 0 )
        removeItem( m_layout->itemAt( 0 ) );
    m_recentTracks.clear();
}

void
RecentlyPlayedListWidget::tracksReturned( Meta::TrackList tracks )
{
    foreach( const Meta::TrackPtr &track, tracks )
        m_recentTracks.insert( track->statistics()->lastPlayed().toTime_t(), track );
}

void
RecentlyPlayedListWidget::setupTracksData()
{
    DEBUG_BLOCK
    foreach( const Meta::TrackPtr &track, m_recentTracks )
        addTrack( track );

    if( !The::engineController()->isPlaying() )
        updateWidget();
    PERF_LOG( "Done setting up recently played tracks" );
}

void
RecentlyPlayedListWidget::removeItem( QGraphicsLayoutItem *item )
{
    QGraphicsLinearLayout *layout = static_cast<QGraphicsLinearLayout*>( item );
    m_layout->removeItem( layout );
    int count = layout->count();
    while( --count >= 0 )
        delete layout->itemAt( 0 );
    delete layout;
}

#include "RecentlyPlayedListWidget.moc"
