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

#include "SimpleTrack.h"

using namespace StatSyncing;

SimpleTrack::SimpleTrack( const Meta::FieldHash &metadata, const QSet<QString> &labels )
    : m_labels( labels )
    , m_metadata( metadata )
{
}

SimpleTrack::~SimpleTrack()
{
}

QString
SimpleTrack::name() const
{
    return m_metadata.value( Meta::valTitle ).toString();
}

QString
SimpleTrack::album() const
{
    return m_metadata.value( Meta::valAlbum ).toString();
}

QString
SimpleTrack::artist() const
{
    return m_metadata.value( Meta::valArtist ).toString();
}

QString
SimpleTrack::composer() const
{
    return m_metadata.value( Meta::valComposer ).toString();
}

int
SimpleTrack::year() const
{
    return m_metadata.value( Meta::valYear ).toInt();
}

int
SimpleTrack::trackNumber() const
{
    return m_metadata.value( Meta::valTrackNr ).toInt();
}

int
SimpleTrack::discNumber() const
{
    return m_metadata.value( Meta::valDiscNr ).toInt();
}

QDateTime
SimpleTrack::firstPlayed() const
{
    return getDateTime( m_metadata.value( Meta::valFirstPlayed ) );
}

QDateTime
SimpleTrack::lastPlayed() const
{
    return getDateTime( m_metadata.value( Meta::valLastPlayed ) );
}

int
SimpleTrack::rating() const
{
    return m_metadata.value( Meta::valRating ).toInt();
}

int
SimpleTrack::playCount() const
{
    return m_metadata.value( Meta::valPlaycount ).toInt();
}

QSet<QString>
SimpleTrack::labels() const
{
    return m_labels;
}

QDateTime
SimpleTrack::getDateTime( const QVariant &v ) const
{
    if( v.toDateTime().isValid() )
        return v.toDateTime();
    else if( v.toUInt() != 0 )
        return QDateTime::fromTime_t( v.toUInt() );
    else
        return QDateTime();
}
