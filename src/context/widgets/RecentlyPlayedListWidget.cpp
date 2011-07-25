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

#include "RecentlyPlayedListWidget.h"
#include "CollectionManager.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"

#include <KIcon>
#include <KSqueezedTextLabel>
#include <Plasma/IconWidget>

#include <QDateTime>
#include <QFontMetricsF>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>

RecentlyPlayedListWidget::RecentlyPlayedListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_trackIcon( KIcon( QLatin1String("media-album-track") ) )
{
    QGraphicsWidget *content = new QGraphicsWidget( this );
    m_layout = new QGraphicsLinearLayout( Qt::Vertical, content );
    setWidget( content );

    EngineController *ec = The::engineController();
    m_currentTrack = ec->currentTrack();
    connect( ec, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(trackChanged(Meta::TrackPtr)) );
    startQuery();
}

RecentlyPlayedListWidget::~RecentlyPlayedListWidget()
{
    clear();
}

void
RecentlyPlayedListWidget::addTrack( const Meta::TrackPtr &track )
{
    if( !track )
        return;
    if( !track->lastPlayed().isValid() )
        return;

    const QString &name = track->prettyName();
    if( name.isEmpty() )
        return;

    QFont font;
    QFontMetricsF fm( font );

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

    QLabel *lastPlayed = new QLabel( Amarok::verboseTimeSince( track->lastPlayed() ) );
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

    uint time = track->lastPlayed().toTime_t();
    m_items.insertMulti( time, itemLayout );
    int index = 0;
    QMapIterator<uint, QGraphicsLayoutItem*> i( m_items );
    while( i.hasNext() )
    {
        i.next();
        if( i.key() == time )
            break;
        ++index;
    }
    m_layout->insertItem( m_layout->count() - index, itemLayout );
}

void
RecentlyPlayedListWidget::startQuery()
{
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
    // "track" is the new song being played, so it hasn't finished yet.
    if( track == m_currentTrack )
        return;
    addTrack( m_currentTrack );
    m_currentTrack = track;
    removeLast();
}

void
RecentlyPlayedListWidget::clear()
{
    prepareGeometryChange();
    int count = m_layout->count();
    while( --count >= 0 )
        removeItem( m_layout->itemAt( 0 ) );
    m_items.clear();
    m_currentTrack.clear();
    m_recentTracks.clear();
}

void
RecentlyPlayedListWidget::tracksReturned( Meta::TrackList tracks )
{
    m_recentTracks << tracks;
}

void
RecentlyPlayedListWidget::setupTracksData()
{
    DEBUG_BLOCK
    foreach( const Meta::TrackPtr &track, m_recentTracks )
        addTrack( track );
    m_recentTracks.clear();
    PERF_LOG( "Done setting up recently played tracks" );
}

void
RecentlyPlayedListWidget::removeLast()
{
    if( m_items.isEmpty() )
        return;

    QGraphicsLayoutItem *item = m_items.begin().value();
    m_items.erase( m_items.begin() );
    removeItem( item );
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
