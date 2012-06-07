/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "CollectionTrack.h"

#include "core/meta/Statistics.h"

using namespace StatSyncing;

CollectionTrack::CollectionTrack( Meta::TrackPtr track )
    : m_track( track )
    , m_trackStats( track->statistics() )
{
    Q_ASSERT( m_track );
    Q_ASSERT( m_trackStats );
}

CollectionTrack::~CollectionTrack()
{
}

QString
CollectionTrack::name() const
{
    return m_track->name();
}

QString
CollectionTrack::album() const
{
    Meta::AlbumPtr album = m_track->album();
    return album ? album->name() : QString();
}

QString
CollectionTrack::artist() const
{
    Meta::ArtistPtr artist = m_track->artist();
    return artist ? artist->name() : QString();
}

QString
CollectionTrack::composer() const
{
    Meta::ComposerPtr composer = m_track->composer();
    return composer ? composer->name() : QString();
}

int
CollectionTrack::year() const
{
    Meta::YearPtr year = m_track->year();
    return year ? year->year() : 0;
}

int
CollectionTrack::trackNumber() const
{
    return m_track->trackNumber();
}

int
CollectionTrack::discNumber() const
{
    return m_track->discNumber();
}

int
CollectionTrack::rating() const
{
    return qBound<int>( 0, m_trackStats->rating(), 10 );
}

QDateTime
CollectionTrack::firstPlayed() const
{
    return m_trackStats->firstPlayed();
}

QDateTime
CollectionTrack::lastPlayed() const
{
    return m_trackStats->lastPlayed();
}

int
CollectionTrack::playcount() const
{
    return m_trackStats->playCount();
}

QSet<QString>
CollectionTrack::labels() const
{
    Meta::LabelList labels = m_track->labels();
    QSet<QString> labelNames;
    foreach( Meta::LabelPtr label, labels )
        labelNames.insert( label->name() );
    return labelNames;
}
