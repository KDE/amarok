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

#include "ClementineTrack.h"

#include "importers/ImporterSqlConnection.h"

#include <QReadLocker>
#include <QStringList>
#include <QWriteLocker>

using namespace StatSyncing;

ClementineTrack::ClementineTrack( const QVariant &filename,
                                  const ImporterSqlConnectionPtr &connection,
                                  const Meta::FieldHash &metadata )
    : SimpleWritableTrack( metadata )
    , m_connection( connection )
    , m_filename( filename )
{
}

ClementineTrack::~ClementineTrack()
{
}

int
ClementineTrack::year() const
{
    const int yr = SimpleWritableTrack::year();
    return yr == -1 ? 0 : yr;
}

int
ClementineTrack::trackNumber() const
{
    const int tn = SimpleWritableTrack::trackNumber();
    return tn == -1 ? 0 : tn;
}

int
ClementineTrack::discNumber() const
{
    const int dn = SimpleWritableTrack::discNumber();
    return dn == -1 ? 0 : dn;
}

QDateTime
ClementineTrack::lastPlayed() const
{
    QReadLocker lock( &m_lock );
    const int lp = m_statistics.value( Meta::valLastPlayed ).toInt();
    return lp == -1 ? QDateTime() : getDateTime( lp );
}

void
ClementineTrack::setLastPlayed( const QDateTime &lastPlayed )
{
    QWriteLocker lock( &m_lock );
        m_statistics.insert( Meta::valLastPlayed, lastPlayed.isValid()
                                                  ? lastPlayed.toSecsSinceEpoch() : -1 );
    m_changes |= Meta::valLastPlayed;
}

int
ClementineTrack::playCount() const
{
    const int pc = SimpleWritableTrack::playCount();
    return pc == -1 ? 0 : pc;
}

void
ClementineTrack::setPlayCount( int playCount )
{
    SimpleWritableTrack::setPlayCount( playCount == 0 ? -1 : playCount );
}

int
ClementineTrack::rating() const
{
    QReadLocker lock( &m_lock );
    const qreal rt = m_statistics.value( Meta::valRating ).toReal();
    return rt < 0 ? 0 : qRound( rt * 10 );
}

void
ClementineTrack::setRating( int rating )
{
    QWriteLocker lock( &m_lock );
    m_statistics.insert( Meta::valRating, rating == 0 ? -1.0 : 0.1 * rating );
    m_changes |= Meta::valRating;
}

void
ClementineTrack::doCommit( const qint64 fields )
{
    QStringList updates;
    QVariantMap bindValues;

    if( fields & Meta::valLastPlayed )
    {
        updates << QStringLiteral("lastplayed = :lastplayed");
        bindValues.insert( QStringLiteral(":lastplayed"), m_statistics.value( Meta::valLastPlayed ) );
    }
    if( fields & Meta::valRating )
    {
        updates << QStringLiteral("rating = :rating");
        bindValues.insert( QStringLiteral(":rating"), m_statistics.value( Meta::valRating ) );
    }
    if( fields & Meta::valPlaycount )
    {
        updates << QStringLiteral("playcount = :playcount");
        bindValues.insert( QStringLiteral(":playcount"), m_statistics.value( Meta::valPlaycount ) );
    }

    if( !updates.empty() )
    {
        const QString query = QStringLiteral("UPDATE songs SET ") + updates.join(QStringLiteral(", ")) +
                              QStringLiteral(" WHERE filename = :name");

        bindValues.insert( QStringLiteral(":name"), m_filename );
        m_connection->query( query, bindValues );
    }
}
