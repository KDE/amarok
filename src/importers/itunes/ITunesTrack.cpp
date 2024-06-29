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

#include "ITunesTrack.h"

#include <QReadLocker>
#include <QWriteLocker>

using namespace StatSyncing;

ITunesTrack::ITunesTrack( const int trackId, const Meta::FieldHash &metadata )
    : SimpleWritableTrack( metadata )
    , m_trackId( trackId )
{
}

ITunesTrack::~ITunesTrack()
{
}

int
ITunesTrack::rating() const
{
    return SimpleWritableTrack::rating() / 10;
}

void
ITunesTrack::setRating( int rating )
{
    if( rating & 1 )
        ++rating;

    SimpleWritableTrack::setRating( rating * 10 );
}

QDateTime
ITunesTrack::lastPlayed() const
{
    QReadLocker lock( &m_lock );

    QDateTime date = QDateTime::fromString(
                                m_statistics.value( Meta::valLastPlayed ).toString(),
                                QStringLiteral("yyyy'-'MM'-'dd'T'hh':'mm':'ss'Z'") );
    if( date.isValid() )
        date.setTimeSpec( Qt::UTC );

    return date;
}

void
ITunesTrack::doCommit( const qint64 changes )
{
    Q_UNUSED( changes );
    Q_EMIT commitCalled( m_trackId, m_statistics );
}
