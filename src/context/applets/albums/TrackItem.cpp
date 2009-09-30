/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "TrackItem.h"
#include "meta/MetaUtility.h"

#include <QFont>

TrackItem::TrackItem()
    : QStandardItem()
{
    setEditable( false );
}

void
TrackItem::setTrack( Meta::TrackPtr trackPtr )
{
    if( m_track )
        unsubscribeFrom( m_track );
    m_track = trackPtr;
    subscribeTo( m_track );

    metadataChanged( m_track );
}

void
TrackItem::metadataChanged( Meta::TrackPtr track )
{
    int trackNumber = track->trackNumber();
    QString trackName = track->prettyName();
    QString trackArtist = track->artist()->prettyName();
    QString trackTime = Meta::msToPrettyTime( track->length() );
    bool isCompilation = track->album()->isCompilation();

    QString text;

    if( isCompilation ) {
        if( trackNumber > 0 )
            text = QString( "%1  %2 - %3 (%4)" ).arg( QString::number( trackNumber ), 4, ' ').arg(trackArtist).arg(trackName).arg(trackTime);
        else
            text = QString( "    %1 - %2 (%3)" ).arg( trackArtist, trackName, trackTime );
    } else {
        if( trackNumber > 0 )
            text = QString( "%1  %2 (%3)" ).arg( QString::number( trackNumber ), 4, ' ').arg(trackName).arg(trackTime);
        else
            text = QString( "    %1 (%2)" ).arg( trackName, trackTime );
    }

    setText( text );
}

void
TrackItem::italicise()
{
    QFont f = font();
    f.setItalic( true );
    setFont( f );
}

void
TrackItem::bold()
{
    QFont f = font();
    f.setBold( true );
    setFont( f );
}
