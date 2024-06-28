/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TagStatisticsStore.h"

#include <core/storage/SqlStorage.h>
#include "core/meta/Meta.h"
#include "core-impl/storage/StorageManager.h"

TagStatisticsStore::TagStatisticsStore( Meta::Track *track )
    : PersistentStatisticsStore( track )
    , m_name( track->name() )
    , m_artist( track->artist() ? track->artist()->name() : QString() )
    , m_album( track->album() ? track->album()->name() : QString() )
{
    auto sql = StorageManager::instance()->sqlStorage();

    const QString query = QStringLiteral("SELECT firstPlayed, lastPlayed, score, rating, playcount FROM "
                          "statistics_tag WHERE name = '%1' AND artist = '%2' AND album = '%3'");
    QStringList result = sql->query( query.arg( sql->escape( m_name ),
                                                sql->escape( m_artist ),
                                                sql->escape( m_album ) ) );
    if( !result.isEmpty() )
    {
        m_firstPlayed = QDateTime::fromString( result.value( 0 ), s_sqlDateFormat );
        m_lastPlayed = QDateTime::fromString( result.value( 1 ), s_sqlDateFormat );
        m_score = result.value( 2 ).toDouble();
        m_rating = result.value( 3 ).toInt();
        m_playCount = result.value( 4 ).toInt();
    }
}

void
TagStatisticsStore::save()
{
    auto sql = StorageManager::instance()->sqlStorage();

    const QString check = QStringLiteral("SELECT COUNT(*) FROM statistics_tag WHERE name = '%1' "
                          "AND artist = '%2' AND album = '%3'");
    QStringList rsCheck = sql->query( check.arg( sql->escape( m_name ),
                                                 sql->escape( m_artist ),
                                                 sql->escape( m_album ) ) );
    if( !rsCheck.isEmpty() )
    {
        QString sqlString;
        if( rsCheck.first().toInt() )
        {
            sqlString = QStringLiteral("UPDATE statistics_tag SET firstPlayed = '%1',lastPlayed = '%2',"
                        "score = %3,rating = %4,playcount=%5 WHERE name = '%6' "
                        "AND artist = '%7' AND album = '%8'");
        }
        else
        {
            sqlString = QStringLiteral("INSERT INTO statistics_tag(firstPlayed,lastPlayed,score,"
                        "rating,playcount,name,artist,album) "
                        "VALUE ('%1','%2',%3,%4,%5,'%6','%7','%8')");
        }
        sqlString = sqlString.arg( m_firstPlayed.toString( s_sqlDateFormat ),
                                   m_lastPlayed.toString( s_sqlDateFormat ),
                                   QString::number( m_score ),
                                   QString::number( m_rating ),
                                   QString::number( m_playCount ),
                                   sql->escape( m_name ),
                                   sql->escape( m_artist ),
                                   sql->escape( m_album ) );
        sql->query( sqlString );
    }
}
