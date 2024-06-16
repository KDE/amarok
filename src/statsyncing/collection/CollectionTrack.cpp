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

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Debug.h"

using namespace StatSyncing;

CollectionTrack::CollectionTrack( Meta::TrackPtr track )
    : m_track( track )
    , m_trackStats( track->statistics() )
    , m_beginUpdateAlreadyCalled( false )
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

void
CollectionTrack::setRating( int rating )
{
    beginUpdate();
    m_trackStats->setRating( rating );
}

QDateTime
CollectionTrack::firstPlayed() const
{
    return m_trackStats->firstPlayed();
}

void
CollectionTrack::setFirstPlayed( const QDateTime &firstPlayed )
{
    beginUpdate();
    m_trackStats->setFirstPlayed( firstPlayed );
}

QDateTime
CollectionTrack::lastPlayed() const
{
    return m_trackStats->lastPlayed();
}

void
CollectionTrack::setLastPlayed( const QDateTime &lastPlayed )
{
    beginUpdate();
    m_trackStats->setLastPlayed( lastPlayed );
}

int
CollectionTrack::playCount() const
{
    return m_trackStats->playCount();
}

int
CollectionTrack::recentPlayCount() const
{
    return m_trackStats->recentPlayCount();
}

void
CollectionTrack::setPlayCount( int playCount )
{
    beginUpdate();
    m_trackStats->setPlayCount( playCount );
}

QSet<QString>
CollectionTrack::labels() const
{
    Meta::LabelList labels = m_track->labels();
    QSet<QString> labelNames;
    for( Meta::LabelPtr label : labels )
        labelNames.insert( label->name() );
    return labelNames;
}

void
CollectionTrack::setLabels( const QSet<QString> &labels )
{
    QSet<QString> existingLabels;
    QMap<QString, Meta::LabelPtr> existingLabelsMap;
    for( const Meta::LabelPtr &label : m_track->labels() )
    {
        existingLabels.insert( label->name() );
        existingLabelsMap.insert( label->name(), label );
    }

    QSet<QString> toRemove = existingLabels - labels;
    for( const QString &labelName : toRemove )
    {
        Q_ASSERT( existingLabelsMap.contains( labelName ) );
        m_track->removeLabel( existingLabelsMap.value( labelName ) );
    }

    QSet<QString> toAdd = labels - existingLabels;
    for( const QString &labelName : toAdd )
    {
        m_track->addLabel( labelName );
    }
}

Meta::TrackPtr
CollectionTrack::metaTrack() const
{
    return m_track;
}

void
CollectionTrack::commit()
{
    if( !m_beginUpdateAlreadyCalled )
        return;

    m_trackStats->endUpdate();
    m_beginUpdateAlreadyCalled = false;
}

void
CollectionTrack::beginUpdate()
{
    if( m_beginUpdateAlreadyCalled )
        return;

    m_trackStats->beginUpdate();
    m_beginUpdateAlreadyCalled = true;
}
