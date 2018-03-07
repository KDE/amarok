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

#include "AlbumsDefs.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"

#include <QCollator>
#include <QFont>
#include <QMutexLocker>

TrackItem::TrackItem()
    : QStandardItem()
{
    setEditable( false );
}

TrackItem::~TrackItem()
{
    QMutexLocker locker( &m_mutex );
    if( m_track )
        unsubscribeFrom( m_track );
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
    QMutexLocker locker( &m_mutex );
    if( !track )
        return;

    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumPtr  album = track->album();

    setData( track->prettyName(), NameRole );
    setData( track->trackNumber(), TrackNumberRole );
    setData( track->length(), TrackLengthRole );

    if( artist )
        setData( artist->prettyName(), TrackArtistRole );

    if( album )
    {
        setData( album->isCompilation(), AlbumCompilationRole );
        int num = 0;
        foreach( const Meta::TrackPtr &track, album->tracks() )
        {
            if( num < track->trackNumber() )
                num = track->trackNumber();
        }
        setData( num, AlbumMaxTrackNumberRole );
    }
    setToolTip( QString( "%1 (%2)" ).arg( track->name(), Meta::msToPrettyTime(track->length()) ) );
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

int
TrackItem::type() const
{
    return TrackType;
}

bool
TrackItem::operator<( const QStandardItem &other ) const
{
    int trackA = data( TrackNumberRole ).toInt();
    int trackB = other.data( TrackNumberRole ).toInt();
    if( trackA < trackB )
        return true;
    else if( trackA == trackB )
    {
        const QString nameA = data( NameRole ).toString();
        const QString nameB = other.data( NameRole ).toString();
        QCollator collator;
        collator.setNumericMode( true );
        collator.setCaseSensitivity( Qt::CaseInsensitive );
        return collator.compare( nameA, nameB ) < 0;
    }
    else
        return false;
}
