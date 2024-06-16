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

#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

#include "QEvent"
#include "QHelpEvent"
#include "QToolTip"

PlaylistInfoWidget::PlaylistInfoWidget( QWidget *parent )
    : QLabel( parent )
{
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::dataChanged,
             this, &PlaylistInfoWidget::updateTotalPlaylistLength );
    // Ignore The::playlist() layoutChanged: rows moving around does not change the total playlist length.
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::modelReset,
             this, &PlaylistInfoWidget::updateTotalPlaylistLength );
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::rowsInserted,
             this, &PlaylistInfoWidget::updateTotalPlaylistLength );
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::rowsRemoved,
             this, &PlaylistInfoWidget::updateTotalPlaylistLength );

    updateTotalPlaylistLength();
}

PlaylistInfoWidget::~PlaylistInfoWidget()
{}

void
PlaylistInfoWidget::updateTotalPlaylistLength() //SLOT
{
    const quint64 totalLength = The::playlist()->totalLength();
    const int trackCount = The::playlist()->qaim()->rowCount();

    if( totalLength > 0 && trackCount > 0 )
    {
        const QString prettyTotalLength = Meta::msToPrettyTime( totalLength );
        setText( i18ncp( "%1 is number of tracks, %2 is time",
                         "%1 track (%2)", "%1 tracks (%2)",
                         trackCount, prettyTotalLength ) );
    }
    else if( ( totalLength == 0 ) && ( trackCount > 0 ) )
    {
        setText( i18ncp( "%1 is number of tracks", "%1 track", "%1 tracks", trackCount ) );
    }
    else // Total Length will not be > 0 if trackCount is 0, so we can ignore it
    {
        setText( i18n( "No tracks" ) );
    }
}

bool
PlaylistInfoWidget::event( QEvent *event )
{
    if( event->type() == QEvent::ToolTip ) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        const quint64 totalLength = The::playlist()->totalLength();
        const int trackCount = The::playlist()->qaim()->rowCount();

        if( totalLength == 0 || trackCount == 0 )
        {
            QToolTip::hideText();
            event->ignore();
        }
        else
        {
            // - determine the queued tracks
            quint64 queuedTotalLength( 0 );
            quint64 queuedTotalSize( 0 );
            int queuedCount( 0 );

            QQueue<quint64> queue = The::playlistActions()->queue();
            for( quint64 id : queue )
            {
                Meta::TrackPtr track = The::playlist()->trackForId( id );
                queuedTotalLength += track->length();
                queuedTotalSize += track->filesize();
                ++queuedCount;
            }

            // - set the tooltip
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

            QToolTip::showText( helpEvent->globalPos(), tooltipLabel );
        }

        return true;
    }
    return QWidget::event(event);
}

