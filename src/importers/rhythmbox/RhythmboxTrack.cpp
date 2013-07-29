/****************************************************************************************
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

#include "RhythmboxTrack.h"

#include "MetaValues.h"

using namespace StatSyncing;

RhythmboxTrack::RhythmboxTrack( const QMap<qint64, QString> &metadata )
    : Track()
    , m_metadata( metadata )
{
}

RhythmboxTrack::~RhythmboxTrack()
{
}

QString
RhythmboxTrack::name() const
{
    return m_metadata[Meta::valTitle];
}

QString
RhythmboxTrack::album() const
{
    return m_metadata[Meta::valAlbum];
}

QString
RhythmboxTrack::artist() const
{
    return m_metadata[Meta::valArtist];
}

QString
RhythmboxTrack::composer() const
{
    return m_metadata[Meta::valComposer];
}

int
RhythmboxTrack::year() const
{
    return m_metadata[Meta::valYear].toInt();
}

int
RhythmboxTrack::trackNumber() const
{
    return m_metadata[Meta::valTrackNr].toInt();
}

int
RhythmboxTrack::discNumber() const
{
    return m_metadata[Meta::valDiscNr].toInt();
}

int
RhythmboxTrack::rating() const
{
    return m_metadata[Meta::valRating].toInt() * 2;
}

QDateTime
RhythmboxTrack::firstPlayed() const
{
    return QDateTime();
}

QDateTime
RhythmboxTrack::lastPlayed() const
{
    return QDateTime::fromTime_t( m_metadata[Meta::valLastPlayed].toUInt() );
}

int
RhythmboxTrack::playCount() const
{
    return m_metadata[Meta::valPlaycount].toInt();
}

QSet<QString>
RhythmboxTrack::labels() const
{
    return QSet<QString>();
}
