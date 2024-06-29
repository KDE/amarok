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

#include "BansheeTrack.h"

#include "importers/ImporterSqlConnection.h"

#include <QStringList>

using namespace StatSyncing;

BansheeTrack::BansheeTrack( const qint64 trackId,
                            const ImporterSqlConnectionPtr &connection,
                            const Meta::FieldHash &metadata )
    : SimpleWritableTrack( metadata )
    , m_connection( connection )
    , m_trackId( trackId )
{
}

BansheeTrack::~BansheeTrack()
{
}

int
BansheeTrack::rating() const
{
    return SimpleWritableTrack::rating() * 2;
}

void
BansheeTrack::setRating( int rating )
{
    SimpleWritableTrack::setRating( (rating + 1) / 2 );
}

void
BansheeTrack::doCommit( const qint64 fields )
{
    QStringList updates;
    QVariantMap bindValues;
    if( fields & Meta::valLastPlayed )
    {
        updates << QStringLiteral("LastPlayedStamp = :lastplayed");
        bindValues.insert( QStringLiteral(":lastplayed"), m_statistics.value( Meta::valLastPlayed ) );
    }
    if( fields & Meta::valRating )
    {
        updates << QStringLiteral("Rating = :rating");
        bindValues.insert( QStringLiteral(":rating"), m_statistics.value( Meta::valRating ) );
    }
    if( fields & Meta::valPlaycount )
    {
        updates << QStringLiteral("PlayCount = :playcount");
        bindValues.insert( QStringLiteral(":playcount"), m_statistics.value( Meta::valPlaycount ) );
    }

    if( !updates.empty() )
    {
        const QString query = QStringLiteral("UPDATE coretracks SET ") + updates.join(QStringLiteral(", ")) +
                              QStringLiteral(" WHERE TrackID = :id");

        bindValues.insert( QStringLiteral(":id"), m_trackId );
        m_connection->query( query, bindValues );
    }
}
