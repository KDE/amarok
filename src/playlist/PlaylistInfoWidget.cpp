/****************************************************************************************
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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

#include "PlaylistInfoWidget.h"

#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"

#include <QLabel>
#include <QHBoxLayout>

PlaylistInfoWidget::PlaylistInfoWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent)
    , m_playlistLengthLabel(new QLabel(this))
{
    connect( Playlist::ModelStack::instance()->bottom(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( updateTotalPlaylistLength() ) );
    // Ignore The::playlist() layoutChanged: rows moving around does not change the total playlist length.
    connect( Playlist::ModelStack::instance()->bottom(), SIGNAL( modelReset() ), this, SLOT( updateTotalPlaylistLength() ) );
    connect( Playlist::ModelStack::instance()->bottom(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( updateTotalPlaylistLength() ) );
    connect( Playlist::ModelStack::instance()->bottom(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( updateTotalPlaylistLength() ) );

    QHBoxLayout* hbox = new QHBoxLayout;
    setLayout(hbox);

    hbox->addWidget(m_playlistLengthLabel);

    updateTotalPlaylistLength();
}

PlaylistInfoWidget::~PlaylistInfoWidget()
{
}

void
PlaylistInfoWidget::updateTotalPlaylistLength() //SLOT
{
    DEBUG_BLOCK

    const quint64 totalLength = The::playlist()->totalLength();
    const int trackCount = The::playlist()->qaim()->rowCount();

    if( totalLength > 0 && trackCount > 0 )
    {
        const QString prettyTotalLength = Meta::msToPrettyTime( totalLength );
        m_playlistLengthLabel->setText( i18ncp( "%1 is number of tracks, %2 is time",
                                                "%1 track (%2)", "%1 tracks (%2)",
                                                trackCount, prettyTotalLength ) );
        m_playlistLengthLabel->show();

        quint64 queuedTotalLength( 0 );
        quint64 queuedTotalSize( 0 );
        int queuedCount( 0 );

        for( int i = 0; i < trackCount; ++i )
        {
            if( The::playlist()->queuePositionOfRow( i ) != 0 )
            {
                queuedTotalLength += The::playlist()->trackAt( i )->length();
                queuedTotalSize += The::playlist()->trackAt( i )->filesize();
                ++queuedCount;
            }
        }

        const quint64 totalSize = The::playlist()->totalSize();
        const QString prettyTotalSize = Meta::prettyFilesize( totalSize );
        const QString prettyQueuedTotalLength = Meta::msToPrettyTime( queuedTotalLength );
        const QString prettyQueuedTotalSize   = Meta::prettyFilesize( queuedTotalSize );

        QString tooltipLabel;
        if( queuedCount > 0 && queuedTotalLength > 0 )
        {
            tooltipLabel = i18n( "Total playlist size: %1", prettyTotalSize )       + '\n'
                         + i18n( "Queue size: %1",          prettyQueuedTotalSize ) + '\n'
                         + i18n( "Queue length: %1",        prettyQueuedTotalLength );
        }
        else
        {
            tooltipLabel = i18n( "Total playlist size: %1", prettyTotalSize );
        }

        m_playlistLengthLabel->setToolTip( tooltipLabel );
    }
    else if( ( totalLength == 0 ) && ( trackCount > 0 ) )
    {
        m_playlistLengthLabel->setText( i18ncp( "%1 is number of tracks", "%1 track", "%1 tracks", trackCount ) );
        m_playlistLengthLabel->show();
        m_playlistLengthLabel->setToolTip( 0 );
    }
    else // Total Length will not be > 0 if trackCount is 0, so we can ignore it
    {
        m_playlistLengthLabel->setText( i18n( "No tracks" ) );
    }
}
