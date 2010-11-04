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
{
    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    QGraphicsWidget *content = new QGraphicsWidget( this );
    content->setLayout( m_layout );
    setWidget( content );

    EngineController *ec = The::engineController();
    m_currentTrack = ec->currentTrack();
    connect( ec, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(trackChanged(Meta::TrackPtr)) );
}

RecentlyPlayedListWidget::~RecentlyPlayedListWidget()
{
    clear();
}

void
RecentlyPlayedListWidget::addTrack( const Meta::TrackPtr &track )
{
    const QString &name = track->prettyName();
    if( name.isEmpty() )
        return;

    QFont font;
    QFontMetricsF fm( font );
    QGraphicsLinearLayout *itemLayout = new QGraphicsLinearLayout;

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

    Plasma::Label *lastPlayed = new Plasma::Label( this );
    lastPlayed->setText( Amarok::verboseTimeSince( track->lastPlayed() ) );
    lastPlayed->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    lastPlayed->setAlignment( Qt::AlignRight );
    lastPlayed->nativeWidget()->setWordWrap( false );

    Plasma::IconWidget *icon = new Plasma::IconWidget( this );
    QSizeF iconSize = icon->sizeFromIconSize( fm.height() );
    icon->setMinimumSize( iconSize );
    icon->setMaximumSize( iconSize );
    icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    icon->setIcon( KIcon( QLatin1String("media-album-track") ) );

    itemLayout->addItem( icon );
    itemLayout->addItem( labelWidget );
    itemLayout->addItem( lastPlayed );

    uint time = track->lastPlayed().toTime_t();
    QMap<uint, QGraphicsLayoutItem*>::const_iterator iBound( m_items.insertMulti( time, itemLayout ) );
    QMap<uint, QGraphicsLayoutItem*>::const_iterator i = m_items.constEnd();
    int index = m_layout->count() - 1;
    while( i-- != iBound )
        --index;
    m_layout->insertItem( index, itemLayout );
}

void
RecentlyPlayedListWidget::removeLast()
{
    if( m_items.isEmpty() )
        return;

    QGraphicsLayoutItem *item = *m_items.end();
    m_items.erase( m_items.end() );
    m_layout->removeItem( item );
    delete item;
}

void
RecentlyPlayedListWidget::startQuery()
{
    DEBUG_BLOCK
    m_recentTracks.clear();

    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    connect( qm, SIGNAL(newResultReady(QString, Meta::TrackList)),
             this, SLOT(tracksReturned(QString, Meta::TrackList)) );
    connect( qm, SIGNAL(queryDone()), SLOT(setupTracksData()) );

    qm->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Track )
      ->excludeFilter( Meta::valTitle, QString(), true, true )
      ->orderBy( Meta::valLastPlayed, true )
      ->limitMaxResultSize( 10 )
      ->run();
}

void
RecentlyPlayedListWidget::trackChanged( Meta::TrackPtr track )
{
    if( !track )
        return;
    if( track != m_currentTrack )
        return;

    m_currentTrack = track;
    addTrack( track );
    removeLast();
}

void
RecentlyPlayedListWidget::clear()
{
    int count = m_layout->count();
    while( --count >= 0 )
    {
        QGraphicsLayoutItem *child = m_layout->itemAt( 0 );
        m_layout->removeItem( child );
        delete child;
    }
    m_items.clear();
}

void
RecentlyPlayedListWidget::tracksReturned( QString id, Meta::TrackList tracks )
{
    Q_UNUSED( id );
    m_recentTracks << tracks;
}

void
RecentlyPlayedListWidget::setupTracksData()
{
    DEBUG_BLOCK
    foreach( const Meta::TrackPtr &track, m_recentTracks )
        addTrack( track );
    m_recentTracks.clear();
}

#include "RecentlyPlayedListWidget.moc"
